#include "imgui.h"
#include "../ps2.h"
#include "../ps2_commands.h"
#include "shared_camera.h"
#include "shared_ui.h"
#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"
#include "glm/matrix.hpp"
#include <iostream>

class haunting_ground : public ps2_game
{
private:
	sentinel_counter sentinel;
	restoring_toggle_state<7> camera_flag;
	tweakable_value_set<float, 6> camera_values;
	multi_toggle_state speed_flag;
	float current_pitch = 0.0f;
	float current_yaw = 0.0f;
	static constexpr int camera_x = 0;
	static constexpr int camera_y = 1;
	static constexpr int camera_z = 2;
	static constexpr int camera_look_x = 3;
	static constexpr int camera_look_y = 4;
	static constexpr int camera_look_z = 5;

public:
	explicit haunting_ground(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, camera_flag(ps2, { {
			{ 0x00122620, 0x00000000, 0xe4800070 },
			{ 0x00122630, 0x00000000, 0xe4800074 },
			{ 0x00122640, 0x00000000, 0xe4800078 },
			{ 0x00122648, 0x00000000, 0x0c0436da },
			{ 0x001226CC, 0x00000000, 0xe6600064 },
			{ 0x001226BC, 0x00000000, 0xe6600060 },
			{ 0x001226DC, 0x00000000, 0xe6600068 }
		} })
		, camera_values(ps2, {
			0x01961568, // camera x
			0x01961564, // camera y
			0x01961560, // camera z
			0x01961570, // camera look x
			0x01961574, // camera look y
			0x01961578, // camera look z
			})
		, speed_flag(ps2, 3)
	{
		const uint32_t speed_address = 0x001F4D88;
		const uint32_t walk_speed_address = 0x0019C634;
		speed_flag.edit_cmd(0).write<uint32_t>(speed_address, 0x3C063F80).write<uint32_t>(walk_speed_address, 0x3C033F80).finalize();
		speed_flag.edit_cmd(1).write<uint32_t>(speed_address, 0x3C063F00).write<uint32_t>(walk_speed_address, 0x3C033F80).finalize();
		speed_flag.edit_cmd(2).write<uint32_t>(speed_address, 0x3C060000).write<uint32_t>(walk_speed_address, 0x3C060000).finalize();
	}

	void draw_game_ui(const pcsx2& ps2, const controller& c, playback& camera_playback) override
	{
		camera_playback.draw_playback_ui(c);
		shared_ui::toggle(c, controller_bindings::freecam, camera_flag, "Freecam");

		const char* speed_description = nullptr;
		switch (speed_flag.current_index())
		{
			case 0: speed_description = "1.0"; break;
			case 1: speed_description = "0.5"; break;
			case 2: speed_description = "0.0"; break;
		}
		shared_ui::button(c, controller_bindings::pause); ImGui::SameLine(); ImGui::Text("Gameplay Speed: "); ImGui::SameLine();
		ImGui::TextColored(speed_flag.current_index() > 0 ? ui_colors::warning : ui_colors::off_obvious, speed_description);
	}

	void update(const pcsx2& ps2, const controller_state& c, playback& camera_playback, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			camera_flag.reset();
			camera_values.reset();
		}

		if (c.button_down(controller_bindings::pause))
		{
			speed_flag.toggle();
			sentinel.increment();
		}

		if (c.button_down(controller_bindings::freecam))
		{
			camera_flag.toggle();
			if (camera_flag.is_on())
			{
				camera_values.start_tweaking();
				current_pitch = glm::asin(camera_values.get(camera_look_y));

				if (glm::cos(current_pitch) > 0.00001f) {
					current_yaw = std::atan2(camera_values.get(camera_look_z), camera_values.get(camera_look_x));
				}
				else {
					current_yaw = 0.0f;
				}
			}
			else
			{
				camera_values.stop_tweaking(true);
			}
			sentinel.increment();
		}
		if (camera_flag.is_on())
		{
			float turn_scale = time_delta * 5.0f;
			float move_scale = time_delta * 200.0f;

			current_yaw += c.get_right_axis().first * turn_scale;
			current_pitch -= c.get_right_axis().second * turn_scale;

			glm::vec3 pos_delta = shared_camera::compute_freecam_pos_delta(c, glm::vec2(move_scale, -move_scale), current_yaw, current_pitch);
			glm::vec3 pos = glm::vec3(camera_values.get(camera_x), camera_values.get(camera_y), camera_values.get(camera_z)) + pos_delta;

			camera_playback.update(time_delta, current_yaw, current_pitch, pos.x, pos.y, pos.z);

			glm::vec3 forward;
			forward.x = cos(current_pitch) * cos(current_yaw);
			forward.y = sin(current_pitch);
			forward.z = cos(current_pitch) * sin(current_yaw);

			camera_values.set(camera_look_x, forward);
			camera_values.set(camera_x, pos);

			camera_values.flush(ps2);
		}
	}
};

ps2_game_static_register<haunting_ground> r("901AAC09", "Haunting Ground (USA)");