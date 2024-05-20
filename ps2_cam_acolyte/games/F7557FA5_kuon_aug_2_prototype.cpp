#include "imgui.h"
#include "../ps2.h"
#include "../ps2_commands.h"
#include "shared_utils.h"
#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"
#include <iostream>

class kuon_prototype : public ps2_game
{
private:
	sentinel_counter sentinel;
	toggle_state camera_flag;
	tweakable_value_set<float, 5> camera_values;
	toggle_state pause_flag;

	enum class freecam_mode_type
	{
		none,
		camera
	};
	freecam_mode_type freecam_mode = freecam_mode_type::none;

	static constexpr int camera_pitch = 0;
	static constexpr int camera_yaw = 1;
	static constexpr int camera_x = 2;
	static constexpr int camera_y = 3;
	static constexpr int camera_z = 4;

public:
	explicit kuon_prototype(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, camera_flag(ps2)
		, camera_values(ps2, {
			0x00325280, // camera pitch
			0x00325284, // camera yaw
			0x00325270, // camera x
			0x00325274, // camera y
			0x00325278, // camera z
			})
		, pause_flag(ps2)
	{
		camera_flag.edit_off()
			.write<int32_t>(0x00133F08, 0x0C04CC40) // 	jal	UpdateCamAnim
			.finalize();
		camera_flag.edit_on()
			.write<int32_t>(0x00133F08, 0x00000000) // jal UpdateCamAnim -> nop
			.finalize();
		pause_flag.edit_off()
			.write<int32_t>(0x00138FD8, 0x0C05E988) // 	jal	PlayDrama
			.write<int32_t>(0x00213148, 0x24020001) // 	Menu_TD_IsPlay 	li	v0,0x1
			.finalize();
		pause_flag.edit_on()
			.write<int32_t>(0x00138FD8, 0x00000000) // jal PlayDrama -> nop
			.write<int32_t>(0x00213148, 0x8F820004) // 	Menu_TD_IsPlay 	li	v0,0x1 -> lw v0,0x4(gp)
			.finalize();
	}

	void draw_game_ui() override
	{
		ImGui::Text("Freecam (A): "); ImGui::SameLine();
		ImGui::TextColored(freecam_mode == freecam_mode_type::camera ? ui_colors::on_obvious : ui_colors::off_obvious, freecam_mode == freecam_mode_type::camera ? "ON" : "OFF");
		ImGui::Text("Pause (X): "); ImGui::SameLine();
		ImGui::TextColored(pause_flag.is_on() ? ui_colors::on_obvious : ui_colors::off_obvious, pause_flag.is_on() ? "ON" : "OFF");
	}

	void update(const pcsx2& ps2, const controller_state& c, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			pause_flag.reset();
			camera_flag.reset();
			camera_values.reset();
			freecam_mode = freecam_mode_type::none;
		}

		if (c.button_down(button_type::BUTTON_X))
		{
			pause_flag.toggle();

			sentinel.increment();
		}

		if (c.button_down(button_type::BUTTON_A))
		{
			if (freecam_mode == freecam_mode_type::camera) freecam_mode = freecam_mode_type::none;
			else freecam_mode = freecam_mode_type::camera;

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
					camera_values.stop_tweaking(true);
				}
			}

			sentinel.increment();
		}

		if (camera_values.currently_tweaking())
		{
			float turn_scale = time_delta * 2.0f;
			float move_scale = time_delta * 50.0f;

			camera_values.add(camera_yaw, c.get_right_axis().first * turn_scale);
			camera_values.add(camera_pitch, -c.get_right_axis().second * turn_scale);

			const float current_yaw = camera_values.get(camera_yaw);
			const float current_pitch = -camera_values.get(camera_pitch);

			glm::vec3 pos_delta = shared_utils::compute_freecam_pos_delta(c, glm::vec2(move_scale, -move_scale), current_yaw, current_pitch);

			camera_values.add(camera_x, pos_delta.x);
			camera_values.add(camera_y, pos_delta.y);
			camera_values.add(camera_z, pos_delta.z);

			camera_values.flush(ps2);
		}
	}
};

ps2_game_static_register<kuon_prototype> r("F7557FA5", "Kuon (Aug 2, 2004 prototype)");