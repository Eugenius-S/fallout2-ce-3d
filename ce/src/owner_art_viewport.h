#ifndef FALLOUT_OWNER_ART_VIEWPORT_H_
#define FALLOUT_OWNER_ART_VIEWPORT_H_

namespace fallout {

struct OwnerArtViewport {
    int x;
    int y;
    int width;
    int height;
};

// Owner full-screen art supports screens up to 8:3. Wider displays keep a
// centered 8:3 content area, with the remaining surface left as side bars.
static inline OwnerArtViewport ownerArtViewport(int screenWidth, int screenHeight)
{
    OwnerArtViewport viewport { 0, 0, screenWidth, screenHeight };
    if (screenWidth > 0 && screenHeight > 0
        && static_cast<long long>(screenWidth) * 3 > static_cast<long long>(screenHeight) * 8) {
        viewport.width = static_cast<int>(static_cast<long long>(screenHeight) * 8 / 3);
        viewport.x = (screenWidth - viewport.width) / 2;
    }
    return viewport;
}

} // namespace fallout

#endif // FALLOUT_OWNER_ART_VIEWPORT_H_
