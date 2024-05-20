#include "ui_view.h"
#include "imgui.h"
#include "version.h"

void ui_main::add_tool_view(ui_tool_view* view)
{
    tool_views.push_back(view);
}

void ui_main::remove_tool_view(ui_tool_view* view)
{
    tool_views.erase(std::remove(tool_views.begin(), tool_views.end(), view), tool_views.end());
}

void ui_main::render()
{
    ++total_frames;

    bool debug_render_frames = false;

    ImGui::Text("PS2 Cam Acolyte v%d.%02d", current_version.major, current_version.minor);
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    for (const ui_tool_view* view : tool_views)
    {
        if (view->has_status())
        {
            view->draw_status();
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
        }
    }
    if (debug_render_frames)
    {
        ImGui::Text("%d", total_frames);
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
    }
    ImGui::NewLine();
    ImGui::Separator();

    ImGui::BeginChild("categories", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
    for (int i = 0; i < tool_views.size(); ++i)
    {
        const ui_tool_view* view = tool_views[i];
        if (view->is_selectable())
        {
            if (ImGui::Selectable(view->selectable_name(), selected_tool_index == i))
                selected_tool_index = i;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("tool");

    if (selected_tool_index >= 0 && selected_tool_index < tool_views.size())
    {
        tool_views[selected_tool_index]->draw_tool();
    }
    else
    {
        selected_tool_index = 0;
    }

    ImGui::EndChild();
}