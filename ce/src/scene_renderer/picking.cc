#include "picking.h"

#include <algorithm>

#include "gl_api.h"
#include "scene_internal.h"

namespace fallout {
namespace scene_renderer {

using namespace internal;

bool pickStaticGroundTile(int logicalX, int logicalY, GroundPickResult* result)
{
    if (!gInitialized || !gHeroProjectionLoaded || gWindow == nullptr || result == nullptr) return false;
    SDL_GL_GetDrawableSize(gWindow, &gWindowWidth, &gWindowHeight);
    int windowWidth = 0, windowHeight = 0;
    SDL_GetWindowSize(gWindow, &windowWidth, &windowHeight);
    const GroundPickProjection projection = {
        { gHeroProjection.squ[0], gHeroProjection.squ[1] },
        { gHeroProjection.sqv[0], gHeroProjection.sqv[1] },
        { gHeroProjection.origin[0], gHeroProjection.origin[1] },
    };
    const StaticPreviewCamera staticCamera = staticPreviewCamera(true);
    const GroundPickCamera camera = {
        { staticCamera.eye[0], staticCamera.eye[1], staticCamera.eye[2] },
        { staticCamera.target[0], staticCamera.target[1], staticCamera.target[2] }, staticCamera.fovY,
        staticCamera.donorScreenRight,
    };
    const GroundPickViewport viewport = { gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, windowWidth, windowHeight };
    GameView view;
    if (!currentGameView(&view)) return pickGroundTile(projection, camera, viewport, logicalX, logicalY, result);

    double worldX, worldZ;
    if (logicalX < 0 || logicalY < 0 || logicalX >= gLogicalWidth || logicalY >= gLogicalHeight
        || !gameViewPickGround(view, logicalX + 0.5, logicalY + 0.5, &worldX, &worldZ)) {
        return false;
    }
    const double u = worldX + 50.0;
    const double v = worldZ + 50.0;
    // This preview is the exported 100x100 Kladwtwn square map only.
    if (u < 0.0 || u >= 100.0 || v < 0.0 || v >= 100.0) return false;
    const double artX = projection.origin[0] + u * projection.squ[0] + v * projection.sqv[0];
    const double artY = projection.origin[1] + u * projection.squ[1] + v * projection.sqv[1];
    const int tile = donorHexFromPixel(artX, artY);
    if (tile < 0) return false;
    *result = { tile, worldX, worldZ, u, v, artX, artY };
    return true;
}

bool staticScreenFromGamePixel(double pixelX, double pixelY, double* logicalX, double* logicalY)
{
    GameView view;
    return currentGameView(&view) && gameViewProjectGroundPixel(view, pixelX, pixelY, logicalX, logicalY);
}

int pickStaticObjects(int logicalX, int logicalY, bool (*wanted)(int pickId, void* context), void* context,
    int* pickIds, int capacity)
{
    if (!gInitialized || !gFloorLoaded || !gWallsLoaded || pickIds == nullptr || capacity <= 0) return 0;
    if (logicalX < 0 || logicalY < 0 || logicalX >= gLogicalWidth || logicalY >= gLogicalHeight) return 0;
    GameView view;
    float mvp[16];
    if (!currentGameView(&view) || !gameViewMatrix(view, mvp)) return 0;
    SDL_GL_GetDrawableSize(gWindow, &gWindowWidth, &gWindowHeight);
    int x, y, width, height;
    if (!fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &x, &y, &width, &height)) return 0;
    const int pixelX = x + static_cast<int>((logicalX + 0.5) * width / gLogicalWidth);
    const int pixelY = gWindowHeight - 1 - (y + static_cast<int>((logicalY + 0.5) * height / gLogicalHeight));

    // One pixel of the back buffer between frames (the next frame clears it
    // whole): the cards as drawn, each in its id's colour. Peeling off the
    // nearest and drawing again lists what lies under it, like the engine's
    // intersect list.
    constexpr unsigned int kScissorTest = 0x0C11;
    constexpr unsigned int kDither = 0x0BD0;
    glViewport(x, y, width, height);
    glEnable(kScissorTest);
    glScissor(pixelX, pixelY, 1, 1);
    glDisable(kDither);
    glDisable(kBlend);
    glDisable(kCullFace);
    glEnable(kDepthTest);
    glDepthFunc(kLess);
    glUseProgram(gSceneProgram);
    glUniformMatrix4fv(gSceneMvpLocation, 1, 0, mvp);
    glUniform1i(gSceneTexturedLocation, 1);
    glUniform1i(gSceneTranslucentLocation, 0);
    glUniform1i(gScenePickLocation, 1);
    glUniform1f(gSceneCutoffLocation, 0.5f);
    glActiveTexture(kTexture0);
    glUniform1i(gSceneTextureLocation, 0);

    int count = 0;
    auto shown = [&](int id) {
        return id >= 0 && std::find(pickIds, pickIds + count, id) == pickIds + count
            && (wanted == nullptr || wanted(id, context));
    };
    auto paint = [&](int id) {
        const unsigned int code = static_cast<unsigned int>(id) + 1;
        glUniform4f(gSceneColorLocation, (code & 0xFF) / 255.0f, ((code >> 8) & 0xFF) / 255.0f,
            ((code >> 16) & 0xFF) / 255.0f, 1.0f);
    };
    while (count < capacity) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glDepthMask(1);
        glClear(kColorBufferBit | kDepthBufferBit);
        if (!gObjectDraws.empty() && gObjectVao != 0) {
            glBindVertexArray(gObjectVao);
            // Flat things lie under the rest; the rest hide by their rows' depth.
            glUniform1i(gSceneDepthAtLocation, 0);
            glDepthMask(0);
            for (const ObjectDraw& draw : gObjectDraws) {
                if (!draw.flat || !shown(draw.pickId)) continue;
                paint(draw.pickId);
                glBindTexture(kTexture2D, draw.texture);
                glDrawArrays(kTriangles, draw.firstVertex, 6);
            }
            glDepthMask(1);
            glUniform1i(gSceneDepthAtLocation, 1);
            for (const ObjectDraw& draw : gObjectDraws) {
                if (draw.flat || !shown(draw.pickId)) continue;
                paint(draw.pickId);
                glBindTexture(kTexture2D, draw.texture);
                glDrawArrays(kTriangles, draw.firstVertex, 6);
            }
        }
        if (gHeroLoaded && shown(gHeroPickId)) {
            glUniform1i(gSceneDepthAtLocation, 1);
            glBindVertexArray(gHeroVao);
            paint(gHeroPickId);
            glBindTexture(kTexture2D, gHeroTexture);
            glDrawArrays(kTriangles, 0, 6);
        }
        unsigned char pixel[4] = {};
        glPixelStorei(kPackAlignment, 1);
        glReadPixels(pixelX, pixelY, 1, 1, kRGBA, kUnsignedByte, pixel);
        glPixelStorei(kPackAlignment, kDefaultPackAlignment);
        const int code = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
        if (code == 0) break;
        pickIds[count++] = code - 1;
    }
    glUniform1i(gScenePickLocation, 0);
    glUniform1i(gSceneDepthAtLocation, 0);
    glDisable(kScissorTest);
    glDisable(kDepthTest);
    glDepthMask(1);
    glBindVertexArray(0);
    glUseProgram(0);
    return count;
}

} // namespace scene_renderer
} // namespace fallout
