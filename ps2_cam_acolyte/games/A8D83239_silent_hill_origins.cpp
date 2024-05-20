#include "imgui.h"
#include "../ps2.h"
#include "../ps2_commands.h"
#include "shared_utils.h"
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
	tweakable_value_set<float, 16> camera_matrix_1;
	tweakable_value_set<float, 16> camera_matrix_2;
	float current_yaw = 0.0f;
	float current_pitch = 0.0f;

	enum class freecam_mode_type
	{
		none,
		camera
	};
	freecam_mode_type freecam_mode = freecam_mode_type::none;

public:
	explicit silent_hill_origins(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, camera_flag(ps2)
		, camera_matrix_1(ps2, 0x0091B5F0)
		, camera_matrix_2(ps2, 0x016E3BD0)
		, brightness_flag(ps2)
	{
		camera_flag.edit_off()
			.write<int32_t>(0x001133DC, 0x0C106574) // jal	pos_004195D0
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

	void draw_game_ui() override
	{
		ImGui::Text("Freecam (A): "); ImGui::SameLine();
		ImGui::TextColored(freecam_mode == freecam_mode_type::camera ? ui_colors::on_obvious : ui_colors::off_obvious, freecam_mode == freecam_mode_type::camera ? "ON" : "OFF");
		ImGui::Text("Brightness Boost (X): "); ImGui::SameLine();
		ImGui::TextColored(brightness_flag.is_on() ? ui_colors::on_obvious : ui_colors::off_obvious, brightness_flag.is_on() ? "ON" : "OFF");
	}

	void update(const pcsx2& ps2, const controller_state& c, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			camera_matrix_1.reset();
			camera_matrix_2.reset();
			freecam_mode = freecam_mode_type::none;
		}

		if (c.button_down(button_type::BUTTON_X))
		{
			brightness_flag.toggle();
			sentinel.increment();
		}

		if (c.button_down(button_type::BUTTON_A))
		{
			if (freecam_mode == freecam_mode_type::camera) freecam_mode = freecam_mode_type::none;
			else freecam_mode = freecam_mode_type::camera;

			if (freecam_mode != freecam_mode_type::none)
			{
				camera_flag.set_on(true);
				if (!camera_matrix_1.currently_tweaking())
				{
					camera_matrix_1.start_tweaking();
				}
				if (!camera_matrix_2.currently_tweaking())
				{
					camera_matrix_2.start_tweaking();
				}
				current_yaw = -glm::acos(camera_matrix_1.get(0));
				current_pitch = glm::acos(camera_matrix_1.get(5));
			}
			else
			{
				camera_flag.set_on(false);
				if (camera_matrix_1.currently_tweaking())
				{
					camera_matrix_1.stop_tweaking(true);
				}
				if (camera_matrix_2.currently_tweaking())
				{
					camera_matrix_2.stop_tweaking(true);
				}
			}

			sentinel.increment();
		}

		if (freecam_mode == freecam_mode_type::camera)
		{
			float turn_scale = time_delta * 3.0f;
			float move_scale = time_delta * 10.0f;

			current_yaw += -c.get_right_axis().first * turn_scale;
			current_pitch += c.get_right_axis().second * turn_scale;

			glm::mat4 pitch_mat({
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, glm::cos(current_pitch), glm::sin(current_pitch), 0.0f,
				0.0f, -glm::sin(current_pitch), glm::cos(current_pitch), 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
				});

			glm::mat4 yaw_mat({
				glm::cos(current_yaw), 0.0f, -glm::sin(current_yaw), 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				glm::sin(current_yaw), 0.0f, glm::cos(current_yaw), 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
				});

			glm::vec3 pos_delta = shared_utils::compute_freecam_pos_delta(c, glm::vec2(-move_scale, -move_scale), current_yaw, current_pitch);

			glm::mat4 position_mat(1.0f);
			position_mat[3] = glm::vec4(camera_matrix_1.get(12) + pos_delta.x, camera_matrix_1.get(13) - pos_delta.y, camera_matrix_1.get(14) + pos_delta.z, 1.0f);

			glm::mat4 final_mat =  position_mat * yaw_mat * pitch_mat;

			int i = 0;
			for (int y = 0; y < 4; ++y)
			{
				for (int x = 0; x < 4; ++x)
				{
					camera_matrix_1.set(i, final_mat[y][x]);
					camera_matrix_2.set(i, final_mat[y][x]);
					++i;
				}
			}

			camera_matrix_1.flush(ps2);
			camera_matrix_2.flush(ps2);
		}
	}
};

ps2_game_static_register<silent_hill_origins> r("A8D83239", "Silent Hill Origins");