#include "controller.h"
#include "imgui.h"
#include <SDL.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <syncstream>

controller_state::controller_state()
{
}

void controller_state::new_frame()
{
    last_update_frame++;
}

float add_dead_zone(float i, float sensitivity, float deadzone)
{
    if (i >= deadzone)
    {
        float value = powf((i - deadzone) / (1.0f - deadzone), 2.0f);
        return value * sensitivity;
    }
    else if (i <= -deadzone)
    {
        float value = -powf((-i - deadzone) / (1.0f - deadzone), 2.0f);
        return value * sensitivity;
    }
    else
    {
        return 0.0f;
    }
}

void controller_state::set_left_axis(float x, float y, float sensitivity, float deadzone)
{
    leftStickX = add_dead_zone(x, sensitivity, deadzone);
    leftStickY = add_dead_zone(y, sensitivity, deadzone);
}

void controller_state::set_right_axis(float x, float y, float sensitivity, float deadzone)
{
    rightStickX = add_dead_zone(x, sensitivity, deadzone);
    rightStickY = add_dead_zone(y, sensitivity, deadzone);
}

void controller_state::set_button_state(button_type t, bool currently_down)
{
    button_states[(size_t)t].down = currently_down;
    button_states[(size_t)t].frame_at_change = last_update_frame;
}

std::pair<float, float> controller_state::get_left_axis() const
{
    return { leftStickX, leftStickY };
}

std::pair<float, float> controller_state::get_right_axis() const
{
    return { rightStickX, rightStickY };
}

bool controller_state::button(button_type t) const
{
    return button_states[(size_t)t].down;
}

bool controller_state::button_down(button_type t) const
{
    return button_states[(size_t)t].down && button_states[(size_t)t].frame_at_change == last_update_frame;
}

bool controller_state::button_up(button_type t) const
{
    return !button_states[(size_t)t].down && button_states[(size_t)t].frame_at_change == last_update_frame;
}

class controller_impl
{
public:
    SDL_GameController* game_controller = nullptr;
};

controller::controller(preferences& prefs)
    : prefs(prefs)
    , impl(new controller_impl)
{
    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

    joystick_sensitivity = prefs.read_float("joystick_sensitivity", joystick_sensitivity);
    joystick_deadzone = prefs.read_float("joystick_deadzone", joystick_deadzone);

    bool found_previous = false;
    if (SDL_NumJoysticks() > 0)
    {
        const char* previous = prefs.read("controller");
        if (previous != nullptr)
        {
            for (int i = 0; i < SDL_NumJoysticks(); ++i)
            {
                if (strcmp(previous, SDL_JoystickPathForIndex(i)) == 0)
                {
                    found_previous = true;
                    use_device(sdl_to_device_list_index(i));
                    break;
                }
            }
        }
    }

    if (!found_previous)
    {
        use_device(0);
    }

    refresh_device_list();
}

controller::~controller()
{
    if (impl->game_controller) {
        SDL_GameControllerClose(impl->game_controller);
    }
}

float to_float(int16_t int16_value)
{
    float float_value = static_cast<float>(int16_value);
    float normalized_value = float_value / 32768.0f;
    return normalized_value;
}

void controller::use_device(int device_list_index)
{
    int current_index = get_current_device_index();

    if (device_list_index == current_index)
    {
        return;
    }

    if (impl->game_controller)
    {
        SDL_GameControllerClose(impl->game_controller);
        impl->game_controller = nullptr;
    }

    int index = device_list_index_to_sdl(device_list_index);

    if (index < 0 || index >= SDL_NumJoysticks())
    {
        prefs.write("controller", "none");
        return;
    }

    impl->game_controller = SDL_GameControllerOpen(index);
    prefs.write("controller", SDL_JoystickPathForIndex(index));
}

void controller::refresh_device_list()
{
    device_list.clear();
    device_list.push_back("None");
    device_list.reserve(SDL_NumJoysticks());
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        device_list.push_back(std::to_string(i + 1) + ". " + SDL_GameControllerNameForIndex(i));
    }
}

// device list 0 is always "None"
int controller::sdl_to_device_list_index(int index) const
{
    return index + 1;
}

int controller::device_list_index_to_sdl(int sdl_index) const
{
    return sdl_index - 1;
}

void controller::new_frame()
{
    state.new_frame();

    if (impl->game_controller)
    {
        int16_t xMove = SDL_GameControllerGetAxis(impl->game_controller, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t yMove = SDL_GameControllerGetAxis(impl->game_controller, SDL_CONTROLLER_AXIS_LEFTY);
        state.set_left_axis(to_float(xMove), to_float(yMove), joystick_sensitivity, joystick_deadzone);
        xMove = SDL_GameControllerGetAxis(impl->game_controller, SDL_CONTROLLER_AXIS_RIGHTX);
        yMove = SDL_GameControllerGetAxis(impl->game_controller, SDL_CONTROLLER_AXIS_RIGHTY);
        state.set_right_axis(to_float(xMove), to_float(yMove), joystick_sensitivity, joystick_deadzone);
    }
}


void controller::handle_event(const SDL_Event& e)
{
    if (e.type == SDL_CONTROLLERBUTTONDOWN)
    {
        state.set_button_state((button_type)e.cbutton.button, true);
    }
    else if (e.type == SDL_CONTROLLERBUTTONUP)
    {
        state.set_button_state((button_type)e.cbutton.button, false);
    }
    else if (e.type == SDL_JOYDEVICEADDED || e.type == SDL_JOYDEVICEREMOVED)
    {
        refresh_device_list();
    }
}

const controller_state& controller::get_state()
{
    return state;
}

int controller::get_current_device_index() const
{
    if (impl->game_controller)
    {
        const char* current_path = SDL_GameControllerPath(impl->game_controller);
        for (int i = 0; i < SDL_NumJoysticks(); ++i)
        {
            const char* compare_path = SDL_JoystickPathForIndex(i);
            if (strcmp(compare_path, current_path) == 0)
            {
                return sdl_to_device_list_index(i);
            }
        }
        
        return 0;
    }
    else
    {
        return 0;
    }
}

const std::vector<std::string>& controller::get_device_list() const
{
    return device_list;
}

bool controller::has_status() const
{
    return true;
}

void controller::draw_status() const
{
    if (impl->game_controller)
    {
        ImGui::Text(device_list[get_current_device_index()].c_str());
    }
    else
    {
        ImGui::TextColored(ui_colors::bad, "No controller selected");
    }
}

bool controller::is_selectable() const
{
    return true;
}

const char* controller::selectable_name() const
{
    return "Controller";
}

void controller::draw_tool()
{
    if (device_list.empty())
    {
        ImGui::TextColored(ui_colors::bad, "No controllers detected.");
    }
    else
    {
        float initial_sensitivity = joystick_sensitivity;
        if (ImGui::Button("Default (1.00)")) { joystick_sensitivity = 1.0f; } ImGui::SameLine();
        ImGui::SliderFloat("Joystick Sensitivity", &joystick_sensitivity, 0.1f, 5.0f, "%.2f");
        if (joystick_sensitivity != initial_sensitivity)
        {
            prefs.write_float("joystick_sensitivity", joystick_sensitivity);
        }

        float initial_deadzone = joystick_deadzone;
        if (ImGui::Button("Default (0.25)")) { joystick_deadzone = 0.25f; } ImGui::SameLine();
        ImGui::SliderFloat("Joystick Deadzone", &joystick_deadzone, 0.0f, 1.0f, "%.2f");
        if (joystick_deadzone != initial_deadzone)
        {
            prefs.write_float("joystick_deadzone", joystick_deadzone);
        }

        int current = get_current_device_index();
        for (int i = 0; i < device_list.size(); ++i)
        {
            if (ImGui::RadioButton(device_list[i].c_str(), current == i))
            {
                if (current != i)
                {
                    use_device(i);
                }
            }
        }
    }
}