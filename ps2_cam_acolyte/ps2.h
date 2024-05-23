#pragma once

#include <thread>
#include <format>
#include "controller_bindings.h"
#include "playback.h"
#include "ps2_game.h"
#include "ps2_commands.h"

class controller;

class pcsx2 : public ui_tool_view, public non_copyable<pcsx2>
{
public:
	struct connection_status
	{
		enum class connected_state
		{
			started,
			connecting,
			connected,
			game_found
		};
		connected_state state;
		std::string game_name;
		std::string game_uuid;
	};

private:
	PINE::PCSX2* ipc = nullptr;
	const controller* controller_ptr;
	std::unique_ptr<ps2_game> current_game;
	connection_status status;
	std::chrono::high_resolution_clock clock;
	std::chrono::high_resolution_clock::time_point last_update_time;
	PINE::Shared::BatchCommand determine_game_command;
	std::vector<char> determine_game_command_reply;
	static constexpr float time_between_game_checks = 0.25f;
	float time_until_next_game_check = 0.0f;
	playback camera_playback;

	void determine_game();

public:
	pcsx2(const controller* controller);
	~pcsx2();
	bool update();
	PINE::PCSX2* get_ipc() const { return ipc; }

	// ui
	bool has_status() const override;
	void draw_status() const override;
	bool is_selectable() const override;
	const char* selectable_name() const override;
	void draw_tool() override;
};