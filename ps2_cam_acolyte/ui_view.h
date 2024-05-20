#pragma once

#include "imgui.h"
#include "utils.h"
#include <vector>

class main_ui;
class ui_impl;

class ui_colors
{
public:
	static constexpr ImVec4 bad = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	static constexpr ImVec4 good = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	static constexpr ImVec4 neutral = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
	static constexpr ImVec4 help = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	static constexpr ImVec4 warning = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
	static constexpr ImVec4 on_obvious = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	static constexpr ImVec4 off_obvious = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
};

class ui_tool_view
{
public:
	virtual bool has_status() const { return false; }
	virtual void draw_status() const {}
	virtual bool is_selectable() const { return false; }
	virtual const char* selectable_name() const { return nullptr; }
	virtual void draw_tool() {}
};

class ui_main : non_copyable<ui_main>
{
	int selected_tool_index = 0;
	int total_frames = 0;
	std::vector<ui_tool_view*> tool_views;

public:
	void add_tool_view(ui_tool_view* view);
	void remove_tool_view(ui_tool_view* view);
	void render();
};