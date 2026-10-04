#pragma once

#include <hvh_shared/hvh_shared.hpp>

namespace vesta::hvh::features
{
	// Scans client.dll, verifies the configured CreateMove signature and, only
	// when it matches, installs the detour. Returns true when the hook is live.
	bool initialize( vesta::hvh_shared::shared_state* shared );

	// Removes the hook if one is installed.
	void shutdown( );

	// ---- pure helpers (kept separate so they are easy to reason about) -------
	// Advances the spin angle by `speed` degrees for the given tick.
	[[nodiscard]] float advance_spin( float current, float speed );

	// Returns the clamped jitter angle for the given tick.
	[[nodiscard]] float jitter_angle( std::int32_t minimum, std::int32_t maximum,
		std::int32_t speed, std::uint32_t tick );

	// Applies the configured pitch mode to a pitch angle in degrees.
	[[nodiscard]] float apply_pitch( std::int32_t mode, float actual );
}
