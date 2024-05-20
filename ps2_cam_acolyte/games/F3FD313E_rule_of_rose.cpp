#include "../ps2.h"
#include "../ps2_commands.h"
#include "shared_utils.h"
#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"

class rule_of_rose : public ps2_game
{
	sentinel_counter sentinel;
	multi_toggle_state speed_flag;
	tweakable_value_set<float, 7> camera_values;
	read_only_value_set<float, 2> camera_true_values;

	static constexpr int camera_forward = 0;
	static constexpr int camera_up = 1;
	static constexpr int camera_right = 2;
	static constexpr int camera_yaw = 3;
	static constexpr int camera_pitch = 4;

	static constexpr int camera_true_yaw = 0;
	static constexpr int camera_true_pitch = 1;

	enum class freecam_mode_type
	{
		none,
		camera
	};
	freecam_mode_type freecam_mode = freecam_mode_type::none;

public:
	explicit rule_of_rose(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
		, speed_flag(ps2, 3)
		, camera_values(ps2, {
			0x00726960, // camera forward
			0x00726964, // camera up
			0x00726968, // camera right
			0x00726974, // camera yaw
			0x00726970, // camera pitch
			0x00726978, // camera roll
			0x00726958, // camera zoom
			})
		, camera_true_values(ps2,
			{
			0x007268F0 + 0x00000054,
			0x007268F0 + 0x00000050
			})
	{
		const uint32_t speedAnimationAddress = 0x006FE890;
		const uint32_t speedTraversal1Address = 0x002EA740;
		const uint32_t speedTraversal2Address = 0x002EA744;
		const uint32_t normalSpeed = 0x3F800000;
		const uint32_t pauseSpeed = 0x00000000;
		const uint32_t fastSpeed = 0x40000000;
		speed_flag.edit_cmd(0).write<uint32_t>(speedAnimationAddress, normalSpeed).write<uint32_t>(speedTraversal1Address, normalSpeed).write<uint32_t>(speedTraversal2Address, normalSpeed).finalize();
		speed_flag.edit_cmd(1).write<uint32_t>(speedAnimationAddress, pauseSpeed).write<uint32_t>(speedTraversal1Address, pauseSpeed).write<uint32_t>(speedTraversal2Address, pauseSpeed).finalize();
		speed_flag.edit_cmd(2).write<uint32_t>(speedAnimationAddress, fastSpeed).write<uint32_t>(speedTraversal1Address, fastSpeed).write<uint32_t>(speedTraversal2Address, fastSpeed).finalize();
	}


	void update(const pcsx2& ps2, const controller_state& c, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			camera_values.reset();
			speed_flag.reset();
		}

		if (c.button_down(button_type::BUTTON_X))
		{
			speed_flag.toggle();
			sentinel.increment();
		}

		if (c.button_down(button_type::BUTTON_A))
		{
			if (freecam_mode == freecam_mode_type::camera) freecam_mode = freecam_mode_type::none;
			else freecam_mode = freecam_mode_type::camera;

			if (freecam_mode != freecam_mode_type::none)
			{
				if (!camera_values.currently_tweaking())
				{
					camera_values.start_tweaking();
				}
			}
			else
			{
				if (camera_values.currently_tweaking())
				{
					camera_values.stop_tweaking(true);
				}
			}
			sentinel.increment();
		}

		if (camera_values.currently_tweaking())
		{
			float turn_scale = time_delta * 3.0f;
			float move_scale = time_delta * 1000.0f;

			camera_values.add(camera_yaw, c.get_right_axis().first * turn_scale);
			camera_values.add(camera_pitch, -c.get_right_axis().second * turn_scale);

			camera_true_values.update();
			float true_yaw = camera_true_values.get(camera_true_yaw);
			float true_pitch = camera_true_values.get(camera_true_pitch);
			float current_yaw = camera_values.get(camera_yaw) + true_yaw;
			float current_pitch = camera_values.get(camera_pitch) + true_pitch;

			glm::vec3 pos_delta = shared_utils::compute_freecam_pos_delta(c, glm::vec2(move_scale, -move_scale), current_yaw, current_pitch);

			camera_values.add(camera_forward, pos_delta.x);
			camera_values.add(camera_up, -pos_delta.y);
			camera_values.add(camera_right, pos_delta.z);

			camera_values.flush(ps2);
		}
	}

	void draw_game_ui() override
	{
		const char* speed_description = nullptr;
		switch (speed_flag.current_index())
		{
			case 0: speed_description = "Normal"; break;
			case 1: speed_description = "Paused"; break;
			case 2: speed_description = "Fast"; break;
		}
		ImGui::Text("Freecam (A): "); ImGui::SameLine();
		ImGui::TextColored(freecam_mode == freecam_mode_type::camera ? ui_colors::on_obvious : ui_colors::off_obvious, freecam_mode == freecam_mode_type::camera ? "ON" : "OFF");
		ImGui::Text("Movement Speed (X): "); ImGui::SameLine();
		ImGui::TextColored(speed_flag.current_index() > 0 ? ui_colors::warning : ui_colors::off_obvious, speed_description);
	}
};

ps2_game_static_register<rule_of_rose> r("F3FD313E", "Rule of Rose");