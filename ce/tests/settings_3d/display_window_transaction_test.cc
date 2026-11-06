#include "../../src/scene/display_window_transaction.h"

#include <cstdio>

using fallout::WindowMode;
using fallout::scene::DisplayWindowTransaction;
using Result = DisplayWindowTransaction::Result;

static bool sizeIs(SDL_Window* window, int width, int height)
{
    int actualWidth = 0, actualHeight = 0;
    SDL_GetWindowSize(window, &actualWidth, &actualHeight);
    return actualWidth == width && actualHeight == height;
}

int main()
{
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { std::fprintf(stderr, "%s\n", SDL_GetError()); return 1; }
    SDL_Window* window = SDL_CreateWindow("test", 10, 10, 800, 600, SDL_WINDOW_HIDDEN);
    if (window == nullptr) return 2;
    {
        DisplayWindowTransaction transaction(window);
        if (transaction.apply(WindowMode::Windowed, 639, 480) != Result::RejectedRestored || !sizeIs(window, 800, 600)) return 3;
        if (transaction.apply(WindowMode::Windowed, 1024, 768) != Result::Applied || !sizeIs(window, 1024, 768)) return 4;
        if (transaction.apply(WindowMode::Windowed, 800, 600) != Result::InvalidState) return 13;
        if (!transaction.rollback() || !sizeIs(window, 800, 600)) return 5;
    }
    {
        DisplayWindowTransaction transaction(window);
        if (transaction.apply(WindowMode::Windowed, 1024, 768) != Result::Applied) return 6;
        transaction.commit();
    }
    if (!sizeIs(window, 1024, 768)) return 7;
    {
        DisplayWindowTransaction transaction(window);
        if (transaction.apply(WindowMode::Windowed, 800, 600) != Result::Applied) return 8;
    }
    if (!sizeIs(window, 1024, 768)) return 9;
    {
        DisplayWindowTransaction transaction(window);
        if (transaction.apply(WindowMode::WindowedFullscreen, 1024, 768) != Result::Applied) return 10;
        if ((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != SDL_WINDOW_FULLSCREEN_DESKTOP) return 11;
        if (!transaction.rollback() || !sizeIs(window, 1024, 768)) return 12;
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
