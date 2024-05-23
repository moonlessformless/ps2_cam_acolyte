#pragma once

#include "../controller_bindings.h"
#include "imgui.h"

class shared_ui
{
public:
	static void toggle(const controller& c, button_type button, const toggle_state& toggle, const char* label)
	{
		shared_ui::button(c, button); ImGui::SameLine(); ImGui::Text("%s: ", label); ImGui::SameLine();
		ImGui::TextColored(toggle.is_on() ? ui_colors::on_obvious : ui_colors::off_obvious, toggle.is_on() ? "ON" : "OFF");
	}

	template <typename T, size_t N>
	static void toggle(const controller& c, button_type button, const tweakable_value_set<T,N>& toggle, const char* label)
	{
		shared_ui::button(c, button); ImGui::SameLine(); ImGui::Text("%s: ", label); ImGui::SameLine();
		ImGui::TextColored(toggle.currently_tweaking() ? ui_colors::on_obvious : ui_colors::off_obvious, toggle.currently_tweaking() ? "ON" : "OFF");
	}

	static void button(const controller& c, button_type button)
	{
		ImGui::Text("[%s]", c.get_button_display_name(button));
	}
};