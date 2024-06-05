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

class siren : public ps2_game
{
private:
	sentinel_counter sentinel;
	tweakable_value_set<float, 1> brightness_flag;
	tweakable_value_set<char, 1> freeze_ai_flag;
	toggle_state camera_flag;
	read_only_value_set<uint32_t, 1> source_camera_location;
	read_only_value_set<float, 16> source_camera_matrix;
	tweakable_value_set<float, 16> final_camera_matrix;
	float current_yaw = 0.0f;
	float current_pitch = 0.0f;
	glm::vec3 current_position = glm::vec3();

	// the source/final camera addresses seem to change bet
	void update_camera_base_address()
	{
		source_camera_location.update();
		size_t source_base = source_camera_location.get(0);
		if (source_camera_matrix.get_address(0) != source_base)
		{
			source_camera_matrix.update_base_address(source_base);
			final_camera_matrix.update_base_address(source_base + 112);
		}
	}

public:
	explicit siren(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, brightness_flag(ps2, 0x002CD764)
		, freeze_ai_flag(ps2, 0x1A97DF1)
		, camera_flag(ps2)
		, source_camera_location(ps2, 0x01000004) // we assemble opcodes to write the source camera base address here when the camera flag is turned on
		, source_camera_matrix(ps2, 0x00C24E70)
		, final_camera_matrix(ps2, 0x00C24EE0)
	{
		// this stops writes to the final camera matrix - we recompute it ourselves starting from the source_camera_matrix
		camera_flag.edit_off()
			.write<int32_t>(0x001068F8, 0x7C880000) // sq t0, 0x0(a0)
			.write<int32_t>(0x001068FC, 0x7C890010) // sq t1, 0x10(a0)
			.write<int32_t>(0x00106900, 0x7C8A0020) // sq t2, 0x20(a0)
			.write<int32_t>(0x00106908, 0xF8840030) // sqc2 vf04, 0x30(a0)
			.finalize();
		camera_flag.edit_on()
			.write<int32_t>(0x001068F8, 0x3C080100) // sq t0, 0x0(a0) -> lui t0, 0x0100
			.write<int32_t>(0x001068FC, 0x35080004) // sq t1, 0x10(a0) -> ori t0, t0, 0x0004
			.write<int32_t>(0x00106900, 0xAD050000) // sq t2, 0x20(a0) -> sw a1, 0(t0) - writes the base address of the source camera to our secret location 0x01000004
			.write<int32_t>(0x00106908, 0x00000000) // sqc2 vf04, 0x30(a0)
			.finalize();
	}

	void draw_game_ui(const pcsx2& ps2, const controller& c, playback& camera_playback) override
	{
		camera_playback.draw_playback_ui(c);
		shared_ui::toggle(c, controller_bindings::freecam, camera_flag, "Freecam");
		shared_ui::toggle(c, controller_bindings::lighting, brightness_flag, "Bright Mode");
		shared_ui::toggle(c, controller_bindings::special, freeze_ai_flag, "Freeze AI");
	}

	void update(const pcsx2& ps2, const controller_state& c, playback& camera_playback, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			camera_flag.reset();
			final_camera_matrix.reset();
			brightness_flag.reset();
			freeze_ai_flag.reset();
		}

		if (c.button_down(controller_bindings::lighting))
		{
			if (!brightness_flag.currently_tweaking())
			{
				brightness_flag.start_tweaking();
				brightness_flag.set(0, 0x41E00000);
				brightness_flag.flush(ps2);
			}
			else
			{
				brightness_flag.stop_tweaking(true);
			}
			sentinel.increment();
		}

		if (c.button_down(controller_bindings::special))
		{
			if (!freeze_ai_flag.currently_tweaking())
			{
				freeze_ai_flag.start_tweaking();
				freeze_ai_flag.set(0, 8);
				freeze_ai_flag.flush(ps2);
			}
			else
			{
				freeze_ai_flag.stop_tweaking(true);
			}
			sentinel.increment();
		}

		if (c.button_down(controller_bindings::freecam))
		{
			if (!camera_flag.is_on())
			{
				camera_flag.set_on(true);
				// hack - give pcsx2 a few frames to assemble the opcode and write the source camera base address, so that when we start tweaking the values are right
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(100ms);
				update_camera_base_address();
				final_camera_matrix.start_tweaking();
				source_camera_matrix.update();
				current_yaw = glm::acos(source_camera_matrix.get(0));
				current_pitch = glm::asin(source_camera_matrix.get(6));
				current_position = glm::vec3(source_camera_matrix.get(12), source_camera_matrix.get(13), source_camera_matrix.get(14));
			}
			else
			{
				camera_flag.set_on(false);
				final_camera_matrix.stop_tweaking(true);
			}

			sentinel.increment();
		}

		if (final_camera_matrix.currently_tweaking())
		{
			update_camera_base_address();

			const float turn_scale = time_delta * 3.0f;
			const float move_scale = time_delta * 1000.0f;

			current_yaw += c.get_right_axis().first * turn_scale;
			current_pitch += -c.get_right_axis().second * turn_scale;

			if (!camera_playback.update(time_delta, current_yaw, current_pitch, current_position.x, current_position.y, current_position.z))
			{
				glm::vec3 pos_delta = shared_camera::compute_freecam_pos_delta(c, glm::vec2(move_scale, -move_scale), current_yaw, current_pitch);
				current_position += glm::vec3(pos_delta.x, pos_delta.z, pos_delta.y);
			}

			// what follows is a C++ recreation of the mips function at 0x001068A0
			// which "cooks" the source camera matrix into a special pre-multiplied form
			// we create a custom source matrix, nop out the final writes of this function, and substitute the results with our own
			glm::mat4 rotation_matrix = shared_camera::compute_rotation_matrix_z_x(current_yaw, current_pitch);
			rotation_matrix = glm::transpose(rotation_matrix);

			glm::vec4 final_position(0.0f, 0.0f, 0.0f, 0.0f);

			final_position += rotation_matrix[0] * current_position.x;
			final_position += rotation_matrix[1] * current_position.y;
			final_position += rotation_matrix[2] * current_position.z;

			glm::mat4 position_mat(1.0f);
			position_mat[3] = glm::vec4(-final_position[0], -final_position[1], -final_position[2], 1.0f);

			glm::mat4 final_mat = position_mat * rotation_matrix;

			final_camera_matrix.set(0, final_mat);

			final_camera_matrix.flush(ps2);
		}
	}
};

ps2_game_static_register<siren> r("D6C48447", "Siren (USA)");