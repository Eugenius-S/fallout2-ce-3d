#include "../../src/scene/display_surface_swap.h"

#include <cstddef>

namespace fallout {
int registered = 0, unregistered = 0;
void textCaptureRegisterExternalBuffer(void*, std::size_t, int, int, int, bool) { ++registered; }
void textCaptureUnregisterExternalBuffer(void*) { ++unregistered; }
}

int main()
{
    using namespace fallout;
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    SDL_Surface* indexed = SDL_CreateRGBSurface(0, 800, 600, 8, 0, 0, 0, 0);
    SDL_Surface* rgba = SDL_CreateRGBSurfaceWithFormat(0, 800, 600, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_Texture* texture = nullptr;
    if (indexed == nullptr || rgba == nullptr) return 2;
    SDL_Color color = { 123, 45, 67, 255 };
    SDL_SetPaletteColors(indexed->format->palette, &color, 7, 1);
    SDL_Surface* original = indexed;
    {
        scene::DisplaySurfaceSwap state;
        if (state.prepare(639, 480, indexed, nullptr)) return 3;
        if (!state.prepare(1280, 720, indexed, nullptr)) return 4;
        if (!state.exchange(indexed, rgba, texture, nullptr) || indexed == original || indexed->w != 1280 || rgba->h != 720) return 5;
        const SDL_Color copied = indexed->format->palette->colors[7];
        if (copied.r != color.r || copied.g != color.g || copied.b != color.b || registered != 1) return 6;
        if (!state.exchange(indexed, rgba, texture, nullptr) || indexed != original || indexed->w != 800 || unregistered != 0) return 7;
    }
    if (unregistered != 1) return 8;
    {
        scene::DisplaySurfaceSwap state;
        if (!state.prepare(1024, 768, indexed, nullptr) || !state.exchange(indexed, rgba, texture, nullptr)) return 15;
    }
    if (indexed == original || indexed->w != 1024 || unregistered != 2) return 16;
    SDL_FreeSurface(indexed);
    SDL_FreeSurface(rgba);
    SDL_Window* window = SDL_CreateWindow("test", 0, 0, 800, 600, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = window != nullptr ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE) : nullptr;
    if (renderer == nullptr) return 9;
    indexed = SDL_CreateRGBSurface(0, 800, 600, 8, 0, 0, 0, 0);
    rgba = SDL_CreateRGBSurfaceWithFormat(0, 800, 600, 32, SDL_PIXELFORMAT_RGB888);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 800, 600);
    if (indexed == nullptr || rgba == nullptr || texture == nullptr || SDL_RenderSetLogicalSize(renderer, 800, 600) != 0) return 10;
    original = indexed;
    {
        scene::DisplaySurfaceSwap state;
        if (!state.prepare(1280, 720, indexed, renderer) || !state.exchange(indexed, rgba, texture, renderer)) return 11;
        int width = 0, height = 0;
        SDL_RenderGetLogicalSize(renderer, &width, &height);
        if (width != 1280 || height != 720 || indexed == original) return 12;
        if (!state.exchange(indexed, rgba, texture, renderer)) return 13;
        SDL_RenderGetLogicalSize(renderer, &width, &height);
        if (width != 800 || height != 600 || indexed != original) return 14;
    }
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(indexed);
    SDL_FreeSurface(rgba);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
