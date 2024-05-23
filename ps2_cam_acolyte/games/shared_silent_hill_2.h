#pragma once

#include "../ps2.h"
#include "../ps2_commands.h"
#include "shared_camera.h"
#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"

class shared_silent_hill_2
{
public:
	static constexpr int camera_pitch = 0;
	static constexpr int camera_yaw = 1;
	static constexpr int camera_x = 2;
	static constexpr int camera_y = 3;
	static constexpr int camera_z = 4;

	static void process_sh2_freecam(const pcsx2& ps2, const controller_state& c,
		float time_delta,
		tweakable_value_set<float, 5>& camera_values,
		tweakable_value_set<float, 3>& james_position_values)
	{
		float turn_scale = time_delta * 3.0f;
		float move_scale = time_delta * 4000.0f;

		camera_values.add(camera_yaw, c.get_right_axis().first * turn_scale);
		camera_values.add(camera_pitch, -c.get_right_axis().second * turn_scale);

		glm::vec3 forward;
		glm::vec3 pos_delta = shared_camera::compute_freecam_pos_delta(c, glm::vec2(move_scale, -move_scale), camera_values.get(camera_yaw), camera_values.get(camera_pitch), &forward);

		camera_values.add(camera_x, pos_delta.x);
		camera_values.add(camera_y, -pos_delta.y);
		camera_values.add(camera_z, pos_delta.z);

		if (james_position_values.currently_tweaking())
		{
			const float distance = 1500.0f;
			james_position_values.set(0, camera_values.get(camera_x) + forward.z * distance);
			james_position_values.set(1, camera_values.get(camera_y) + forward.y * distance + 500.0f);
			james_position_values.set(2, camera_values.get(camera_z) + forward.x * distance);
			james_position_values.flush(ps2);
		}

		camera_values.flush(ps2);
	}
};