#include <iostream>
#include <chrono>
#include "controller.h"
#include "misc_ui_views.h"
#include "ps2.h"
#include "ui_host.h"
#include "preferences.h"
#include <SDL_events.h>
#include <chrono>

extern void draw_ui();

int main()
{
    preferences prefs;
    ui_host ui_host;
    ui_main ui_main;
    controller controller(prefs);
    pcsx2 ps2;

    game_library_ui_view game_library_ui;
    about_ui_view about_ui_view;

    ui_main.add_tool_view(&ps2);
    ui_main.add_tool_view(&game_library_ui);
    ui_main.add_tool_view(&controller);
    ui_main.add_tool_view(&about_ui_view);

    std::chrono::high_resolution_clock clock;

    auto last_rendered_frame_clock = clock.now();

    int pending_render_frames = 0;
    bool done = false;
    while (!done)
    {
        auto frame_start_time = clock.now();

        controller.new_frame();

        bool render_now = false;
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                done = true;
            }

            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
            {
                done = true;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
            {
                render_now = true;
            }

            controller.handle_event(event);
            ui_host.handle_event(event);
            pending_render_frames = 2; // some events take a frame to play out, so request 2 renders
        }

        ps2.update(controller.get_state());

        // ensure we never render faster than 60hz to keep the app low profile
        using namespace std::chrono_literals;
        if (pending_render_frames > 0 && (render_now || (clock.now() - last_rendered_frame_clock) > 16ms))
        {
            ui_host.start_frame();
            ui_main.render();
            ui_host.end_frame();

            last_rendered_frame_clock = clock.now();
            --pending_render_frames;
        }

        // if we're going faster than 100hz, slow down to keep app low profile
        auto total_frame_time = clock.now() - frame_start_time;
        if (total_frame_time < 10ms)
        {
            std::this_thread::sleep_for(10ms - total_frame_time);
        }
    }

    return 0;
}