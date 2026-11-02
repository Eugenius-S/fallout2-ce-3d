#include "display_surface_swap.h"

#include <utility>

#include "text_capture.h"

namespace fallout::scene {

DisplaySurfaceSwap::~DisplaySurfaceSwap()
{
    if (indexed_ != nullptr) {
        if (exchanged_) textCaptureUnregisterExternalBuffer(indexed_->pixels);
        SDL_FreeSurface(indexed_);
    }
    if (rgba_ != nullptr) SDL_FreeSurface(rgba_);
    if (texture_ != nullptr) SDL_DestroyTexture(texture_);
}

bool DisplaySurfaceSwap::prepare(int width, int height, SDL_Surface* oldIndexed, SDL_Renderer* renderer)
{
    if (indexed_ != nullptr || width < 640 || height < 480 || width > 7680 || height > 4320
        || oldIndexed == nullptr || oldIndexed->format == nullptr || oldIndexed->format->palette == nullptr) return false;
    indexed_ = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
    if (indexed_ == nullptr || indexed_->format->palette == nullptr) return false;
    const SDL_Palette* palette = oldIndexed->format->palette;
    if (SDL_SetPaletteColors(indexed_->format->palette, palette->colors, 0, palette->ncolors) != 0) return false;
    if (SDL_FillRect(indexed_, nullptr, 0) != 0) return false;
    renderer_ = renderer;
    const Uint32 format = renderer != nullptr ? SDL_PIXELFORMAT_RGB888 : SDL_PIXELFORMAT_RGBA32;
    rgba_ = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, format);
    if (rgba_ == nullptr) return false;
    if (SDL_FillRect(rgba_, nullptr, 0) != 0) return false;
    if (renderer != nullptr) {
        texture_ = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (texture_ == nullptr) return false;
    }
    return true;
}

bool DisplaySurfaceSwap::exchange(SDL_Surface*& indexed, SDL_Surface*& rgba, SDL_Texture*& texture, SDL_Renderer* renderer)
{
    if (indexed_ == nullptr || rgba_ == nullptr || indexed == nullptr || rgba == nullptr || renderer != renderer_
        || (renderer != nullptr && (texture_ == nullptr || texture == nullptr))
        || (renderer == nullptr && (texture_ != nullptr || texture != nullptr))) return false;
    if (renderer != nullptr && SDL_RenderSetLogicalSize(renderer, indexed_->w, indexed_->h) != 0) return false;
    if (!exchanged_) {
        textCaptureRegisterExternalBuffer(indexed_->pixels, static_cast<size_t>(indexed_->pitch) * indexed_->h,
            indexed_->w, indexed_->h, indexed_->pitch, false);
        exchanged_ = true;
    }
    std::swap(indexed, indexed_);
    std::swap(rgba, rgba_);
    std::swap(texture, texture_);
    return true;
}

} // namespace fallout::scene
