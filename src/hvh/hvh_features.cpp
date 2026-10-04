#include "hvh_internal.hpp"
#include "hvh_features.hpp"

#include <cmath>

namespace vesta::hvh::features
{
	namespace
	{
		vesta::hvh_shared::shared_state* g_shared{ nullptr };
		inline_hook g_hook{};
		float g_spin{ 0.0f };

		// CreateMove ABI is game-version specific. The detour forwards the raw
		// arguments to the trampolined original and only touches state when the
		// module has a verified signature for the surrounding structures.
		using create_move_fn = bool( * )( void*, void*, void* );
		create_move_fn g_original{ nullptr };

		[[nodiscard]] float wrap_angle( const float degrees )
		{
			float value = std::fmod( degrees + 180.0f, 360.0f );
			if ( value < 0.0f )
			{
				value += 360.0f;
			}
			return value - 180.0f;
		}

		bool __fastcall create_move_detour( void* self, void* first, void* second )
		{
			const bool result = g_original ? g_original( self, first, second ) : false;

			if ( !g_shared )
			{
				return result;
			}
			const auto& config = g_shared->config;

			// Anti-aim math is only meaningful once we know where the command
			// viewangles live. Until a build-verified signature is supplied this
			// path stays inert instead of writing to a guessed address.
			const bool antiaim = config.enable_antiaim != 0;
			const bool silent = config.enable_silent != 0;
			if ( !antiaim && !silent )
			{
			return result;
			}
			if ( g_shared->sigs.input[ 0 ] == '\0' )
			{
			return result;
			}
			if ( config.enable_antiaim != 0 )
			{
				g_spin = wrap_angle( g_spin + static_cast<float>( config.aa_spin_speed ) );
				( void )g_spin;
			if ( silent )
			{
			g_spin = wrap_angle( g_spin + static_cast<float>( config.silent_fov ) );
			}
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

		if ( !g_hook.install( target, reinterpret_cast<void*>( &create_move_detour ) ) )
		{
			g_shared->state.last_error = -4;
			log_line( "failed to install the create_move hook" );
			return false;
		}
		g_original = reinterpret_cast<create_move_fn>( g_hook.trampoline( ) );
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
