#pragma once

#include "preferences.h"
#include "ui_view.h"
#include <SDL_events.h>
#include <thread>
#include <array>
#include <vector>
#include <string>

class controller_impl;

// currently this directly matches SDL_GAMECONTROLLER
enum class button_type
{
	BUTTON_INVALID = -1,
	BUTTON_A,
	BUTTON_B,
	BUTTON_X,
	BUTTON_Y,
	BUTTON_BACK,
	BUTTON_GUIDE,
	BUTTON_START,
	BUTTON_LEFTSTICK,
	BUTTON_RIGHTSTICK,
	BUTTON_LEFTSHOULDER,
	BUTTON_RIGHTSHOULDER,
	BUTTON_DPAD_UP,
	BUTTON_DPAD_DOWN,
	BUTTON_DPAD_LEFT,
	BUTTON_DPAD_RIGHT,
	BUTTON_MISC1,    /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
	BUTTON_PADDLE1,  /* Xbox Elite paddle P1 (upper left, facing the back) */
	BUTTON_PADDLE2,  /* Xbox Elite paddle P3 (upper right, facing the back) */
	BUTTON_PADDLE3,  /* Xbox Elite paddle P2 (lower left, facing the back) */
	BUTTON_PADDLE4,  /* Xbox Elite paddle P4 (lower right, facing the back) */
	BUTTON_TOUCHPAD, /* PS4/PS5 touchpad button */
	BUTTON_MAX
};

constexpr size_t button_type_count = static_cast<size_t>(button_type::BUTTON_MAX);

class controller_state
{
private:
	struct button_state
	{
		bool down = false;
		size_t frame_at_change = 0;
	};
	std::array<button_state, button_type_count> button_states;
	float leftStickX = 0.0f;
	float leftStickY = 0.0f;
	float rightStickX = 0.0f;
	float rightStickY = 0.0f;
	size_t last_update_frame = 0;

public:
	controller_state();
	void new_frame();
	void set_left_axis(float x, float y, float sensitivity, float deadzone);
	void set_right_axis(float x, float y, float sensitivity, float deadzone);
	void set_button_state(button_type, bool currently_down);

	std::pair<float,float> get_left_axis() const;
	std::pair<float, float> get_right_axis() const;
	bool button(button_type) const;
	bool button_down(button_type) const;
	bool button_up(button_type) const;
};

class controller : public ui_tool_view
{
private:
	preferences& prefs;
	std::unique_ptr<controller_impl> impl;
	controller_state state;
	std::vector<std::string> device_list;
	float joystick_sensitivity = 1.0f;
	float joystick_deadzone = 0.25f;

	void refresh_device_list();
	int sdl_to_device_list_index(int index) const;
	int device_list_index_to_sdl(int index) const;

public:
	explicit controller(preferences& prefs);
	~controller();
	void new_frame();
	void handle_event(const SDL_Event& event);
	const controller_state& get_state() const;
	void use_device(int index);
	int get_current_device_index() const;
	const std::vector<std::string>& get_device_list() const;
	const char* get_button_display_name(button_type type) const;

	// ui
	bool has_status() const override;
	void draw_status() const override;
	bool is_selectable() const override;
	const char* selectable_name() const override;
	void draw_tool() override;
};