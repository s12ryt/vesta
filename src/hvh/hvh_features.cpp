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
		bool g_first_call_logged{ false };
		bool g_first_write_logged{ false };
		bool g_write_failed_logged{ false };

		// CreateMove is CCSGOInput::CreateMove with the signature
		//   double( CCSGOInput* self, unsigned int slot, CUserCmd* command )
		// and it returns a double. The third argument is the command the engine
		// serialises to the server, so steering it is what changes the shot. The
		// offsets follow the published CS2 layouts (VeryElusive
		// internal-cheat-sdk, PELover CS2-Internal-Cheat):
		//   CUserCmd::pBase             @ 0x30  (CBaseUserCmdPB*)
		//   CBaseUserCmdPB::pViewangles @ 0x40  (CCmdQAngle*)
		//   CCmdQAngle::angValue        @ 0x18  (pitch, yaw, roll)
		using create_move_fn = double( * )( void*, unsigned int, void* );
		create_move_fn g_original{ nullptr };

		constexpr std::uintptr_t k_cmd_base_offset{ 0x30 };
		constexpr std::uintptr_t k_base_viewangles_offset{ 0x40 };
		constexpr std::uintptr_t k_angles_value_offset{ 0x18 };

		// The camera angles and the third person flag live on the input object and
		// were both confirmed live on this build.
		constexpr std::uintptr_t k_view_angles_offset{ 0x688 };
		constexpr std::uintptr_t k_third_person_offset{ 0x5201 };

		[[nodiscard]] float wrap_angle( const float degrees )
		{
			float value = std::fmod( degrees + 180.0f, 360.0f );
			if ( value < 0.0f )
			{
				value += 360.0f;
			}
			return value - 180.0f;
		}

		// Walks CUserCmd -> pBase -> pViewangles and returns the writable angle
		// triple, or null when any hop is missing or unreadable.
		[[nodiscard]] float* command_angles( void* command )
		{
			if ( !command )
			{
				return nullptr;
			}
			void* base = nullptr;
			if ( !safe_read( static_cast< const std::uint8_t* >( command ) + k_cmd_base_offset, base ) || !base )
			{
				return nullptr;
			}
			void* view = nullptr;
			if ( !safe_read( static_cast< const std::uint8_t* >( base ) + k_base_viewangles_offset, view ) || !view )
			{
				return nullptr;
			}
			auto* angles = static_cast< std::uint8_t* >( view ) + k_angles_value_offset;
			if ( !readable( angles, sizeof( float ) * 3u ) )
			{
				return nullptr;
			}
			return reinterpret_cast< float* >( angles );
		}
	}

	double __fastcall create_move_detour( void* self, unsigned int slot, void* command )
	{
		if ( !g_original )
		{
			return 0.0;
		}
		if ( !g_shared )
		{
			return g_original( self, slot, command );
		}

		++g_shared->state.hook_calls;
		if ( !g_first_call_logged )
		{
			g_first_call_logged = true;
			log_line( "create_move detour entered (self=%p command=%p)",
				static_cast< const void* >( self ), static_cast< const void* >( command ) );
		}

		const auto& config = g_shared->config;
		const auto& aim = g_shared->aim;

		// Third person is a plain bool on the input object for this build.
		{
			auto* camera = reinterpret_cast< std::uint8_t* >( self ) + k_third_person_offset;
			if ( readable( camera, sizeof( std::uint8_t ) ) )
			{
				*camera = config.enable_thirdperson != 0 ? 1 : 0;
			}
		}

		const bool silent = config.enable_silent != 0 && aim.valid != 0;
		const bool antiaim = config.enable_antiaim != 0;
		if ( !silent && !antiaim )
		{
			return g_original( self, slot, command );
		}

		auto* camera = reinterpret_cast< float* >(
			reinterpret_cast< std::uint8_t* >( self ) + k_view_angles_offset );
		const bool camera_readable = readable( camera, sizeof( float ) * 3u );
		const float saved_pitch = camera_readable ? camera[ 0 ] : 0.0f;
		const float saved_yaw = camera_readable ? camera[ 1 ] : 0.0f;
		const float saved_roll = camera_readable ? camera[ 2 ] : 0.0f;

		// The engine rebuilds the outgoing command inside the original, so the
		// command it produced is the one that gets steered.
		const double result = g_original( self, slot, command );

		float* angles = command_angles( command );
		if ( !angles )
		{
			if ( !g_write_failed_logged )
			{
				g_write_failed_logged = true;
				log_line( "the command view angles could not be resolved; aim writes stay disabled" );
			}
			return result;
		}

		if ( antiaim )
		{
			g_spin = wrap_angle( g_spin + static_cast< float >( config.aa_spin_speed ) );
			angles[ 0 ] = apply_pitch( config.aa_pitch, angles[ 0 ] );
			angles[ 1 ] = wrap_angle( g_spin );
		}
		else
		{
			angles[ 0 ] = aim.pitch;
			angles[ 1 ] = wrap_angle( aim.yaw );
		}

		if ( !g_first_write_logged )
		{
			g_first_write_logged = true;
			log_line( "command view angles written (silent=%d antiaim=%d angles=%p)",
				silent ? 1 : 0, antiaim ? 1 : 0, static_cast< void* >( angles ) );
		}

		// Anti-aim also moves the local view so the spin is visible; silent aim
		// keeps the camera exactly where the user was looking.
		if ( camera_readable )
		{
			if ( antiaim )
			{
				camera[ 0 ] = apply_pitch( config.aa_pitch, saved_pitch );
				camera[ 1 ] = wrap_angle( g_spin );
			}
			else
			{
				camera[ 0 ] = saved_pitch;
				camera[ 1 ] = saved_yaw;
				camera[ 2 ] = saved_roll;
			}
		}

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
