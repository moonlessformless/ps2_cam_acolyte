#include "shared_silent_hill_2.h"
#include "imgui.h"
#include <iostream>

class silent_hill_prototype : public ps2_game
{
private:
	sentinel_counter sentinel;
	toggle_state camera_flag;
	multi_toggle_state speed_flag;
	restoring_toggle_state<7> brightness_flag;
	tweakable_value_set<float, 5> camera_values;
	restoring_toggle_state<3> moving_james_flag;
	tweakable_value_set<float, 3> james_position_values;
	restoring_toggle_state<6> cutscene_values;

	enum class freecam_mode_type
	{
		none,
		camera,
		james
	};
	freecam_mode_type freecam_mode = freecam_mode_type::none;

public:
	explicit silent_hill_prototype(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, camera_flag(ps2)
		, speed_flag(ps2, 3)
		, camera_values(ps2, {
			0x10E5C00, // camera pitch
			0x10E5C04, // camera yaw
			0x10E5B40, // camera x
			0x10E5B44, // camera y
			0x10E5B48, // camera z
		})
		, brightness_flag(ps2,
		{{
			{ 0x00175DD4, 0x00000000, 0xE42CDAC8 }, // set fog min -> nop
			{ 0x00175DE4, 0x00000000, 0xE42CDACC }, // set fog max -> nop
			{ 0x002adac8, 0xFFFFFFFF, 0xFFFFFFFF }, // fog min distance -> max
			{ 0x002adacc, 0xFFFFFFFF, 0xFFFFFFFF }, // fog max distance -> max
			{ 0x0017D48C, 0x3C023F80, 0x3C023F80 }, // change the light sceVu0ScaleVector to a sceVu0ClampVector
			{ 0x0017D4A0, 0x0C082280, 0x0C082196 },
			{ 0x0017D4A4, 0x46006346, 0x00000000 }
		}})
		, moving_james_flag(ps2,
		{{
			{ 0x0010F044, 0x00000000, 0x0C0474F0 }, // disable scAddPos
			{ 0x0010F030, 0x00000000, 0x0C0454F4 }, // disable PlayerUpdatePosition2D
			{ 0x0012E788, 0x00000000, 0x0C04C874 }  // disable clCollectCharaPosition
		}})
		, james_position_values(ps2, {
			0x003C84F0, // james x
			0x003C84F4, // james y
			0x003C84F8, // james z
		})
		, cutscene_values(ps2,
		{{
			// COMING SOON patch out in EvProgClockTime
			{ 0x01F024A0, 0x00000000, 0x24443E40 },
			{ 0x01F024A8, 0x00000000, 0x0C08D9AC },
			// disable drawing of fade out - Make_Filter_Packet call
			{ 0x0017B0A8, 0x34020000, 0x70001628 },
			{ 0x00179790, 0x34150000, 0x00000000 },
			// ScreenEffectFade disable - may be dangerous
			{ 0x0020DFC0, 0x00000000, 0x27BDFFF0 },
			{ 0x0020DFC4, 0x03E00008, 0x7FBF0000 }
		}})
	{
		camera_flag.edit_off()
			.write<int32_t>(0x00196164, 0x0C067210) // jal vcRenewalCamData
			.write<int32_t>(0x0019CB44, 0x0C067510) // jal vcAdjCamOfsAngByOfsAngSpd
			.finalize();
		camera_flag.edit_on()
			.write<int32_t>(0x00196164, 0x00000000) // jal vcRenewalCamData -> nop
			.write<int32_t>(0x0019CB44, 0x00000000) // jal vcAdjCamOfsAngByOfsAngSpd -> nop
			.finalize();

		const uint32_t set_delta_command = 0x0020A97C; // command that sets the delta
		const uint32_t anim_speed_flag = 0x011B6B28;
		const uint32_t sim_speed_flag = 0x011B6B1B;
		speed_flag.edit_cmd(0).write<uint32_t>(set_delta_command, 0x0C08805C).write<uint8_t>(anim_speed_flag, 0x02).write<uint8_t>(sim_speed_flag, 0x3D).finalize();
		speed_flag.edit_cmd(1).write<uint32_t>(set_delta_command, 0x00000000).write<uint8_t>(anim_speed_flag, 0x01).write<uint8_t>(sim_speed_flag, 0x3C).finalize();
		speed_flag.edit_cmd(2).write<uint32_t>(set_delta_command, 0x00000000).write<uint8_t>(anim_speed_flag, 0x00).write<uint8_t>(sim_speed_flag, 0x30).finalize();

		ps2_ipc_cmd cmd(ps2);

		// disable noise
		cmd.write<uint32_t>(0x00178564, 0x1060FF9C);

		cmd.send();
	}

	void draw_game_ui() override
	{
		const char* speed_description = nullptr;
		switch (speed_flag.current_index())
		{
			case 0: speed_description = "1.0"; break;
			case 1: speed_description = "0.5"; break;
			case 2: speed_description = "0.0"; break;
		}
		ImGui::PushTextWrapPos(0.0f); ImGui::TextColored(ui_colors::help, "Note: the freecam behaves oddly during cutscenes but can still be moved."); ImGui::PopTextWrapPos();
		ImGui::Text("Freecam (A): "); ImGui::SameLine();
		ImGui::TextColored(freecam_mode == freecam_mode_type::camera ? ui_colors::on_obvious : ui_colors::off_obvious, freecam_mode == freecam_mode_type::camera ? "ON" : "OFF");
		ImGui::Text("Speed (X): "); ImGui::SameLine();
		ImGui::TextColored(speed_flag.current_index() > 0 ? ui_colors::warning : ui_colors::off_obvious, speed_description);
		ImGui::Text("Move James (B): "); ImGui::SameLine();
		ImGui::TextColored(freecam_mode == freecam_mode_type::james ? ui_colors::on_obvious : ui_colors::off_obvious, freecam_mode == freecam_mode_type::james ? "ON" : "OFF");
		ImGui::Text("Bright Mode/No Fog (Y): "); ImGui::SameLine();
		ImGui::TextColored(brightness_flag.is_on() ? ui_colors::on_obvious : ui_colors::off_obvious, brightness_flag.is_on() ? "ON (flashlight must be off)" : "OFF");
		ImGui::NewLine();
		ImGui::PushTextWrapPos(0.0f); ImGui::TextColored(ui_colors::help, "This disables fading out and cutscenes, but may cause issues. Use before entering the apartment fight with Pyramid Head to avoid the COMING SOON blocker."); ImGui::PopTextWrapPos();
		if (ImGui::Button(cutscene_values.is_on() ? "Enable Cutscenes" : "Disable Cutscenes"))
		{
			cutscene_values.toggle();
		}

		if (freecam_mode == freecam_mode_type::camera)
		{
			ImGui::Text("Camera: %.2f %.2f %.2f", camera_values.get(2), camera_values.get(3), camera_values.get(4));
		}
		else if (freecam_mode == freecam_mode_type::james)
		{
			ImGui::Text("James: %.2f %.2f %.2f", james_position_values.get(0), james_position_values.get(1), james_position_values.get(2));
		}
	}

	void sync_freecam_mode(const pcsx2& ps2)
	{
		if (freecam_mode != freecam_mode_type::none)
		{
			camera_flag.set_on(true);
			if (!camera_values.currently_tweaking())
			{
				camera_values.start_tweaking();
			}
		}
		else
		{
			camera_flag.set_on(false);
			if (camera_values.currently_tweaking())
			{
				camera_values.stop_tweaking(moving_james_flag.is_on());
			}
		}

		if (freecam_mode == freecam_mode_type::james)
		{
			moving_james_flag.set_on(true);
			if (!james_position_values.currently_tweaking())
			{
				james_position_values.start_tweaking();
			}
		}
		else
		{
			moving_james_flag.set_on(false);
			if (james_position_values.currently_tweaking())
			{
				james_position_values.stop_tweaking(false);
			}
		}

		sentinel.increment();
	}

	void update(const pcsx2& ps2, const controller_state& c, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			speed_flag.reset();
			camera_flag.reset();
			brightness_flag.reset();
			camera_values.reset();
			moving_james_flag.reset();
			james_position_values.reset();
			cutscene_values.reset();
			freecam_mode = freecam_mode_type::none;
		}

		if (c.button_down(button_type::BUTTON_X))
		{
			speed_flag.toggle();
			sentinel.increment();
		}
		if (c.button_down(button_type::BUTTON_B))
		{
			if (freecam_mode == freecam_mode_type::james) freecam_mode = freecam_mode_type::none;
			else freecam_mode = freecam_mode_type::james;
			sync_freecam_mode(ps2);
		}
		if (c.button_down(button_type::BUTTON_A))
		{
			if (freecam_mode == freecam_mode_type::camera) freecam_mode = freecam_mode_type::none;
			else freecam_mode = freecam_mode_type::camera;
			sync_freecam_mode(ps2);
		}
		if (c.button_down(button_type::BUTTON_Y))
		{
			brightness_flag.toggle();
			sentinel.increment();
		}

		if (camera_values.currently_tweaking())
		{
			shared_silent_hill_2::process_sh2_freecam(ps2, c, time_delta, camera_values, james_position_values);
		}
	}
};

ps2_game_static_register<silent_hill_prototype> r("D915592E", "Silent Hill 2 (Jul 13, 2001 prototype)");