#include <stdafx.hpp>
#include <render/menu/localization.hpp>
#include <render/menu/menu.hpp>
#include <render/menu/internal.hpp>
#include <features/hvh/hvh.hpp>

#include <imgui.h>

using namespace render::menu::detail;

void menu_t::draw_hvh()
{
    auto &hvh = features::hvh::controller();
    auto &s = hvh.settings;
    bool changed = false;

    // The shared block stores flags as int32 so both sides agree on the layout;
    // the menu works with bool/int views and writes them back on change.
    const auto toggle_int = [&](const char *label, std::int32_t &value) {
        bool on = value != 0;
        const bool before = on;
        toggle_row(label, on);
        if (on != before)
        {
            value = on ? 1 : 0;
            changed = true;
        }
    };
    const auto slider_int = [&](const char *label, std::int32_t &value, int minimum, int maximum,
                                const char *suffix = "") {
        int view = static_cast<int>(value);
        slider_row(label, view, minimum, maximum, suffix);
        if (view != value)
        {
            value = view;
            changed = true;
        }
    };
    const auto select_int = [&](const char *label, std::int32_t &value,
                                std::span<const char *const> options) {
        int view = static_cast<int>(value);
        select_row(label, view, options);
        if (view != value)
        {
            value = view;
            changed = true;
        }
    };

    const auto status_row = [&](const char *label, const char *value, ImVec4 color) {
        begin_row(label, 148.0f);
        ImGui::TextColored(color, "%s", value);
        end_row();
    };

    const char *state_text = render::localization::tr("Not Injected");
    ImVec4 state_color = k_text_muted;
    switch (hvh.status())
    {
    case features::hvh::inject_status::injected:
        state_text = render::localization::tr("Injected");
        state_color = ImVec4{0.38f, 0.86f, 0.55f, 1.0f};
        break;
    case features::hvh::inject_status::failed:
        state_text = render::localization::tr("Failed");
        state_color = ImVec4{1.0f, 0.38f, 0.42f, 1.0f};
        break;
    default:
        break;
    }

    ImGui::SetCursorPos({14.0f, 24.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    ImGui::BeginChild("##hvh_cards", {682.0f, 612.0f}, false);
    begin_cards("##hvh_grid");

    static constexpr const char *aim_modes[]{"Nearest", "Lowest HP", "Crosshair"};
    static constexpr const char *hitbox_modes[]{"Head", "Upper Chest", "Chest", "Pelvis", "Nearest"};
    static constexpr const char *priority_modes[]{"Distance", "FOV", "Crosshair"};
    static constexpr const char *pitch_modes[]{"Off", "Down", "Up", "Zero", "Fake Down"};
    static constexpr const char *yaw_modes[]{"Off", "Spin", "Jitter", "Fake"};

    if (m_hvh_inject_pending)
    {
        card_in_column("hvh_confirm", "CONFIRM INJECTION", 3, 0, [&] {
            begin_row(render::localization::tr("Confirm"), 148.0f);
            clipped_row_text(render::localization::tr("Inject vesta_hvh.dll into Counter-Strike 2?"));
            end_row();
            if (button_row(render::localization::tr("Proceed"), render::localization::tr("Inject"),
                           row_action_icon::check))
            {
                hvh.inject();
                m_hvh_inject_pending = false;
            }
            if (button_row(render::localization::tr("Abort"), render::localization::tr("Cancel")))
                m_hvh_inject_pending = false;
        });
    }

    card_in_column("hvh_status", "HVH STATUS", 4, 0, [&] {
        status_row(render::localization::tr("State"), state_text, state_color);
        status_row(render::localization::tr("Hook"),
                   hvh.hook_ready() ? render::localization::tr("Ready")
                                    : render::localization::tr("Inactive"),
                   hvh.hook_ready() ? k_text_main : k_text_muted);
        status_row(render::localization::tr("Signatures"),
                   hvh.signature_found() ? render::localization::tr("Found")
                                         : render::localization::tr("Missing"),
                   hvh.signature_found() ? k_text_main : k_text_muted);
        const auto error = hvh.last_error();
        status_row(render::localization::tr("Last Error"),
                   error.empty() ? render::localization::tr("None") : error.data(), k_text_muted);
    });

    card_in_column("hvh_injection", "INJECTION", 2, 0, [&] {
        if (button_row(render::localization::tr("Configuration"), render::localization::tr("Inject"),
                       row_action_icon::check) &&
            !hvh.dll_active())
            m_hvh_inject_pending = true;
        if (button_row(render::localization::tr("Ejection"), render::localization::tr("Eject")))
            hvh.eject();
    });

    card_in_column("hvh_rage", "RAGE", 11, 1, [&] {
        toggle_int("Enable Rage", s.enable_rage);
        select_int("Mode", s.rage_aim_mode, aim_modes);
        select_int("Hitbox", s.rage_hitbox, hitbox_modes);
        select_int("Priority", s.rage_priority, priority_modes);
        slider_int("FOV", s.rage_fov, 1, 180, " deg");
        slider_int("Smoothing", s.rage_smoothing, 1, 100);
        slider_int("Min Damage", s.rage_min_damage, 1, 100);
        toggle_int("Auto Fire", s.rage_autofire);
        toggle_int("Auto Wall", s.rage_autowall);
        toggle_int("Auto Scope", s.rage_autoscope);
        toggle_int("Silent Aim", s.rage_silent);
    });

    card_in_column("hvh_antiaim", "ANTI-AIM", 8, 1, [&] {
        toggle_int("Enable Anti-Aim", s.enable_antiaim);
        select_int("Pitch", s.aa_pitch, pitch_modes);
        select_int("Yaw Mode", s.aa_yaw_mode, yaw_modes);
        slider_int("Spin Speed", s.aa_spin_speed, 1, 180);
        slider_int("Jitter Min", s.aa_jitter_min, -180, 180);
        slider_int("Jitter Max", s.aa_jitter_max, -180, 180);
        toggle_int("Fake Lag", s.aa_fake_lag);
        toggle_int("Desync", s.aa_desync);
    });

    card_in_column("hvh_trigger", "TRIGGER", 3, 0, [&] {
        toggle_int("Enable Trigger", s.enable_trigger);
        slider_int("Delay", s.trigger_delay_ms, 0, 500, " ms");
        slider_int("Hitchance", s.trigger_hitchance, 0, 100, "%");
    });

    card_in_column("hvh_movement", "MOVEMENT", 2, 0, [&] {
        toggle_int("Bunny Hop", s.enable_bhop);
        toggle_int("Auto Stop", s.enable_auto_stop);
    });

    end_cards();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    if (changed)
    {
        hvh.publish();
        hvh.save();
    }
}
