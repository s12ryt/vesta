#include "hvh_internal.hpp"
#include "hvh_features.hpp"

#include <cmath>

namespace vesta::hvh::features
{
	namespace
	{
		vesta::hvh_shared::shared_state* g_shared{ nullptr };
		vtable_hook g_hook{};
		float g_spin{ 0.0f };

		// The live view angles were proven at CCSGOInput + 0x688 (pitch, yaw,
		// roll) for this build via a motion-diff of the running game, and the
		// third person camera flag sits at CCSGOInput + 0x5201. CreateMove is
		// CCSGOInput::CreateMove, so its first argument is that instance.
		constexpr std::uintptr_t k_view_angles_offset{ 0x688 };
		constexpr std::uintptr_t k_third_person_offset{ 0x5201 };

		using create_move_fn = bool( * )( void*, void*, void* );
		create_move_fn g_original{ nullptr };

		// The aim angles live somewhere inside the CUserCmd. Their offset is not
		// documented for this build, so they are located a few times by searching
		// for the live input angles and the path is then cached. The search is
		// throttled hard: probing every candidate every call made the game lag,
		// so it only runs a handful of times and then gives up.
		enum class angle_path : int { none = 0, inside_command = 1, through_pointer = 2 };
		angle_path g_angle_path{ angle_path::none };
		std::uintptr_t g_angle_pointer_offset{ 0 };
		std::uintptr_t g_angle_offset{ 0 };
		std::uint32_t g_detour_calls{ 0 };
		std::uint32_t g_locate_attempts{ 0 };
		bool g_locate_gave_up{ false };
		bool g_first_call_logged{ false };

		[[nodiscard]] float wrap_angle( const float degrees )
		{
			float value = std::fmod( degrees + 180.0f, 360.0f );
			if ( value < 0.0f )
			{
				value += 360.0f;
			}
			return value - 180.0f;
		}

		// No system call here: the caller validates the whole range once so the
		// inner loop stays a plain memory comparison.
		[[nodiscard]] bool angle_pair_matches( const void* base, const std::uintptr_t offset,
			const float pitch, const float yaw )
		{
			const auto* values = reinterpret_cast<const float*>(
				reinterpret_cast<const std::uint8_t*>( base ) + offset );
			return std::fabs( values[ 0 ] - pitch ) < 1.0f
				&& std::fabs( wrap_angle( values[ 1 ] - yaw ) ) < 1.0f;
		}

		// Finds the current input angles inside the command buffer (or the object
		// it points at) and caches the path that reached them.
		void locate_command_angles( void* self, void* command )
		{
			if ( !self || !command )
			{
				return;
			}
			const auto* input = reinterpret_cast<const std::uint8_t*>( self ) + k_view_angles_offset;
			if ( !readable( input, sizeof( float ) * 2u ) )
			{
				return;
			}
			const auto* current = reinterpret_cast<const float*>( input );
			const float pitch = current[ 0 ];
			const float yaw = current[ 1 ];

			constexpr std::uintptr_t k_command_scan{ 0x600 };
			if ( readable( command, k_command_scan ) )
			{
				for ( std::uintptr_t offset = 0; offset < k_command_scan; offset += 4u )
				{
					if ( angle_pair_matches( command, offset, pitch, yaw ) )
					{
						g_angle_path = angle_path::inside_command;
						g_angle_pointer_offset = 0;
						g_angle_offset = offset;
						log_line( "command angles located inside the command at 0x%llX",
							static_cast< unsigned long long >( offset ) );
						return;
					}
				}
			}

			const auto* bytes = reinterpret_cast<const std::uint8_t*>( command );
			for ( std::uintptr_t pointer_offset = 0; pointer_offset < 0x200u; pointer_offset += 8u )
			{
				if ( !readable( bytes + pointer_offset, sizeof( void* ) ) )
				{
					continue;
				}
				void* target = *reinterpret_cast<void* const*>( bytes + pointer_offset );
				if ( !target || !readable( target, 0x400u ) )
				{
					continue;
				}
				for ( std::uintptr_t offset = 0; offset < 0x400u; offset += 4u )
				{
					if ( angle_pair_matches( target, offset, pitch, yaw ) )
					{
						g_angle_path = angle_path::through_pointer;
						g_angle_pointer_offset = pointer_offset;
						g_angle_offset = offset;
						log_line( "command angles located through pointer 0x%llX at 0x%llX",
							static_cast< unsigned long long >( pointer_offset ),
							static_cast< unsigned long long >( offset ) );
						return;
					}
				}
			}
		}

		// Returns the cached command aim angles. The lookup is allowed a few
		// attempts only, so a failed search cannot stall CreateMove every tick.
		[[nodiscard]] float* command_angles( void* self, void* command )
		{
			if ( g_angle_path == angle_path::none )
			{
				if ( g_locate_gave_up || ( g_detour_calls % 128u ) != 1u )
				{
					return nullptr;
				}
				++g_locate_attempts;
				locate_command_angles( self, command );
				if ( g_angle_path == angle_path::none )
				{
					if ( g_locate_attempts >= 5u )
					{
						g_locate_gave_up = true;
						log_line( "command angles were not found; aim writes stay disabled" );
					}
					return nullptr;
				}
			}
			void* base = nullptr;
			if ( g_angle_path == angle_path::inside_command )
			{
				base = command;
			}
			else if ( g_angle_path == angle_path::through_pointer )
			{
				const auto* slot = reinterpret_cast<const std::uint8_t*>( command ) + g_angle_pointer_offset;
				if ( !readable( slot, sizeof( void* ) ) )
				{
					return nullptr;
				}
				base = *reinterpret_cast<void* const*>( slot );
			}
			if ( !base )
			{
				return nullptr;
			}
			auto* address = reinterpret_cast<std::uint8_t*>( base ) + g_angle_offset;
			if ( !readable( address, sizeof( float ) * 2u ) )
			{
				return nullptr;
			}
			return reinterpret_cast<float*>( address );
		}
		}

		bool __fastcall create_move_detour( void* self, void* first, void* second )
		{
			if ( !g_original )
			{
				return false;
			}

			if ( !g_shared )
			{
				return g_original( self, first, second );
			}

			++g_detour_calls;
			++g_shared->state.hook_calls;
			if ( !g_first_call_logged )
			{
				g_first_call_logged = true;
				log_line( "create_move detour entered (self=%p command=%p)",
					static_cast< const void* >( self ), static_cast< const void* >( second ) );
			}

			const auto& config = g_shared->config;
			const auto& aim = g_shared->aim;

			// Third person is a plain bool on the input object for this build.
			{
				auto* camera = reinterpret_cast<std::uint8_t*>( self ) + k_third_person_offset;
				if ( readable( camera, sizeof( std::uint8_t ) ) )
				{
					*camera = config.enable_thirdperson != 0 ? 1 : 0;
				}
			}

			const bool silent = config.enable_silent != 0 && aim.valid != 0;
			const bool antiaim = config.enable_antiaim != 0;
			if ( !silent && !antiaim )
			{
				return g_original( self, first, second );
			}

			// The game builds the outgoing command and its move checksum from the
			// input view angles, so they are overwritten here and restored right
			// after the original runs. Restoring keeps the local camera still.
			auto* angles = reinterpret_cast<float*>(
				reinterpret_cast<std::uint8_t*>( self ) + k_view_angles_offset );
			if ( !readable( angles, sizeof( float ) * 3u ) )
			{
				return g_original( self, first, second );
			}

			const float saved_pitch = angles[ 0 ];
			const float saved_yaw = angles[ 1 ];
			const float saved_roll = angles[ 2 ];

			if ( silent )
			{
				angles[ 0 ] = aim.pitch;
				angles[ 1 ] = wrap_angle( aim.yaw );
			}
			else
			{
				g_spin = wrap_angle( g_spin + static_cast<float>( config.aa_spin_speed ) );
				angles[ 0 ] = apply_pitch( config.aa_pitch, angles[ 0 ] );
				angles[ 1 ] = wrap_angle( angles[ 1 ] + g_spin );
			}

			const bool result = g_original( self, first, second );

			angles[ 0 ] = saved_pitch;
			angles[ 1 ] = saved_yaw;
			angles[ 2 ] = saved_roll;
			return result;
		}

	float advance_spin( const float current, const float speed )
	{
		float value = std::fmod( current + speed + 180.0f, 360.0f );
		if ( value < 0.0f )
		{
			value += 360.0f;
		}
		return value - 180.0f;
	}

	float jitter_angle( const std::int32_t minimum, const std::int32_t maximum,
		const std::int32_t speed, const std::uint32_t tick )
	{
		if ( speed <= 0 )
		{
			return static_cast<float>( minimum );
		}
		const auto phase = ( tick / static_cast<std::uint32_t>( speed ) ) % 2u;
		return static_cast<float>( phase == 0 ? maximum : minimum );
	}

	float apply_pitch( const std::int32_t mode, const float actual )
	{
		switch ( static_cast<vesta::hvh_shared::pitch_mode>( mode ) )
		{
		case vesta::hvh_shared::pitch_mode::down: return 89.0f;
		case vesta::hvh_shared::pitch_mode::up: return -89.0f;
		case vesta::hvh_shared::pitch_mode::zero: return 0.0f;
		case vesta::hvh_shared::pitch_mode::fake_down: return -89.0f;
		case vesta::hvh_shared::pitch_mode::off:
		default: return actual;
		}
	}

	bool initialize( vesta::hvh_shared::shared_state* shared )
	{
		g_shared = shared;
		if ( !g_shared )
		{
			return false;
		}

		std::size_t client_size{};
		auto* client = module_base( L"client.dll", &client_size );
		if ( !client || client_size == 0 )
		{
			g_shared->state.last_error = -1;
			log_line( "client.dll was not found" );
			return false;
		}
		g_shared->state.signature_found = 0;

		const auto& table = offsets( );
		const char* signature = g_shared->sigs.create_move[ 0 ] != '\0' ? g_shared->sigs.create_move : table.create_move_sig;
		if ( signature[ 0 ] == '\0' )
		{
			g_shared->state.last_error = -2;
			log_line( "create_move signature is not configured; module is passive" );
			return false;
		}

		auto* target = scan_pattern( client, client_size, signature );
		if ( !target )
		{
			g_shared->state.last_error = -3;
			log_line( "create_move signature did not match" );
			return false;
		}
		g_shared->state.signature_found = 1;

		// Resolve the virtual-table slot that holds the scanned function and swap
		// it. Patching the slot instead of the code avoids splitting an
		// instruction inside the prologue (a five-byte JMP corrupted CreateMove
		// and crashed the game) and leaves client.dll .text untouched.
		auto** slot = find_pointer_entry( client, client_size, target );
		if ( !slot )
		{
			g_shared->state.last_error = -4;
			log_line( "create_move vtable slot was not found" );
			return false;
		}

		if ( !g_hook.install( slot, reinterpret_cast<void*>( &create_move_detour ) ) )
		{
			g_shared->state.last_error = -5;
			log_line( "failed to install the create_move vtable hook" );
			return false;
		}
		g_original = reinterpret_cast<create_move_fn>( g_hook.original( ) );

		g_shared->state.hook_ready = 1;
		g_shared->state.last_error = 0;
		log_line( "create_move hook installed" );
		return true;
	}

	void shutdown( )
	{
		if ( g_hook.installed( ) )
		{
			g_hook.remove( );
			log_line( "create_move hook removed" );
		}
		g_original = nullptr;
		if ( g_shared )
		{
			g_shared->state.hook_ready = 0;
		}
	}
}
