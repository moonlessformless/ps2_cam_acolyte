#include "shared_silent_hill_2.h"
#include "shared_ui.h"
#include <iostream>

class silent_hill_2_directors_cut : public ps2_game
{
private:
	sentinel_counter sentinel;
	restoring_toggle_state<3> camera_flag;
	tweakable_value_set<float, 5> camera_values;
	restoring_toggle_state<3> moving_james_flag;
	tweakable_value_set<float, 3> james_position_values;
	bool should_restore_camera_on_finish = true;

	enum class freecam_mode_type
	{
		none,
		camera,
		james
	};
	freecam_mode_type freecam_mode = freecam_mode_type::none;

public:
	explicit silent_hill_2_directors_cut(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, camera_flag(ps2, {{
			{ 0x0019D208, 0x00000000, 0x0C068E3C }, // jal vcRenewalCamData -> nop
			{ 0x001A3C08, 0x00000000, 0x0C069124 }, // jal vcAdjCamOfsAngByOfsAngSpd -> nop
			{ 0x001A3C34, 0x00000000, 0x0C09102C } // unknown restoring -> nop
		}})
		, camera_values(ps2, {
			0x10F7A00, // camera pitch
			0x10F7A04, // camera yaw
			0x10F7940, // camera x
			0x10F7944, // camera y
			0x10F7948, // camera z
			})
		, moving_james_flag(ps2,
			{{
				{ 0x0011198C, 0x00000000, 0x0C047AA4 }, // disable scAddPos
				{ 0x00130B98, 0x00000000, 0x0C04D340 }, // disable PlayerUpdatePosition2D
				{ 0x0012E788, 0x00000000, 0x3C022000 }  // disable clCollectCharaPosition
			}})
		, james_position_values(ps2, {
			0x00400830, // james x
			0x00400834, // james y
			0x00400838, // james z
			})
	{
		ps2_ipc_cmd cmd(ps2);

		// disable noise
		cmd.write<uint32_t>(0x0017E994, 0x1060FF9C);

		cmd.send();
	}

	void sync_freecam_mode(const pcsx2& ps2)
	{
		if (freecam_mode != freecam_mode_type::none)
		{
			camera_flag.set_on(true);
			camera_values.start_tweaking();
		}
		else
		{
			camera_flag.set_on(false);
			camera_values.stop_tweaking(moving_james_flag.is_on());
		}

		if (freecam_mode == freecam_mode_type::james)
		{
			moving_james_flag.set_on(true);
			james_position_values.start_tweaking();
		}
		else
		{
			moving_james_flag.set_on(false);
			james_position_values.stop_tweaking(false);
		}

		sentinel.increment();
	}

	void draw_game_ui(const pcsx2& ps2, const controller& c, playback& camera_playback) override
	{
		camera_playback.draw_playback_ui(c);
		ImGui::PushTextWrapPos(0.0f); ImGui::TextColored(ui_colors::help, "Note: the freecam behaves oddly during cutscenes but can still be moved."); ImGui::PopTextWrapPos();
		shared_ui::button(c, controller_bindings::freecam); ImGui::SameLine(); ImGui::Text("Freecam: "); ImGui::SameLine();
		ImGui::TextColored(freecam_mode == freecam_mode_type::camera ? ui_colors::on_obvious : ui_colors::off_obvious, freecam_mode == freecam_mode_type::camera ? "ON" : "OFF");
		shared_ui::button(c, controller_bindings::special); ImGui::SameLine(); ImGui::Text("Move James: "); ImGui::SameLine();
		ImGui::TextColored(freecam_mode == freecam_mode_type::james ? ui_colors::on_obvious : ui_colors::off_obvious, freecam_mode == freecam_mode_type::james ? "ON" : "OFF");
		ImGui::NewLine();

		if (freecam_mode == freecam_mode_type::camera)
		{
			ImGui::Text("Camera: %.2f %.2f %.2f", camera_values.get(2), camera_values.get(3), camera_values.get(4));
		}
		else if (freecam_mode == freecam_mode_type::james)
		{
			ImGui::Text("James: %.2f %.2f %.2f", james_position_values.get(0), james_position_values.get(1), james_position_values.get(2));
		}
	}

	void update(const pcsx2& ps2, const controller_state& c, playback& camera_playback, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			camera_flag.reset();
			camera_values.reset();
			moving_james_flag.reset();
			james_position_values.reset();
			freecam_mode = freecam_mode_type::none;
		}

		if (c.button_down(controller_bindings::special))
		{
			if (freecam_mode == freecam_mode_type::james) freecam_mode = freecam_mode_type::none;
			else freecam_mode = freecam_mode_type::james;
			sync_freecam_mode(ps2);
		}
		if (c.button_down(controller_bindings::freecam))
		{
			if (freecam_mode == freecam_mode_type::camera) freecam_mode = freecam_mode_type::none;
			else freecam_mode = freecam_mode_type::camera;
			sync_freecam_mode(ps2);
		}

		if (camera_values.currently_tweaking())
		{
			shared_silent_hill_2::process_sh2_freecam(ps2, c, camera_playback, time_delta, camera_values, james_position_values);
		}
	}
};

ps2_game_static_register<silent_hill_2_directors_cut> r("6BBD4932", "Silent Hill 2 Director's Cut (EU)");