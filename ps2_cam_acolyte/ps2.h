#pragma once

#include <thread>
#include <format>
#include "controller.h"
#include "ps2_game.h"
#include "ps2_commands.h"

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
	std::unique_ptr<ps2_game> current_game;
	connection_status status;
	std::chrono::high_resolution_clock clock;
	std::chrono::high_resolution_clock::time_point last_update_time;
	PINE::Shared::BatchCommand determine_game_command;
	std::vector<char> determine_game_command_reply;
	static constexpr float time_between_game_checks = 0.5f;
	float time_until_next_game_check = time_between_game_checks;

	void determine_game();

public:
	pcsx2();
	~pcsx2();
	void update(const controller_state& c);
	PINE::PCSX2* get_ipc() const { return ipc; }

	// ui
	bool has_status() const override;
	void draw_status() const override;
	bool is_selectable() const override;
	const char* selectable_name() const override;
	void draw_tool() override;
};