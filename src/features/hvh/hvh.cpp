#include <stdafx.hpp>
#include <external/json.hpp>

#include <features/hvh/hvh.hpp>

namespace features::hvh {
	namespace {
		constexpr wchar_t k_dll_name[]{ L"vesta_hvh.dll" };
		constexpr wchar_t k_config_name[]{ L"hvh.json" };

		constexpr auto k_remote_load_timeout{ 5'000u };
		constexpr auto k_status_poll_timeout{ 3'000u };
		constexpr auto k_heartbeat_stale_ms{ 2'000u };
	}

	void controller_t::refresh_paths( )
	{
		wchar_t buffer[ MAX_PATH ]{};
		const auto length = ::GetModuleFileNameW( nullptr, buffer, MAX_PATH );
		if ( length == 0 || length >= MAX_PATH )
		{
			m_directory = std::filesystem::current_path( );
		}
		else
		{
			m_directory = std::filesystem::path( std::wstring( buffer, length ) ).parent_path( );
		}
		m_dll_path = m_directory / k_dll_name;
	}

	bool controller_t::ensure_mapping( )
	{
		if ( m_view )
		{
			return true;
		}

		const auto bytes = static_cast<DWORD>( sizeof( vesta::hvh_shared::shared_state ) );
		m_mapping = ::CreateFileMappingW( INVALID_HANDLE_VALUE, nullptr,
			PAGE_READWRITE, 0, bytes, vesta::hvh_shared::k_mapping_name );
		if ( !m_mapping )
		{
			m_last_error = "CreateFileMapping failed for the HvH shared block.";
			return false;
		}

		const bool fresh = ::GetLastError( ) != ERROR_ALREADY_EXISTS;
		void* view = ::MapViewOfFile( m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, bytes );
		if ( !view )
		{
			m_last_error = "MapViewOfFile failed for the HvH shared block.";
			::CloseHandle( m_mapping );
			m_mapping = nullptr;
			return false;
		}

		m_view = static_cast<vesta::hvh_shared::shared_state*>( view );
		if ( fresh || !vesta::hvh_shared::valid( *m_view ) )
		{
			*m_view = vesta::hvh_shared::shared_state{};
		}
		m_view->config = settings;
		m_view->state.unload_request = 0;
		return true;
	}

	void controller_t::release_mapping( )
	{
		if ( m_view )
		{
			::UnmapViewOfFile( m_view );
			m_view = nullptr;
		}
		if ( m_mapping )
		{
			::CloseHandle( m_mapping );
			m_mapping = nullptr;
		}
	}

	void controller_t::initialize( )
	{
		if ( m_initialized )
		{
			return;
		}
		refresh_paths( );
		load( );
		ensure_mapping( );
		m_initialized = true;
	}

	void controller_t::publish( )
	{
		if ( !m_initialized )
		{
			initialize( );
		}
		if ( !m_view )
		{
			return;
		}
		// Only the config half is written so the DLL's status half is preserved.
		m_view->config = settings;
	}

	inject_status controller_t::status( ) const
	{
		if ( m_view && vesta::hvh_shared::valid( *m_view ) && m_view->state.dll_loaded )
		{
			return inject_status::injected;
		}
		return m_status == inject_status::failed
			? inject_status::failed
			: inject_status::not_injected;
	}

	bool controller_t::dll_active( ) const
	{
		if ( !m_view || !vesta::hvh_shared::valid( *m_view ) || !m_view->state.dll_loaded )
		{
			return false;
		}
		const auto now = static_cast<std::uint32_t>( ::GetTickCount( ) );
		return now - m_view->state.heartbeat < k_heartbeat_stale_ms;
	}

	bool controller_t::signature_found( ) const
	{
		return m_view && vesta::hvh_shared::valid( *m_view ) && m_view->state.signature_found != 0;
	}

	bool controller_t::hook_ready( ) const
	{
		return m_view && vesta::hvh_shared::valid( *m_view ) && m_view->state.hook_ready != 0;
	}

	int controller_t::targets_found( ) const
	{
		return m_view && vesta::hvh_shared::valid( *m_view ) ? m_view->state.targets_found : 0;
	}

	std::string_view controller_t::last_error( ) const
	{
		return m_last_error;
	}

	const std::filesystem::path& controller_t::dll_path( ) const
	{
		return m_dll_path;
	}

	bool controller_t::inject( )
	{
		m_last_error.clear( );
		if ( dll_active( ) )
		{
			m_status = inject_status::injected;
			return true;
		}

		refresh_paths( );
		if ( !std::filesystem::exists( m_dll_path ) )
		{
			m_last_error = "vesta_hvh.dll was not found next to the Vesta executable.";
			m_status = inject_status::failed;
			return false;
		}

		if ( !ensure_mapping( ) )
		{
			m_status = inject_status::failed;
			return false;
		}
		m_view->state.unload_request = 0;
		m_view->config = settings;

		const auto pid = static_cast<DWORD>( app::context().process.process_id( ) );
		if ( pid == 0 )
		{
			m_last_error = "the game process is not available.";
			m_status = inject_status::failed;
			return false;
		}

		constexpr auto rights = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION
			| PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;
		const HANDLE process = ::OpenProcess( rights, FALSE, pid );
		if ( !process )
		{
			m_last_error = "OpenProcess failed; run Vesta with elevated rights.";
			m_status = inject_status::failed;
			return false;
		}

		const auto path = m_dll_path.wstring( );
		const auto path_bytes = static_cast<SIZE_T>( ( path.size( ) + 1 ) * sizeof( wchar_t ) );
		void* remote = ::VirtualAllocEx( process, nullptr, path_bytes,
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
		if ( !remote )
		{
			m_last_error = "VirtualAllocEx failed in the game process.";
			::CloseHandle( process );
			m_status = inject_status::failed;
			return false;
		}

		bool ok = ::WriteProcessMemory( process, remote, path.c_str( ), path_bytes, nullptr ) != FALSE;
		if ( ok )
		{
			const auto kernel32 = ::GetModuleHandleW( L"kernel32.dll" );
			const auto load_library = kernel32
				? ::GetProcAddress( kernel32, "LoadLibraryW" )
				: nullptr;
			if ( !load_library )
			{
				m_last_error = "failed to resolve LoadLibraryW.";
				ok = false;
			}
			else
			{
				const HANDLE thread = ::CreateRemoteThread( process, nullptr, 0,
					reinterpret_cast<LPTHREAD_START_ROUTINE>( load_library ),
					remote, 0, nullptr );
				if ( !thread )
				{
					m_last_error = "CreateRemoteThread failed in the game process.";
					ok = false;
				}
				else
				{
					const auto wait = ::WaitForSingleObject( thread, k_remote_load_timeout );
					DWORD exit_code{};
					::GetExitCodeThread( thread, &exit_code );
					::CloseHandle( thread );
					if ( wait != WAIT_OBJECT_0 || exit_code == 0 )
					{
						m_last_error = "the game refused to load vesta_hvh.dll.";
						ok = false;
					}
				}
			}
		}
		else
		{
			m_last_error = "WriteProcessMemory failed in the game process.";
		}

		::VirtualFreeEx( process, remote, 0, MEM_RELEASE );
		::CloseHandle( process );

		if ( !ok )
		{
			m_status = inject_status::failed;
			return false;
		}

		// Give the DLL a moment to map the shared block and report its heartbeat.
		const auto deadline = ::GetTickCount64( ) + k_status_poll_timeout;
		while ( ::GetTickCount64( ) < deadline )
		{
			if ( dll_active( ) )
			{
				m_status = inject_status::injected;
				return true;
			}
			::Sleep( 25 );
		}

		m_last_error = "vesta_hvh.dll loaded but did not report a heartbeat.";
		m_status = inject_status::failed;
		return false;
	}

	bool controller_t::eject( )
	{
		m_last_error.clear( );
		if ( !m_view )
		{
			m_status = inject_status::not_injected;
			return true;
		}

		m_view->state.unload_request = 1;
		const auto deadline = ::GetTickCount64( ) + k_status_poll_timeout;
		while ( ::GetTickCount64( ) < deadline )
		{
			if ( m_view->state.dll_loaded == 0 )
			{
				break;
			}
			::Sleep( 25 );
		}
		m_view->state.unload_request = 0;
		m_status = inject_status::not_injected;
		return true;
	}

	void controller_t::shutdown( )
	{
		if ( m_view )
		{
			m_view->state.unload_request = 1;
		}
		save( );
		release_mapping( );
		m_initialized = false;
	}

	void controller_t::load( )
	{
		if ( m_directory.empty( ) )
		{
			refresh_paths( );
		}
		const auto file = m_directory / k_config_name;
		std::ifstream in( file );
		if ( !in )
		{
			return;
		}

		nlohmann::json j;
		try
		{
			in >> j;
		}
		catch ( ... )
		{
			m_last_error = "hvh.json is malformed; using defaults.";
			return;
		}
		if ( !j.is_object( ) )
		{
			return;
		}

		const auto get = [ &j ]( const char* key, std::int32_t fallback )
		{
			const auto it = j.find( key );
			return it != j.end( ) && it->is_number_integer( )
				? it->get<std::int32_t>( )
				: fallback;
		};

		settings.enable_rage = get( "enable_rage", settings.enable_rage );
		settings.rage_aim_mode = get( "rage_aim_mode", settings.rage_aim_mode );
		settings.rage_hitbox = get( "rage_hitbox", settings.rage_hitbox );
		settings.rage_priority = get( "rage_priority", settings.rage_priority );
		settings.rage_fov = get( "rage_fov", settings.rage_fov );
		settings.rage_smoothing = get( "rage_smoothing", settings.rage_smoothing );
		settings.rage_autofire = get( "rage_autofire", settings.rage_autofire );
		settings.rage_autowall = get( "rage_autowall", settings.rage_autowall );
		settings.rage_autoscope = get( "rage_autoscope", settings.rage_autoscope );
		settings.rage_silent = get( "rage_silent", settings.rage_silent );
		settings.rage_min_damage = get( "rage_min_damage", settings.rage_min_damage );
		settings.rage_visible_only = get( "rage_visible_only", settings.rage_visible_only );
		settings.rage_target_key = get( "rage_target_key", settings.rage_target_key );

		settings.enable_trigger = get( "enable_trigger", settings.enable_trigger );
		settings.trigger_delay_ms = get( "trigger_delay_ms", settings.trigger_delay_ms );
		settings.trigger_hitchance = get( "trigger_hitchance", settings.trigger_hitchance );
		settings.trigger_key = get( "trigger_key", settings.trigger_key );

		settings.enable_antiaim = get( "enable_antiaim", settings.enable_antiaim );
		settings.aa_pitch = get( "aa_pitch", settings.aa_pitch );
		settings.aa_yaw_base = get( "aa_yaw_base", settings.aa_yaw_base );
		settings.aa_yaw_mode = get( "aa_yaw_mode", settings.aa_yaw_mode );
		settings.aa_yaw_offset = get( "aa_yaw_offset", settings.aa_yaw_offset );
		settings.aa_spin_speed = get( "aa_spin_speed", settings.aa_spin_speed );
		settings.aa_jitter_min = get( "aa_jitter_min", settings.aa_jitter_min );
		settings.aa_jitter_max = get( "aa_jitter_max", settings.aa_jitter_max );
		settings.aa_jitter_speed = get( "aa_jitter_speed", settings.aa_jitter_speed );
		settings.aa_fake_lag = get( "aa_fake_lag", settings.aa_fake_lag );
		settings.aa_desync = get( "aa_desync", settings.aa_desync );
		settings.aa_lby_mode = get( "aa_lby_mode", settings.aa_lby_mode );

		settings.enable_bhop = get( "enable_bhop", settings.enable_bhop );
		settings.enable_auto_stop = get( "enable_auto_stop", settings.enable_auto_stop );
	}

	void controller_t::save( )
	{
		if ( m_directory.empty( ) )
		{
			refresh_paths( );
		}
		nlohmann::json j{
			{ "enable_rage", settings.enable_rage },
			{ "rage_aim_mode", settings.rage_aim_mode },
			{ "rage_hitbox", settings.rage_hitbox },
			{ "rage_priority", settings.rage_priority },
			{ "rage_fov", settings.rage_fov },
			{ "rage_smoothing", settings.rage_smoothing },
			{ "rage_autofire", settings.rage_autofire },
			{ "rage_autowall", settings.rage_autowall },
			{ "rage_autoscope", settings.rage_autoscope },
			{ "rage_silent", settings.rage_silent },
			{ "rage_min_damage", settings.rage_min_damage },
			{ "rage_visible_only", settings.rage_visible_only },
			{ "rage_target_key", settings.rage_target_key },
			{ "enable_trigger", settings.enable_trigger },
			{ "trigger_delay_ms", settings.trigger_delay_ms },
			{ "trigger_hitchance", settings.trigger_hitchance },
			{ "trigger_key", settings.trigger_key },
			{ "enable_antiaim", settings.enable_antiaim },
			{ "aa_pitch", settings.aa_pitch },
			{ "aa_yaw_base", settings.aa_yaw_base },
			{ "aa_yaw_mode", settings.aa_yaw_mode },
			{ "aa_yaw_offset", settings.aa_yaw_offset },
			{ "aa_spin_speed", settings.aa_spin_speed },
			{ "aa_jitter_min", settings.aa_jitter_min },
			{ "aa_jitter_max", settings.aa_jitter_max },
			{ "aa_jitter_speed", settings.aa_jitter_speed },
			{ "aa_fake_lag", settings.aa_fake_lag },
			{ "aa_desync", settings.aa_desync },
			{ "aa_lby_mode", settings.aa_lby_mode },
			{ "enable_bhop", settings.enable_bhop },
			{ "enable_auto_stop", settings.enable_auto_stop },
		};

		std::ofstream out( m_directory / k_config_name );
		if ( out )
		{
			out << j.dump( 2 );
		}
	}

	controller_t& controller( )
	{
		static controller_t instance{};
		return instance;
	}

} // namespace features::hvh
