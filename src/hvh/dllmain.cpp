#include "hvh_internal.hpp"
#include "hvh_features.hpp"

#include <atomic>
#include <cstdlib>

namespace vesta::hvh
{
	namespace
	{
		vesta::hvh_shared::shared_state* g_shared{ nullptr };
		HANDLE g_mapping{ nullptr };
		HANDLE g_worker{ nullptr };
		std::atomic<bool> g_running{ false };
		HMODULE g_module{ nullptr };

		// ---- logging ---------------------------------------------------------
		std::FILE* g_log{ nullptr };

		void open_log( )
		{
			if ( g_log )
			{
				return;
			}
			wchar_t temp[ MAX_PATH ]{};
			const auto length = ::GetTempPathW( MAX_PATH, temp );
			std::wstring path( temp, length );
			path += L"vesta_hvh.log";
			g_log = _wfsopen( path.c_str( ), L"a+", _SH_DENYNO );
		}

		// ---- ipc -------------------------------------------------------------
		bool map_shared( )
		{
			const auto bytes = static_cast<DWORD>( sizeof( vesta::hvh_shared::shared_state ) );
			g_mapping = ::CreateFileMappingW( INVALID_HANDLE_VALUE, nullptr,
				PAGE_READWRITE, 0, bytes, vesta::hvh_shared::k_mapping_name );
			if ( !g_mapping )
			{
				return false;
			}
			const bool fresh = ::GetLastError( ) != ERROR_ALREADY_EXISTS;
			void* view = ::MapViewOfFile( g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, bytes );
			if ( !view )
			{
				::CloseHandle( g_mapping );
				g_mapping = nullptr;
				return false;
			}
			g_shared = static_cast<vesta::hvh_shared::shared_state*>( view );
			if ( fresh || !vesta::hvh_shared::valid( *g_shared ) )
			{
				*g_shared = vesta::hvh_shared::shared_state{};
			}
			g_shared->state.unload_request = 0;
			g_shared->state.dll_loaded = 1;
			g_shared->state.heartbeat = static_cast<std::uint32_t>( ::GetTickCount( ) );
			return true;
		}

		void unmap_shared( )
		{
			if ( g_shared )
			{
				g_shared->state.dll_loaded = 0;
				g_shared->state.hook_ready = 0;
				::UnmapViewOfFile( g_shared );
				g_shared = nullptr;
			}
			if ( g_mapping )
			{
				::CloseHandle( g_mapping );
				g_mapping = nullptr;
			}
		}

		DWORD WINAPI worker( LPVOID )
		{
			open_log( );
			log_line( "vesta_hvh worker started (build %u)", vesta::hvh_shared::k_version );

			if ( !map_shared( ) )
			{
				log_line( "failed to map the shared block; exiting" );
				return 0;
			}

			const bool ready = features::initialize( g_shared );
			if ( !ready )
			{
				log_line( "feature hook was not installed; the module stays passive" );
			}

			while ( g_running.load( ) )
			{
				g_shared->state.heartbeat = static_cast<std::uint32_t>( ::GetTickCount( ) );
				g_shared->state.tick += 1;

				if ( g_shared->state.unload_request != 0 )
				{
					log_line( "unload requested" );
					break;
				}
				::Sleep( 1 );
			}

			features::shutdown( );
			unmap_shared( );
			log_line( "vesta_hvh worker stopped" );
			log_shutdown( );

			g_running.store( false );
			::FreeLibraryAndExitThread( g_module, 0 );
			return 0;
		}
	}

	void log_line( const char* format, ... )
	{
		if ( !g_log )
		{
			return;
		}
		SYSTEMTIME time{};
		::GetLocalTime( &time );
		std::fprintf( g_log, "[%02u:%02u:%02u.%03u] ", time.wHour, time.wMinute,
			time.wSecond, time.wMilliseconds );
		va_list args;
		va_start( args, format );
		std::vfprintf( g_log, format, args );
		va_end( args );
		std::fputc( '\n', g_log );
		std::fflush( g_log );
	}

	void log_shutdown( )
	{
		if ( g_log )
		{
			std::fclose( g_log );
			g_log = nullptr;
		}
	}

	std::uint8_t* module_base( const wchar_t* module_name, std::size_t* out_size )
	{
		const auto module = ::GetModuleHandleW( module_name );
		if ( !module )
		{
			return nullptr;
		}
		if ( out_size )
		{
			const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>( module );
			const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
				reinterpret_cast<const std::uint8_t*>( module ) + dos->e_lfanew );
			*out_size = nt->OptionalHeader.SizeOfImage;
		}
		return reinterpret_cast<std::uint8_t*>( module );
	}

	std::uint8_t* scan_pattern( std::uint8_t* base, std::size_t size, const char* pattern )
	{
		if ( !base || !pattern || !pattern[ 0 ] )
		{
			return nullptr;
		}

		struct token { std::uint8_t value; bool wildcard; };
		token tokens[ 64 ]{};
		std::size_t count{ 0 };

		const char* cursor = pattern;
		while ( *cursor && count < 64 )
		{
			while ( *cursor == ' ' )
			{
				++cursor;
			}
			if ( !*cursor )
			{
				break;
			}
			if ( cursor[ 0 ] == '?' && cursor[ 1 ] == '?' )
			{
				tokens[ count++ ] = { 0, true };
				cursor += 2;
			}
			else
			{
				char pair[ 3 ]{ cursor[ 0 ], cursor[ 1 ], 0 };
				tokens[ count++ ] = {
					static_cast<std::uint8_t>( std::strtoul( pair, nullptr, 16 ) ), false };
				cursor += 2;
			}
		}
		if ( count == 0 )
		{
			return nullptr;
		}

		for ( std::size_t i = 0; i + count <= size; ++i )
		{
			bool match = true;
			for ( std::size_t k = 0; k < count; ++k )
			{
				if ( !tokens[ k ].wildcard && base[ i + k ] != tokens[ k ].value )
				{
					match = false;
					break;
				}
			}
			if ( match )
			{
				return base + i;
			}
		}
		return nullptr;
	}

	bool readable( const void* address, std::size_t size )
	{
		if ( !address || size == 0 )
		{
			return false;
		}
		MEMORY_BASIC_INFORMATION info{};
		if ( ::VirtualQuery( address, &info, sizeof( info ) ) == 0 )
		{
			return false;
		}
		if ( info.State != MEM_COMMIT || ( info.Protect & PAGE_GUARD )
			|| ( info.Protect & PAGE_NOACCESS ) )
		{
			return false;
		}
		const auto start = reinterpret_cast<std::uintptr_t>( address );
		const auto end = start + size;
		const auto region_end = reinterpret_cast<std::uintptr_t>( info.BaseAddress ) + info.RegionSize;
		return end <= region_end;
	}

	bool writable( void* address, std::size_t size )
	{
		if ( !readable( address, size ) )
		{
			return false;
		}
		MEMORY_BASIC_INFORMATION info{};
		::VirtualQuery( address, &info, sizeof( info ) );
		const auto protect = info.Protect & 0xFF;
		return protect == PAGE_READWRITE || protect == PAGE_WRITECOPY
			|| protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
	}

	bool patch( void* address, const std::uint8_t* bytes, std::size_t size )
	{
		if ( !address || !bytes || size == 0 )
		{
			return false;
		}
		DWORD previous{};
		if ( !::VirtualProtect( address, size, PAGE_EXECUTE_READWRITE, &previous ) )
		{
			return false;
		}
		std::memcpy( address, bytes, size );
		::VirtualProtect( address, size, previous, &previous );
		::FlushInstructionCache( ::GetCurrentProcess( ), address, size );
		return true;
	}

	void** find_pointer_entry( std::uint8_t* base, std::size_t size, const void* target )
	{
		if ( !base || size < sizeof( void* ) || !target )
		{
			return nullptr;
		}
		const auto begin = reinterpret_cast<std::uintptr_t>( base );
		const auto end = begin + size;
		const std::size_t count = size / sizeof( void* );
		auto** entries = reinterpret_cast<void**>( base );

		void** fallback{ nullptr };
		for ( std::size_t i = 0; i < count; ++i )
		{
			if ( entries[ i ] != target )
			{
				continue;
			}
			if ( !fallback )
			{
				fallback = entries + i;
			}
			// Prefer a hit inside a run of pointers that all point back into this
			// module: that is the shape of a virtual table.
			const auto previous = i > 0
				? reinterpret_cast<std::uintptr_t>( entries[ i - 1 ] ) : 0;
			const auto next = i + 1 < count
				? reinterpret_cast<std::uintptr_t>( entries[ i + 1 ] ) : 0;
			if ( previous >= begin && previous < end && next >= begin && next < end )
			{
				return entries + i;
			}
		}
		return fallback;
	}

	bool vtable_hook::install( void** slot, void* detour )
	{
		if ( installed( ) || !slot || !detour )
		{
			return false;
		}
		void* original{};
		std::memcpy( &original, slot, sizeof( original ) );
		if ( !original )
		{
			return false;
		}
		void* replacement = detour;
		if ( !patch( slot, reinterpret_cast<const std::uint8_t*>( &replacement ), sizeof( replacement ) ) )
		{
			return false;
		}
		m_slot = slot;
		m_original = original;
		return true;
	}

	void vtable_hook::remove( )
	{
		if ( !m_slot )
		{
			return;
		}
		void* original = m_original;
		( void )patch( m_slot, reinterpret_cast<const std::uint8_t*>( &original ), sizeof( original ) );
		m_slot = nullptr;
		m_original = nullptr;
	}


	const game_offsets& offsets( )
	{
		static const game_offsets table{ "48 8B C4 4C 89 40 18 48 89 48 08 55 53 57 41 55" };
		return table;
	}
}

// ---- module entry point -----------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, LPVOID )
{
	using namespace vesta::hvh;
	switch ( reason )
	{
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls( instance );
		g_module = instance;
		g_running.store( true );
		g_worker = ::CreateThread( nullptr, 0, worker, nullptr, 0, nullptr );
		if ( !g_worker )
		{
			g_running.store( false );
			return FALSE;
		}
		break;
	case DLL_PROCESS_DETACH:
		g_running.store( false );
		features::shutdown( );
		break;
	default:
		break;
	}
	return TRUE;
}
