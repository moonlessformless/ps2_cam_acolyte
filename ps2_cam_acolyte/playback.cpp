#include "playback.h"
#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtx/spline.inl>

playback::playback()
{
	keyframes.reserve(2048);
}

void playback::clear()
{
	keyframes.clear();
}

void playback::draw_playback_ui(const controller& c)
{
	ImGui::Text("Camera Playback");
	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();
	if (current_mode == mode::none)
	{
		if (!keyframes.empty())
		{
			if (ImGui::Button("Play"))
			{
				current_mode = mode::playing;
				playhead = 0;
				frame_progress = 0.0f;
			}
		}
		else
		{
			if (ImGui::Button("Record"))
			{
				current_mode = mode::recording;
				playhead = 0;
				frame_progress = 0.0f;
			}
		}
		ImGui::SameLine();
		ImGui::Text("%d frames", keyframes.size());
	}
	else if (current_mode == mode::recording)
	{
		if (ImGui::Button("Stop"))
		{
			current_mode = mode::none;
		}
		ImGui::SameLine();
		ImGui::Text("%d frames", keyframes.size());
	}
	else if (current_mode == mode::playing)
	{
		if (ImGui::Button("Stop"))
		{
			current_mode = mode::none;
		}
		ImGui::SameLine();
		ImGui::Text("%d/%d", playhead, keyframes.size());

		ImGui::SameLine();
		ImGui::PushItemWidth(100.0f);
		ImGui::SliderFloat("Speed", &playback_speed, 0.0f, 4.0f, "%.1f");
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::PushItemWidth(100.0f);
		ImGui::SliderInt("Smoothing", &keyframe_delta, 1, 10);
		ImGui::PopItemWidth();
	}
	if (current_mode == mode::none)
	{
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			clear();
		}
	}
	ImGui::Separator();
}

float interpolate(float x0, float x1, float x2, float x3, float t)
{
	float a = (3.0 * x1 - 3.0 * x2 + x3 - x0) / 2.0;
	float b = (2.0 * x0 - 5.0 * x1 + 4.0 * x2 - x3) / 2.0;
	float c = (x2 - x0) / 2.0;
	float d = x1;

	return a * t * t * t + b * t * t + c * t + d;
}

bool playback::update(float time_delta, float& yaw, float& pitch, float& x, float& y, float& z)
{
	constexpr float framerate = 30.0f;

	switch (current_mode)
	{
	case playback::mode::none:
		return false;
	case playback::mode::recording:
		frame_progress += time_delta * framerate;
		while (frame_progress > 1.0f)
		{
			keyframes.emplace_back(keyframe{
				.yaw = yaw,
				.pitch = pitch,
				.x = x,
				.y = y,
				.z = z
				});
			++playhead;
			frame_progress -= 1.0f;
		}
		return false;
	case playback::mode::playing:
		frame_progress += time_delta * framerate * playback_speed / (float)keyframe_delta;
		while (frame_progress > 1.0f)
		{
			playhead += keyframe_delta;
			playhead = playhead % keyframes.size();
			frame_progress -= 1.0f;
		}
		if (playhead > keyframe_delta && playhead < keyframes.size() - keyframe_delta * 2)
		{
			const auto& prev_keyframe = keyframes[playhead - keyframe_delta];
			const auto& keyframe = keyframes[playhead];
			const auto& next_keyframe = keyframes[playhead + keyframe_delta];
			const auto& next_next_keyframe = keyframes[playhead + keyframe_delta * 2];
			yaw = interpolate(prev_keyframe.yaw, keyframe.yaw, next_keyframe.yaw, next_next_keyframe.yaw, frame_progress);
			pitch = interpolate(prev_keyframe.pitch, keyframe.pitch, next_keyframe.pitch, next_next_keyframe.pitch, frame_progress);
			x = interpolate(prev_keyframe.x, keyframe.x, next_keyframe.x, next_next_keyframe.x, frame_progress);
			y = interpolate(prev_keyframe.y, keyframe.y, next_keyframe.y, next_next_keyframe.y, frame_progress);
			z = interpolate(prev_keyframe.z, keyframe.z, next_keyframe.z, next_next_keyframe.z, frame_progress);
		}
		return true;
	default:
		return false;
	}
}

bool playback::needs_render() const
{
	return current_mode != mode::none;
}