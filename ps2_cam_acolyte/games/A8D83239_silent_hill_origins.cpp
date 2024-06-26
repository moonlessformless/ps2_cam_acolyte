﻿#include "imgui.h"
#include "../ps2.h"
#include "../ps2_commands.h"
#include "shared_camera.h"
#include "shared_ui.h"
#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"
#include "glm/matrix.hpp"
#include <iostream>

class silent_hill_origins : public ps2_game
{
private:
	sentinel_counter sentinel;
	toggle_state camera_flag;
	toggle_state brightness_flag;
	tweakable_value_set<char, 1> pause_toggle; // this is a tweakable because we need to continually flush the value
	tweakable_value_set<float, 16> camera_matrix_1;
	tweakable_value_set<float, 16> camera_matrix_2;
	float current_yaw = 0.0f;
	float current_pitch = 0.0f;

public:
	explicit silent_hill_origins(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, camera_flag(ps2)
		, camera_matrix_1(ps2, 0x0091B5F0)
		, camera_matrix_2(ps2, 0x016E3BD0)
		, brightness_flag(ps2)
		, pause_toggle(ps2, 0x00339CEC)
	{
		camera_flag.edit_off()
			.write<int32_t>(0x001133DC, 0x0C0899E8) // jal	pos_004195D0
			.finalize();
		camera_flag.edit_on()
			.write<int32_t>(0x001133DC, 0x00000000) // jal	pos_004195D0 -> nop
			.finalize();

		brightness_flag.edit_off()
			.write<int32_t>(0x00929684, 0x41B00000)
			.write<int32_t>(0x00929688, 0x3F800000)
			.write<int32_t>(0x00929690, 0xC414A420)
			.finalize();
		brightness_flag.edit_on()
			.write<int32_t>(0x00929684, 0x43FA0000)
			.write<int32_t>(0x00929688, 0x43F78000)
			.write<int32_t>(0x00929690, 0x00000000)
			.finalize();
	}

	void draw_game_ui(const pcsx2& ps2, const controller& c, playback& camera_playback) override
	{
		camera_playback.draw_playback_ui(c);
		shared_ui::toggle(c, controller_bindings::freecam, camera_flag, "Freecam");
		shared_ui::toggle(c, controller_bindings::lighting, brightness_flag, "Brightness Boost");
		shared_ui::toggle(c, controller_bindings::pause, pause_toggle, "Pause");
	}

	void update(const pcsx2& ps2, const controller_state& c, playback& camera_playback, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			camera_flag.reset();
			camera_matrix_1.reset();
			camera_matrix_2.reset();
			brightness_flag.reset();
			pause_toggle.reset();
		}

		if (c.button_down(controller_bindings::lighting))
		{
			brightness_flag.toggle();
			sentinel.increment();
		}

		if (c.button_down(controller_bindings::pause))
		{
			if (!pause_toggle.currently_tweaking())
			{
				pause_toggle.start_tweaking();
			}
			else
			{
				pause_toggle.set(0, 0x00);
				pause_toggle.flush(ps2);
				pause_toggle.stop_tweaking();
			}
			sentinel.increment();
		}

		if (c.button_down(controller_bindings::freecam))
		{
			if (!camera_flag.is_on())
			{
				camera_flag.set_on(true);
				camera_matrix_1.start_tweaking();
				camera_matrix_2.start_tweaking();
				current_yaw = -glm::acos(camera_matrix_1.get(0));
				current_pitch = glm::acos(camera_matrix_1.get(5));
			}
			else
			{
				camera_flag.set_on(false);
				camera_matrix_1.stop_tweaking(true);
				camera_matrix_2.stop_tweaking(true);
			}

			sentinel.increment();
		}

		// end of cinematics, etc. can reset this flag, so we keep writing it
		if (pause_toggle.currently_tweaking())
		{
			pause_toggle.set(0, 0x01);
			pause_toggle.flush(ps2);
		}

		if (camera_flag.is_on())
		{
			float turn_scale = time_delta * 3.0f;
			float move_scale = time_delta * 10.0f;

			current_yaw += -c.get_right_axis().first * turn_scale;
			current_pitch += c.get_right_axis().second * turn_scale;

			glm::vec3 pos_delta = shared_camera::compute_freecam_pos_delta(c, glm::vec2(-move_scale, -move_scale), current_yaw, -current_pitch);
			glm::vec3 pos = glm::vec3(camera_matrix_1.get(12), camera_matrix_1.get(13), camera_matrix_1.get(14)) + pos_delta;

			camera_playback.update(time_delta, current_yaw, current_pitch, pos.x, pos.y, pos.z);

			glm::mat4 position_mat(1.0f);
			position_mat[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);

			glm::mat4 final_mat =  position_mat * shared_camera::compute_rotation_matrix_y_x(current_yaw, current_pitch);

			camera_matrix_1.set(0, final_mat);
			camera_matrix_2.set(0, final_mat);

			camera_matrix_1.flush(ps2);
			camera_matrix_2.flush(ps2);
		}
	}
};

ps2_game_static_register<silent_hill_origins> r("A8D83239", "Silent Hill Origins (USA)");