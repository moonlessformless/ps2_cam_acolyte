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

class silent_hill_shattered_memories : public ps2_game
{
private:
	sentinel_counter sentinel;
	tweakable_value_set<float, 16> camera_matrix;
	restoring_toggle_state<1> camera_flag;
	restoring_toggle_state<1> pause_flag;
	float current_yaw = 0.0f;
	float current_pitch = 0.0f;

public:
	explicit silent_hill_shattered_memories(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, camera_flag(ps2, { {
			{ 0x002EB718, 0x00000000, 0x0C0BD5F4 }
		} })
		, pause_flag(ps2, { {
			{ 0x002CF7C8, 0x00000000, 0x0C0B392E }
		} })
		, camera_matrix(ps2, 0x12C4520)
	{
	}

	void draw_game_ui(const pcsx2& ps2, const controller& c, playback& camera_playback) override
	{
		camera_playback.draw_playback_ui(c);
		ImGui::PushTextWrapPos(0.0f); ImGui::TextColored(ui_colors::help, "Note: Freecam use can cause infinite door loads - consider saving a save state before use and loading after."); ImGui::PopTextWrapPos();
		shared_ui::toggle(c, controller_bindings::freecam, camera_flag, "Freecam");
		shared_ui::toggle(c, controller_bindings::pause, pause_flag, "Pause");
	}

	void update_camera_matrix_base_address(const pcsx2& ps2)
	{
		// pretty complicated series of indirections
		// this replicates the beginning of the function at 00126148
		// and starting at a global camera object
		// navigates down to the current camera mode's matrix
		try
		{
			uint32_t s1 = ps2.get_ipc()->Read<uint32_t>(0x00794020 + 0x24);
			uint32_t s0 = ps2.get_ipc()->Read<uint32_t>(s1);
			uint32_t a1 = ps2.get_ipc()->Read<uint32_t>(s0 + 0x1bc + 0x4);

			if (a1 != camera_matrix.get_address(0))
			{
				camera_matrix.update_base_address(a1 + 0x10);
			}
		}
		catch (const PINE::PCSX2::IPCStatus& error)
		{

		}
	}

	void update(const pcsx2& ps2, const controller_state& c, playback& camera_playback, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			pause_flag.reset();
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
				update_camera_matrix_base_address(ps2);
				camera_matrix.start_tweaking();
				current_yaw = -glm::acos(camera_matrix.get(0));
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
			update_camera_matrix_base_address(ps2);

			float turn_scale = time_delta * 3.0f;
			float move_scale = time_delta * 10.0f;

			current_yaw -= c.get_right_axis().first * turn_scale;
			current_pitch += c.get_right_axis().second * turn_scale;

			glm::vec3 pos_delta = shared_camera::compute_freecam_pos_delta(c, glm::vec2(-move_scale, -move_scale), current_yaw, -current_pitch);
			glm::vec3 pos = glm::vec3(camera_matrix.get(12), camera_matrix.get(13), camera_matrix.get(14)) + pos_delta;

			camera_playback.update(time_delta, current_yaw, current_pitch, pos.x, pos.y, pos.z);

			glm::mat4 position_mat(1.0f);
			position_mat[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);

			glm::mat4 final_mat = position_mat * shared_camera::compute_rotation_matrix_y_x(current_yaw, current_pitch);

			//camera_matrix_first_person.set(0, final_mat);
			camera_matrix.set(0, final_mat);

			//camera_matrix_first_person.flush(ps2);
			camera_matrix.flush(ps2);
		}
	}
};

ps2_game_static_register<silent_hill_shattered_memories> r("61A7E622", "Silent Hill - Shattered Memories (USA)");