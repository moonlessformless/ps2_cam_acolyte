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

class silent_hill_origins : public ps2_game
{
private:
	sentinel_counter sentinel;
	toggle_state camera_flag;
	toggle_state brightness_flag;
	tweakable_value_set<char, 1> pause_toggle; // this is a tweakable because we need to continually flush the value
	tweakable_value_set<float, 16> camera_matrix_1;
	tweakable_value_set<float, 16> camera_matrix_2;
	read_only_value_set<char, 1> in_gameplay;
	read_only_value_set<float, 3> player_pos;
	read_only_value_set<float, 3> player_forward;
	float current_yaw = 0.0f;
	float current_pitch = 0.0f;

	bool modern_controls = false;
	toggle_state modern_controls_instructions;
	tweakable_value_set<char, 5> modern_controls_flags;
	float modern_controls_x_offset = 0.0f;
	float modern_controls_y_offset = 0.0f;
	float modern_controls_distance_offset = 0.0f;
	// you have to first enter combat stance, then attack
	float time_until_attack_allowed = 0.0f;
	float time_until_stop_attack_allowed = 0.0f;
	float time_until_fully_out_of_cinematic = 0.0f;

	const int modern_controls_flags_block_offset = 0;
	const int modern_controls_flags_run_offset = 1;
	const int modern_controls_flags_attack_offset = 2;
	const int modern_controls_flags_stance_offset = 3;
	const int modern_controls_flags_on = 4;

public:
	explicit silent_hill_origins(const pcsx2& ps2)
		: sentinel(ps2, 0x00080000)
		, camera_flag(ps2)
		, camera_matrix_1(ps2, 0x0091B5F0)
		, camera_matrix_2(ps2, 0x016E3BD0)
		, in_gameplay(ps2, 0x003386b8)
		, brightness_flag(ps2)
		, pause_toggle(ps2, 0x00339CEC)
		, player_pos(ps2, 0x3449c8)
		, player_forward(ps2, 0x3449d4)
		, modern_controls_instructions(ps2)
		, modern_controls_flags(ps2, { 0x00080004, 0x00080008, 0x0008000C, 0x00080010, 0x00080040 })
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

		modern_controls_instructions.edit_off()
			.write<int32_t>(0x0012FCF4, 0xE62018E4) // restore locking camera orientation
			.write<int32_t>(0x0012FCF8, 0xE62118E8) // restore locking camera orientation
			// we leave the other instructions in, because mondern_controls_flags_on skips the new flag code
			// and doing too much opcode patching seems to cause pcsx2 instability
			.finalize();

		modern_controls_instructions.edit_on()
			.write<int32_t>(0x0012FCF4, 0x00000000) // this "locks" the player's forward to the camera orientation when they started moving, which we don't want
			.write<int32_t>(0x0012FCF8, 0x00000000) // this "locks" the player's forward to the camera orientation when they started moving, which we don't want
			// write a function in scratch to replace 0x0019FF28
			// 0x0019FF28's job is to reads input from the stack and write to the current player state(which moves in memory based on room)
			// our replacement function instead reads the input from the location in scratch offset by a2
			// 1) load the flag from scratch 0x00080000 + a2
			.write<int32_t>(0x00080080 + 4*0, 0x27BDFFE0) // save off ra & t1 - addiu sp,sp,-0x20
			.write<int32_t>(0x00080080 + 4*1, 0xFFBF0000) // sd ra,0x0(sp)
			.write<int32_t>(0x00080080 + 4*2, 0xFFA90008) // sd t1,0x8(sp)
			.write<int32_t>(0x00080080 + 4*3, 0xFFA50010) // sd a1,0x10(sp)


			.write<int32_t>(0x00080080 + 4*4, 0x0280202D) // set up and call the original function first so any side effects are identical
			.write<int32_t>(0x00080080 + 4*5, 0x0C067FCA)
			.write<int32_t>(0x00080080 + 4*6, 0x00000000) // nop - branch delay slot

			.write<int32_t>(0x00080080 + 4*7, 0xDFBF0000) // restore ra & t1 - ld ra,0x0(sp)
			.write<int32_t>(0x00080080 + 4*8, 0xDFA90008) // ld t1,0x8(sp)
			.write<int32_t>(0x00080080 + 4*9, 0xDFA50010) // ld a1,0x10(sp)
			.write<int32_t>(0x00080080 + 4*10, 0x27BD0020) // addiu sp,sp,0x20

			.write<int32_t>(0x00080080 + 4*11, 0x3C080008) // lui t0, 0x0008
			.write<int32_t>(0x00080080 + 4*12, 0x910A0040) // lbu t2, 0x40(t0)
			.write<int32_t>(0x00080080 + 4*13, 0x11400004) // beq t2, zero, 0x000800b0 - skip flag setting if our modern_controls_flags_on is 0
			.write<int32_t>(0x00080080 + 4*14, 0x01094020) // add t0, t0, t1
			.write<int32_t>(0x00080080 + 4*15, 0x81090000) // lb t1, 0x0(t0)
			// 2) apply the flag, following the same logic from 0x0019FF28, which is to write to ((a1 + 4) + 4)
			.write<int32_t>(0x00080080 + 4*16, 0x8CAA0004) // lw t2, 0x4(a1)
			.write<int32_t>(0x00080080 + 4*17, 0xA1490004) // sb t1, 0x4(t2)
			.write<int32_t>(0x00080080 + 4*18, 0x03E00008) // jr ra
			.write<int32_t>(0x00080080 + 4*19, 0x00000000) // nop - branch delay slot
			// each call to 0x0019FF28 is replaced by setting a2 to the flag offset, and then calling the scratch function
			.write<int32_t>(0x00130A0C, 0x34090004) // block propagation - ori t1, zero, 0x04
			.write<int32_t>(0x00130A10, 0x0C020020) // block propagation - swap 0x0019FF28 for our 0x00080080
			.write<int32_t>(0x00130A1C, 0x34090008) // run propagation - ori t1, zero, 0x08
			.write<int32_t>(0x00130A20, 0x0C020020) // run propagation - swap 0x0019FF28 for our 0x00080080
			.write<int32_t>(0x00130A54, 0x3409000C) // stance propagation - ori t1, zero, 0x0C
			.write<int32_t>(0x00130A58, 0x0C020020) // stance propagation - swap 0x0019FF28 for our 0x00080080
			.write<int32_t>(0x00130A64, 0x34090010) // attack propagation - ori t1, zero, 0x10
			.write<int32_t>(0x00130A68, 0x0C020020) // attack propagation - swap 0x0019FF28 for our 0x00080080
			.finalize();

		modern_controls = ps2.get_prefs().read_bool("A8D83239_modern_controls", false);
		modern_controls_x_offset = ps2.get_prefs().read_float("A8D83239_modern_controls_x_offset", 0.0f);
		modern_controls_y_offset = ps2.get_prefs().read_float("A8D83239_modern_controls_y_offset", 1.5f);
		modern_controls_distance_offset = ps2.get_prefs().read_float("A8D83239_modern_controls_distance_offset", 3.0f);
	}

	void draw_game_ui(const pcsx2& ps2, const controller& c, playback& camera_playback) override
	{
		preferences& prefs = ps2.get_prefs();

		camera_playback.draw_playback_ui(c);
		if (!modern_controls)
		{
			shared_ui::toggle(c, controller_bindings::freecam, camera_flag, "Freecam");
		}
		shared_ui::toggle(c, controller_bindings::lighting, brightness_flag, "Brightness Boost");
		shared_ui::toggle(c, controller_bindings::pause, pause_toggle, "Pause");
		ImGui::Separator();
		bool modern_was_on = modern_controls;
		ImGui::Checkbox("Modern Controls Mode", &modern_controls);
		if (modern_was_on != modern_controls)
		{
			prefs.write_bool("A8D83239_modern_controls", modern_controls);
			reset_all();
		}
		if (modern_controls)
		{
			ImGui::Text("Select the game controller you are playing with on PCSX2:");
			ps2.get_game_controller().draw_tool();
			ImGui::Text("Camera Presets: ");
			if (ImGui::Button("Default"))
			{
				prefs.write_on_change_float("A8D83239_modern_controls_x_offset", &modern_controls_x_offset, 0.0f);
				prefs.write_on_change_float("A8D83239_modern_controls_y_offset", &modern_controls_y_offset, 1.5f);
				prefs.write_on_change_float("A8D83239_modern_controls_distnace_offset", &modern_controls_distance_offset, 3.0f);
			}
			ImGui::SameLine();
			if (ImGui::Button("Over Shoulder (Left)"))
			{
				prefs.write_on_change_float("A8D83239_modern_controls_x_offset", &modern_controls_x_offset, 0.35f);
				prefs.write_on_change_float("A8D83239_modern_controls_y_offset", &modern_controls_y_offset, 1.7f);
				prefs.write_on_change_float("A8D83239_modern_controls_distnace_offset", &modern_controls_distance_offset, 1.6f);
			}
			ImGui::SameLine();
			if (ImGui::Button("Over Shoulder (Right)"))
			{
				prefs.write_on_change_float("A8D83239_modern_controls_x_offset", &modern_controls_x_offset, -0.35f);
				prefs.write_on_change_float("A8D83239_modern_controls_y_offset", &modern_controls_y_offset, 1.7f);
				prefs.write_on_change_float("A8D83239_modern_controls_distnace_offset", &modern_controls_distance_offset, 1.6f);
			}
			shared_ui::slider(prefs, "Left/Right Offset", "A8D83239_modern_controls_x_offset", &modern_controls_x_offset, -10.0f, 10.0f);
			shared_ui::slider(prefs, "Height Offset", "A8D83239_modern_controls_y_offset", &modern_controls_y_offset, -2.0f, 2.0f);
			shared_ui::slider(prefs, "Distance", "A8D83239_modern_controls_distance_offset", &modern_controls_distance_offset, 0.0f, 10.0f);
		}
		else
		{
			ImGui::TextWrapped("Gives the game a modern third person control scheme. Enable to select additional options.");
		}
	}

	void reset_all()
	{
		camera_flag.reset();
		camera_matrix_1.reset();
		camera_matrix_2.reset();
		brightness_flag.reset();
		pause_toggle.reset();
		modern_controls_instructions.reset();
		modern_controls_flags.reset();
	}

	void update(const pcsx2& ps2, const controller_state& c, playback& camera_playback, float time_delta) override
	{
		if (sentinel.has_reset())
		{
			reset_all();
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

		if (modern_controls)
		{
			bool changed = false;

			in_gameplay.update();

			if (in_gameplay.get(0) == 0)
			{
				time_until_fully_out_of_cinematic = 0.5f;
			}
			else
			{
				time_until_fully_out_of_cinematic -= time_delta;
			}

			bool camera_should_be_on = time_until_fully_out_of_cinematic <= 0.0f;
			if (camera_should_be_on && !camera_flag.is_on())
			{
				camera_flag.set_on(true);
				camera_matrix_1.start_tweaking();
				camera_matrix_2.start_tweaking();
				player_forward.update();
				glm::vec3 forward = glm::vec3(player_forward.get(0), player_forward.get(1), player_forward.get(2));
				current_yaw = atan2(forward.z, forward.x);
				current_pitch = 0.0f;
				changed = true;
			}
			else if (!camera_should_be_on && camera_flag.is_on())
			{
				camera_flag.set_on(false);
				camera_matrix_1.stop_tweaking(false);
				camera_matrix_2.stop_tweaking(false);
				changed = true;
			}

			if (camera_should_be_on && !modern_controls_flags.currently_tweaking())
			{
				modern_controls_flags.start_tweaking();
				modern_controls_flags.set(modern_controls_flags_on, 1);
				modern_controls_flags.flush(ps2);
				modern_controls_instructions.set_on(true);
				changed = true;
			}
			else if (!camera_should_be_on && modern_controls_flags.currently_tweaking())
			{
				modern_controls_flags.set(modern_controls_flags_on, 0);
				modern_controls_flags.flush(ps2);
				modern_controls_flags.stop_tweaking(false);
				changed = true;
			}

			if (changed)
			{
				sentinel.increment();
			}

			if (modern_controls_flags.currently_tweaking())
			{
				const controller_state& game_controller_state = ps2.get_game_controller().get_state();
				bool wantsToAttack = game_controller_state.get_right_trigger() > 0.5f;
				bool wantsToRun = (game_controller_state.get_left_trigger() > 0.5f && !wantsToAttack) || game_controller_state.button(controller_bindings::square);

				modern_controls_flags.set(modern_controls_flags_block_offset, wantsToRun ? 1 : 0);
				modern_controls_flags.set(modern_controls_flags_run_offset, wantsToRun ? 1 : 0);

				// they're entering combat stance, so use traditional controls
				if (game_controller_state.button(controller_bindings::r1))
				{
					time_until_stop_attack_allowed = 0.0f;
					time_until_attack_allowed = 0.0f;
					modern_controls_flags.set(modern_controls_flags_stance_offset, 1);
					modern_controls_flags.set(modern_controls_flags_attack_offset, game_controller_state.button(controller_bindings::cross) ? 1 : 0);
				}
				else
				{
					// the combat system requires you to briefly hold combat stance before attacking
					// it also requires you to hold the attack button for a bit before you will actually attack(and not cancel)
					// in order to allow tapping or holding R2 to attack, we keep the flags set for a bit after the button change occurs
					if (wantsToAttack)
					{
						modern_controls_flags.set(modern_controls_flags_stance_offset, 1);
						time_until_stop_attack_allowed = 0.1f;
						if (time_until_attack_allowed > 0.0f)
						{
							time_until_attack_allowed -= time_delta;
						}
						modern_controls_flags.set(modern_controls_flags_attack_offset, time_until_attack_allowed < 0.0f ? 1 : 0);
					}
					else
					{
						time_until_attack_allowed = 0.1f;
						modern_controls_flags.set(modern_controls_flags_stance_offset, 0);
						if (time_until_stop_attack_allowed > 0.0f)
						{
							time_until_stop_attack_allowed -= time_delta;
						}
						modern_controls_flags.set(modern_controls_flags_attack_offset, game_controller_state.button(controller_bindings::cross) || time_until_stop_attack_allowed > 0.0f ? 1 : 0);
					}
				}
				modern_controls_flags.flush(ps2);
			}
		}
		else
		{
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
		}

		// end of cinematics, etc. can reset this flag, so we keep writing it
		if (pause_toggle.currently_tweaking())
		{
			pause_toggle.set(0, 0x01);
			pause_toggle.flush(ps2);
		}

		if (camera_flag.is_on())
		{
			float final_yaw = 0.0f;
			float final_pitch = 0.0f;

			float turn_scale = time_delta * 3.0f;
			float move_scale = time_delta * 10.0f;

			glm::vec3 pos;

			if (modern_controls)
			{
				player_pos.update();
				player_forward.update();

				const controller_state& game_controller_state = ps2.get_game_controller().get_state();

				current_yaw += +game_controller_state.get_right_axis().first * turn_scale;
				current_pitch += -game_controller_state.get_right_axis().second * turn_scale;

				current_pitch = glm::clamp(current_pitch, glm::radians(-70.0f), glm::radians(30.0f));

				glm::vec3 cam_forward = glm::vec3(
					glm::cos(current_yaw) * glm::cos(current_pitch),
					glm::sin(current_pitch),
					glm::sin(current_yaw) * glm::cos(current_pitch)
				);

				glm::vec3 cam_right = glm::cross(cam_forward, glm::vec3(0.0f, 1.0f, 0.0f));

				glm::vec3 forward = glm::vec3(player_forward.get(0), player_forward.get(1), player_forward.get(2));

				pos = glm::vec3(player_pos.get(0), player_pos.get(1) + modern_controls_y_offset, player_pos.get(2)) - cam_forward * modern_controls_distance_offset + cam_right * modern_controls_x_offset;

				final_yaw = glm::radians(90.0f) - atan2(cam_forward.z, cam_forward.x);
				final_pitch = -asin(cam_forward.y);
			}
			else
			{
				current_yaw += -c.get_right_axis().first * turn_scale;
				current_pitch += c.get_right_axis().second * turn_scale;

				glm::vec3 pos_delta = shared_camera::compute_freecam_pos_delta(c, glm::vec2(-move_scale, -move_scale), current_yaw, -current_pitch);
				pos = glm::vec3(camera_matrix_1.get(12), camera_matrix_1.get(13), camera_matrix_1.get(14)) + pos_delta;

				final_yaw = current_yaw;
				final_pitch = current_pitch;
			}

			camera_playback.update(time_delta, final_yaw, final_pitch, pos.x, pos.y, pos.z);

			glm::mat4 position_mat(1.0f);
			position_mat[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);

			glm::mat4 final_mat = position_mat * shared_camera::compute_rotation_matrix_y_x(final_yaw, final_pitch);

			camera_matrix_1.set(0, final_mat);
			camera_matrix_2.set(0, final_mat);

			camera_matrix_1.flush(ps2);
			camera_matrix_2.flush(ps2);
		}
	}
};

ps2_game_static_register<silent_hill_origins> r("A8D83239", "Silent Hill Origins (USA)");