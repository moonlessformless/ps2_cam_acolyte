#include "misc_ui_views.h"
#include "ps2_game.h"
#include "version.h"
#include "imgui.h"

bool game_library_ui_view::is_selectable() const { return true;  }
const char* game_library_ui_view::selectable_name() const { return "Game Library"; }
void game_library_ui_view::draw_tool()
{
	for (const std::string& s : ps2_game_registry::list_ps2_games())
	{
		ImGui::Text(s.c_str());
	}
}

void OsOpenInShell(const char* path)
{
#ifdef _WIN32
    ::ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWDEFAULT);
#else
#if __APPLE__
    const char* open_executable = "open";
#else
    const char* open_executable = "xdg-open";
#endif
    char command[256];
    snprintf(command, 256, "%s \"%s\"", open_executable, path);
    system(command);
#endif
}

bool about_ui_view::is_selectable() const { return true; }
const char* about_ui_view::selectable_name() const { return "About"; }
void about_ui_view::draw_tool()
{
	ImGui::Text("Written by Moonless Formless");
	ImGui::Text("PS2 Cam Acolyte v%d.%02d", current_version.major, current_version.minor);
    if (ImGui::Button("Check for Newer Version"))
    {
        OsOpenInShell("https://github.com");
    }
}