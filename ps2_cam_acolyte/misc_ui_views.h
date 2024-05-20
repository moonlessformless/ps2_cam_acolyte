#pragma once

#include "ui_view.h"
#include "ps2.h"

class game_library_ui_view : public ui_tool_view
{
public:
	bool is_selectable() const override;
	const char* selectable_name() const override;
	void draw_tool() override;
};

class about_ui_view : public ui_tool_view
{
public:
	bool is_selectable() const override;
	const char* selectable_name() const override;
	void draw_tool() override;
};