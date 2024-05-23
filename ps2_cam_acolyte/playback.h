#pragma once

#include <vector>

class controller;

class playback
{
private:
	struct keyframe
	{
		float yaw = 0.0f;
		float pitch = 0.0f;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};

	std::vector<keyframe> keyframes;

	enum class mode
	{
		none,
		recording,
		playing
	};

	mode current_mode = mode::none;
	int playhead = 0;
	float playback_speed = 1.0f;
	float frame_progress = 0.0f;
	int keyframe_delta = 1;

public:
	playback();
	void clear();
	void draw_playback_ui(const controller& c);
	// if this returns true, the values were written to
	bool update(float time_delta, float& yaw, float& pitch, float& x, float& y, float& z);
	bool needs_render() const;
};