#pragma once

#include "utils.h"
#include <SDL_events.h>

struct SDL_Window;
class ID3D11ShaderResourceView;
class ID3D11Device;
class ID3D11DeviceContext;
class IDXGISwapChain;
class ID3D11RenderTargetView;

class ui_host : non_copyable<ui_host>
{
    SDL_Window* window = nullptr;
    ID3D11Device* d3d_device = nullptr;
    ID3D11DeviceContext* d3d_device_context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    ID3D11RenderTargetView* main_render_target_view = nullptr;

private:
    bool create_device();
    void cleanup_device();
    void create_render_target();
    void cleanup_render_target();

public:
    ui_host();
    ~ui_host();
    void start_frame();
    void handle_event(const SDL_Event& event);
    void end_frame();
};