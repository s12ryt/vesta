#pragma once

// Shared contract between the external Vesta client and the injected HvH module.
// This header must stay dependency-light (C++ core only) because it is compiled
// into BOTH the external executable (which uses the precompiled stdafx header)
// and the standalone injected DLL (which does not).

#include <cstdint>

namespace vesta::hvh_shared
{
	inline constexpr std::uint32_t k_magic = 0x56485648u; // 'HVHV' in little endian
	inline constexpr std::uint32_t k_version = 1u;

	// Local per-session mapping. Both sides open it by name via
	// CreateFileMappingW / OpenFileMappingW. "Local\\" keeps it inside the
	// current logon session, which is sufficient since the game runs in the
	// same session as the external client.
	inline constexpr wchar_t k_mapping_name[] = L"Local\\vesta_hvh_shared_v1";

	enum class aim_mode : std::int32_t
	{
		nearest = 0,
		lowest_hp = 1,
		crosshair = 2,
	};

	enum class hitbox_mode : std::int32_t
	{
		head = 0,
		upper_chest = 1,
		chest = 2,
		pelvis = 3,
		nearest = 4,
	};

	enum class yaw_mode : std::int32_t
	{
		off = 0,
		spin = 1,
		jitter = 2,
		fake = 3,
	};

	enum class pitch_mode : std::int32_t
	{
		off = 0,
		down = 1,
		up = 2,
		zero = 3,
		fake_down = 4,
	};

	enum class priority_mode : std::int32_t
	{
		distance = 0,
		fov = 1,
		crosshair = 2,
	};

	// The external client is the only writer of this block. The DLL reads it.
	struct settings
	{
		// ---- rage ----
		std::int32_t enable_rage{ 0 };
		std::int32_t rage_aim_mode{ 0 };
		std::int32_t rage_hitbox{ 0 };
		std::int32_t rage_priority{ 0 };
		std::int32_t rage_fov{ 180 };
		std::int32_t rage_smoothing{ 100 }; // 1..100, 100 = snap
		std::int32_t rage_autofire{ 0 };
		std::int32_t rage_autowall{ 1 };
		std::int32_t rage_autoscope{ 1 };
		std::int32_t rage_silent{ 1 };
		std::int32_t rage_min_damage{ 1 };
		std::int32_t rage_visible_only{ 0 };
		std::int32_t rage_target_key{ 0 };

		// ---- trigger ----
		std::int32_t enable_trigger{ 0 };
		std::int32_t trigger_delay_ms{ 0 };
		std::int32_t trigger_hitchance{ 100 };
		std::int32_t trigger_key{ 0 };

		// ---- anti-aim ----
		std::int32_t enable_antiaim{ 0 };
		std::int32_t aa_pitch{ 0 };
		std::int32_t aa_yaw_base{ 0 }; // 0 = local view, 1 = at targets
		std::int32_t aa_yaw_mode{ 0 };
		std::int32_t aa_yaw_offset{ 0 }; // -180..180
		std::int32_t aa_spin_speed{ 20 };
		std::int32_t aa_jitter_min{ -90 };
		std::int32_t aa_jitter_max{ 90 };
		std::int32_t aa_jitter_speed{ 5 };
		std::int32_t aa_fake_lag{ 0 };
		std::int32_t aa_desync{ 0 }; // 0..58
		std::int32_t aa_lby_mode{ 0 };

		// ---- movement ----
		std::int32_t enable_bhop{ 0 };
		std::int32_t enable_auto_stop{ 0 };
	};

	// The DLL is the only writer of this block. The external client reads it.
	struct status
	{
		std::int32_t unload_request{ 0 };
		std::int32_t dll_loaded{ 0 };
		std::int32_t hook_ready{ 0 };
		std::int32_t signature_found{ 0 };
		std::int32_t local_player_valid{ 0 };
		std::int32_t targets_found{ 0 };
		std::int32_t last_error{ 0 };
		std::uint32_t heartbeat{ 0 };
		std::uint32_t tick{ 0 };
		std::uint32_t tick_ms{ 0 };
	};

	struct shared_state
	{
		std::uint32_t magic{ k_magic };
		std::uint32_t version{ k_version };
		std::uint32_t size{ static_cast<std::uint32_t>( sizeof( shared_state ) ) };
		std::uint32_t reserved{ 0 };
		settings config{};
		status state{};
	};

	[[nodiscard]] inline bool valid( const shared_state& block ) noexcept
	{
		return block.magic == k_magic
			&& block.version == k_version
			&& block.size == static_cast<std::uint32_t>( sizeof( shared_state ) );
	}
}
