#ifndef FALLOUT_SCENE_DISPLAY_SURFACE_SWAP_H
#define FALLOUT_SCENE_DISPLAY_SURFACE_SWAP_H

#include <SDL.h>

namespace fallout::scene {

// Prepare the 8-bit game surface, palette-expanded surface and (on SDL's
// renderer path) texture before replacing any live pointers. Exchanging twice
// rolls back. The caller must suppress rendering until all aliases are rebound.
class DisplaySurfaceSwap {
public:
    DisplaySurfaceSwap() = default;
    ~DisplaySurfaceSwap();
    DisplaySurfaceSwap(const DisplaySurfaceSwap&) = delete;
    DisplaySurfaceSwap& operator=(const DisplaySurfaceSwap&) = delete;

    bool prepare(int width, int height, SDL_Surface* oldIndexed, SDL_Renderer* renderer);
    bool exchange(SDL_Surface*& indexed, SDL_Surface*& rgba, SDL_Texture*& texture, SDL_Renderer* renderer);

private:
    SDL_Surface* indexed_ = nullptr;
    SDL_Surface* rgba_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    bool exchanged_ = false;
};

} // namespace fallout::scene

#endif
