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

class nothing : public ps2_game
{
private:
	sentinel_counter sentinel;

public:
	explicit nothing(const pcsx2& ps2)
		: sentinel(ps2, 0x01000000)
	{
	}

	void draw_game_ui(const pcsx2& ps2, const controller& c, playback& camera_playback) override
	{
		ImGui::Text("No game running.");
	}

	void update(const pcsx2& ps2, const controller_state& c, playback& camera_playback, float time_delta) override
	{
		if (sentinel.has_reset())
		{
		}

		if (c.button_down(controller_bindings::freecam))
		{
			// ...
			sentinel.increment();
		}
	}
};

ps2_game_static_register<nothing> r("00000000", "No game running");