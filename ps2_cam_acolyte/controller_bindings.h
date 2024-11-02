#pragma once

#include "controller.h"

class controller_bindings
{
public:
	static constexpr button_type freecam = button_type::BUTTON_A;
	static constexpr button_type pause = button_type::BUTTON_X;
	static constexpr button_type lighting = button_type::BUTTON_Y;
	static constexpr button_type special = button_type::BUTTON_B;

	static constexpr button_type cross = button_type::BUTTON_A;
	static constexpr button_type square = button_type::BUTTON_X;
	static constexpr button_type triangle = button_type::BUTTON_Y;
	static constexpr button_type circle = button_type::BUTTON_B;
	static constexpr button_type l1 = button_type::BUTTON_LEFTSHOULDER;
	static constexpr button_type r1 = button_type::BUTTON_RIGHTSHOULDER;
};