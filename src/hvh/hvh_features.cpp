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
		// roll) for this build via a motion-diff of the running game. CreateMove
		// is CCSGOInput::CreateMove, so its first argument is the CCSGOInput
		// instance and its third argument is the CUserCmd the server will see.
		constexpr std::uintptr_t k_view_angles_offset{ 0x688 };

		using create_move_fn = bool( * )( void*, void*, void* );
		create_move_fn g_original{ nullptr };

		// The aim angles live somewhere inside the CUserCmd. Their offset is not
		// documented for this build, so they are located once by searching for
		// the live input angles and the path is cached for later ticks.
		enum class angle_path : int { none = 0, inside_command = 1, through_pointer = 2 };
		angle_path g_angle_path{ angle_path::none };
		std::uintptr_t g_angle_pointer_offset{ 0 };
		std::uintptr_t g_angle_offset{ 0 };

		[[nodiscard]] float wrap_angle( const float degrees )
		{
			float value = std::fmod( degrees + 180.0f, 360.0f );
			if ( value < 0.0f )
			{
				value += 360.0f;
			}
			return value - 180.0f;
		}

		[[nodiscard]] bool angle_pair_matches( const void* base, const std::uintptr_t offset,
			const float pitch, const float yaw )
		{
			const auto* address = reinterpret_cast<const std::uint8_t*>( base ) + offset;
			if ( !readable( address, sizeof( float ) * 2u ) )
			{
				return false;
			}
			const auto* values = reinterpret_cast<const float*>( address );
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

			for ( std::uintptr_t offset = 0; offset < 0x600u; offset += 4u )
			{
				if ( angle_pair_matches( command, offset, pitch, yaw ) )
				{
					g_angle_path = angle_path::inside_command;
					g_angle_pointer_offset = 0;
					g_angle_offset = offset;
					return;
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
				if ( !target || !readable( target, sizeof( float ) * 2u ) )
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
						return;
					}
				}
			}
		}

		// Returns the cached command aim angles, locating them on first use.
		[[nodiscard]] float* command_angles( void* self, void* command )
		{
			if ( g_angle_path == angle_path::none )
			{
				locate_command_angles( self, command );
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

		bool __fastcall create_move_detour( void* self, void* first, void* second )
		{
			if ( !g_original )
			{
				return false;
			}

			// The original rebuilds the command from the mouse delta, so the aim
			// angles are rewritten afterwards to survive into the outgoing command.
			const bool result = g_original( self, first, second );

			if ( !g_shared )
			{
				return result;
			}
			++g_shared->state.hook_calls;

			const auto& config = g_shared->config;
			const auto& aim = g_shared->aim;
			const bool silent = config.enable_silent != 0 && aim.valid != 0;
			const bool antiaim = config.enable_antiaim != 0;
			if ( !silent && !antiaim )
			{
				return result;
			}

			auto* angles = command_angles( self, second );
			if ( !angles )
			{
				return result;
			}

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
			return result;
		}
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
