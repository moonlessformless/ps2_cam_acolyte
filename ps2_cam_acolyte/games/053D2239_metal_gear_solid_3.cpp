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

class metal_gear_solid_3 : public ps2_game
{
private:
	sentinel_counter sentinel;
	restoring_toggle_state<3> sixty_fps_flag;
	restoring_toggle_state<3> slomo_flag;
	restoring_toggle_state<6> camera_flag;
	tweakable_value_set<float, 16> camera_matrix;
	tweakable_value_set<float, 2> camera_zoom;
	float current_yaw = 0.0f;
	float current_pitch = 0.0f;

public:
	explicit metal_gear_solid_3(const pcsx2& ps2)
		: sentinel(ps2, 0x00AA000) // our normal sentinel address gets written to by streaming data
		, sixty_fps_flag(ps2, { {
			{ 0x001D5AD8, 0x00000000, 0x00000040 },
			{ 0x001D6DB8, 0x00000001, 0x00000002 },
			{ 0x001D6DBC, 0x00000000, 0x00000000 },\
		}})
		// this slomo flag is pretty weak, it just sets 60hz gameplay update but renders at 30hz - todo: find a better way to pause/slow down mgs 3 gameplay
		, slomo_flag(ps2, { {
			{ 0x001D5AD8, 0x00000000, 0x00000040 },
			{ 0x001D6DB8, 0x00000002, 0x00000002 },
			{ 0x001D6DBC, 0x00000000, 0x00000000 },\
		} })
		, camera_flag(ps2, { {
			{ 0x001284A8, 0x00000000, 0xB1EA0007 },
			{ 0x001284AC, 0x00000000, 0xB5EA0000 },
			{ 0x001284B0, 0x00000000, 0xB1EB000F },
			{ 0x001284B4, 0x00000000, 0xB5EB0008 },
			{ 0x00147640, 0x00000000, 0xF8810000 }, // zoom x
			{ 0x00147654, 0x00000000, 0xF8820010 } // zoom y
		} })
		, camera_matrix(ps2, 0x0259320)
		, camera_zoom(ps2, { {
			0x02593A0, // we don't affect these, just lock them
			0x02593B4
		}})
	{
	}

	void draw_game_ui(const pcsx2& ps2, const controller& c, playback& camera_playback) override
	{
		camera_playback.draw_playback_ui(c);
		if (!sixty_fps_flag.is_on())
		{
			if (ImGui::Button("Enable 60 FPS Gameplay"))
			{
				sixty_fps_flag.set_on(true);
				sentinel.increment();
			}
		}
		else if (sixty_fps_flag.is_on())
		{
			if (ImGui::Button("Disable 60 FPS Gameplay"))
			{
				sixty_fps_flag.set_on(false);
				sentinel.increment();
			}
		}
		shared_ui::toggle(c, controller_bindings::freecam, camera_flag, "Freecam");
		ImGui::TextColored(ui_colors::help, "Note: 60 fps will be temporarily disabled when slomo is on");
		shared_ui::toggle(c, controller_bindings::pause, slomo_flag, "Slomo");
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
			slomo_flag.toggle();
			sentinel.increment();
		}

		if (c.button_down(controller_bindings::freecam))
		{
			if (!camera_flag.is_on())
			{
				camera_flag.set_on(true);
				camera_matrix.start_tweaking();
				camera_zoom.start_tweaking();
				current_yaw = -glm::acos(camera_matrix.get(0));
				current_pitch = glm::acos(camera_matrix.get(5));
			}
			else
			{
				camera_zoom.stop_tweaking();
				camera_flag.set_on(false);
				camera_matrix.stop_tweaking(true);
			}
			sentinel.increment();
		}

		if (sixty_fps_flag.is_on())
		{
			sixty_fps_flag.force_send(); // occasionally the game will overwrite these values
		}

		if (slomo_flag.is_on())
		{
			slomo_flag.force_send();
		}

		if (camera_matrix.currently_tweaking())
		{
			float turn_scale = time_delta * 3.0f;
			float move_scale = time_delta * 10000.0f;

			current_yaw += -c.get_right_axis().first * turn_scale;
			current_pitch += -c.get_right_axis().second * turn_scale;

			glm::vec3 pos_delta = shared_camera::compute_freecam_pos_delta(c, glm::vec2(-move_scale, -move_scale), current_yaw, -current_pitch);
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

ps2_game_static_register<metal_gear_solid_3> r("053D2239", "Metal Gear Solid 3: Subsistence (USA)");