#pragma once

#include "../controller.h"
#include "glm/trigonometric.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"
#include "glm/matrix.hpp"

class shared_camera
{
public:
	static glm::vec3 compute_freecam_pos_delta(const controller_state& c, glm::vec2 move_joystick_scale,
		float yaw, float pitch,
		glm::vec3* optional_out_forward = nullptr)
	{
		glm::vec3 forward(
			glm::cos(yaw) * glm::cos(pitch),
			glm::sin(pitch),
			glm::sin(yaw) * glm::cos(pitch)
		);

		glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));

		const float x_delta = c.get_left_axis().first * move_joystick_scale.x;
		const float y_delta = c.get_left_axis().second * move_joystick_scale.y;

		if (optional_out_forward)
		{
			*optional_out_forward = forward;
		}

		return glm::vec3(
			(x_delta * right.z) + (y_delta * forward.z),
			(x_delta * right.y) + (y_delta * forward.y),
			(x_delta * right.x) + (y_delta * forward.x)
		);
	}

	static glm::mat4 compute_rotation_matrix_z_x(float current_yaw, float current_pitch)
	{
		glm::mat4 yaw_mat({
			glm::cos(current_yaw), -glm::sin(current_yaw), 0.0f, 0.0f,
			glm::sin(current_yaw), glm::cos(current_yaw), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			});

		glm::mat4 pitch_mat({
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, glm::cos(current_pitch), glm::sin(current_pitch), 0.0f,
			0.0f, -glm::sin(current_pitch), glm::cos(current_pitch), 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			});

		return yaw_mat * pitch_mat;
	}

	static glm::mat4 compute_rotation_matrix_y_x(float current_yaw, float current_pitch)
	{
		glm::mat4 yaw_mat({
			glm::cos(current_yaw), 0.0f, -glm::sin(current_yaw), 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			glm::sin(current_yaw), 0.0f, glm::cos(current_yaw), 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			});

		glm::mat4 pitch_mat({
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, glm::cos(current_pitch), glm::sin(current_pitch), 0.0f,
			0.0f, -glm::sin(current_pitch), glm::cos(current_pitch), 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			});

		return yaw_mat * pitch_mat;
	}
};