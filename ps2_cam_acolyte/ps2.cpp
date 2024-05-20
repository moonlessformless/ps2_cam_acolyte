#include <chrono>
#include <thread>
#include <iostream>
#include "ps2.h"
#include "imgui.h"
#include <unistd.h>
#include <memory>

pcsx2::pcsx2()
    : ipc(new PINE::PCSX2())
{
    last_update_time = clock.now();
    status.state = connection_status::connected_state::connecting;

    ipc->InitializeBatch();
    ipc->GetGameUUID<true>();
    determine_game_command = ipc->FinalizeBatch();
}

pcsx2::~pcsx2()
{
    determine_game_command.Free();
}

void pcsx2::determine_game()
{
    try
    {
        ipc->SendCommand(determine_game_command);
        char* uuid = ipc->GetReply<PINE::PCSX2::MsgUUID>(determine_game_command, 0, &determine_game_command_reply);
        for (int i = 0; i < strlen(uuid); ++i) { uuid[i] = toupper(uuid[i]); }

        if (status.state != connection_status::connected_state::game_found || strcmp(uuid, status.game_uuid.c_str()) != 0)
        {
            current_game = ps2_game_registry::create_ps2_game(uuid, *this);
            status.state = connection_status::connected_state::game_found;
            const char* name = ps2_game_registry::name_for_uuid(uuid);
            if (name != nullptr)
            {
                status.game_name = name;
            }
            else
            {
                status.game_name = "Unsupported Game";
            }
            status.game_uuid = uuid;
        }
    }
    catch (const PINE::PCSX2::IPCStatus& error)
    {
        std::cerr << "IPC error: " << error;
        current_game = nullptr;
        status.state = connection_status::connected_state::connecting;
    }
}

void pcsx2::update(const controller_state& c)
{
    auto now = clock.now();
    auto elapsed = now - last_update_time;
    float time_delta = std::chrono::duration_cast<std::chrono::duration<float>>(elapsed).count();
    last_update_time = now;

    time_until_next_game_check -= time_delta;

    if (time_until_next_game_check <= 0.0f)
    {
        determine_game();
        time_until_next_game_check = time_between_game_checks;
    }

    if (current_game)
    {
        current_game->update(*this, c, time_delta);
    }
}


bool pcsx2::has_status() const
{
    return true;
}

void pcsx2::draw_status() const
{
    switch (status.state)
    {
    case connection_status::connected_state::started:
        ImGui::TextColored(ui_colors::neutral, "Connecting to PCSX2...");
        break;
    case connection_status::connected_state::connecting:
        ImGui::TextColored(ui_colors::neutral, "Connecting to PCSX2...");
        break;
    case connection_status::connected_state::connected:
        ImGui::TextColored(ui_colors::neutral, "Connected. No applicable game running.");
        break;
    case connection_status::connected_state::game_found:
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.8f, 1.0f), "[%s]", status.game_uuid.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.8f, 1.0f), status.game_name.c_str());
        break;
    }
}

bool pcsx2::is_selectable() const
{
    return true;
}

const char* pcsx2::selectable_name() const
{
    return "Current Game";
}

void pcsx2::draw_tool()
{
    if (current_game != nullptr)
    {
        current_game->draw_game_ui();
    }
    else
    {
        draw_status();
    }
}