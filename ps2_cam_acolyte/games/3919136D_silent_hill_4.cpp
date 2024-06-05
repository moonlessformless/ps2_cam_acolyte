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

class silent_hill_4 : public ps2_game
{
private:
	sentinel_counter sentinel;
	restoring_toggle_state<4> camera_flag;
	restoring_toggle_state<2> pause_flag;
	tweakable_value_set<float, 16> camera_matrix;
	float current_yaw = 0.0f;
	float current_pitch = 0.0f;

	// 0021FA28 - no bars/noise filter

public:
	explicit silent_hill_4(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, camera_flag(ps2, { {
			{ 0x00148DB0, 0x00000000, 0x7C860000 },
			{ 0x00148DB4, 0x00000000, 0x7C870010 },
			{ 0x00148DB8, 0x00000000, 0x7C880020 },
			{ 0x00148DBC, 0x00000000, 0x7C890030 },
		} })
		, camera_matrix(ps2, 0x0045A170)
		, pause_flag(ps2, { {
			{ 0x0015D790, 0x03E00008, 0x27BDFFF0 }, // jr ra at start of the "cinematic update" subsystem
			{ 0x0015D740, 0x03E00008, 0x27BDFFF0 } // jr ra at start of the "gameplay" subsystem
		} })
	{
	}

	void draw_game_ui(const pcsx2& ps2, const controller& c, playback& camera_playback) override
	{
		camera_playback.draw_playback_ui(c);
		shared_ui::toggle(c, controller_bindings::freecam, camera_flag, "Freecam");
		shared_ui::toggle(c, controller_bindings::pause, pause_flag, "Pause");
	}

	void update(const pcsx2& ps2, const controller_state& c, playback& camera_playback, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			camera_flag.reset();
			camera_matrix.reset();
		}

		if (c.button_down(controller_bindings::pause))
		{
			pause_flag.toggle();

			sentinel.increment();
		}

		if (c.button_down(controller_bindings::freecam))
		{
			if (!camera_flag.is_on())
			{
				camera_flag.set_on(true);
				camera_matrix.start_tweaking();
				current_yaw = glm::acos(camera_matrix.get(0));
				current_pitch = -glm::acos(camera_matrix.get(5));
			}
			else
			{
				camera_flag.set_on(false);
				camera_matrix.stop_tweaking(true);
			}

			sentinel.increment();
		}

		if (camera_flag.is_on())
		{
			float turn_scale = time_delta * 3.0f;
			float move_scale = time_delta * 1000.0f;

			current_yaw += c.get_right_axis().first * turn_scale;
			current_pitch -= c.get_right_axis().second * turn_scale;

			glm::vec3 pos_delta = shared_camera::compute_freecam_pos_delta(c, glm::vec2(move_scale, -move_scale), current_yaw, -current_pitch);
			glm::vec3 pos = glm::vec3(camera_matrix.get(12), camera_matrix.get(13), camera_matrix.get(14)) + pos_delta;

			camera_playback.update(time_delta, current_yaw, current_pitch, pos.x, pos.y, pos.z);

			glm::mat4 position_mat(1.0f);
			position_mat[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);

			glm::mat4 final_mat = position_mat * shared_camera::compute_rotation_matrix_y_x(current_yaw, current_pitch);

			camera_matrix.set(0, final_mat);
			camera_matrix.flush(ps2);
		}
	}
};

ps2_game_static_register<silent_hill_4> r("3919136D", "Silent Hill 4 - The Room (USA)");