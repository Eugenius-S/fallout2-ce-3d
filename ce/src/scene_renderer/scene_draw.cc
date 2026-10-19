#include "scene_draw.h"

#include <algorithm>

#include <lodepng.h>

#include "context.h"
#include "gl_api.h"
#include "scene_internal.h"

namespace fallout {
namespace scene_renderer {

using namespace internal;

namespace {

void drawStaticPreview(bool withWalls, bool withHero)
{
    if (!gInitialized || !gFloorLoaded || (withWalls && !gWallsLoaded)) return;
    SDL_GL_GetDrawableSize(gWindow, &gWindowWidth, &gWindowHeight);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    // The engine shows black where there is no floor and beyond the map.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(kColorBufferBit | kDepthBufferBit);
    int x, y, width, height;
    if (!fitRect(gLogicalWidth, gLogicalHeight, gWindowWidth, gWindowHeight, &x, &y, &width, &height)) return;
    glViewport(x, y, width, height);

    float mvp[16];
    const StaticPreviewCamera camera = staticPreviewCamera(withWalls);
    GameView view;
    if (!withWalls || !currentGameView(&view) || !gameViewMatrix(view, mvp)) {
        makeFloorMvp(static_cast<float>(width) / height, camera, mvp);
    }
    glEnable(kDepthTest);
    glDepthFunc(kLess);
    glDepthMask(1);
    glDisable(kCullFace);
    if (withWalls) {
        glDisable(kBlend);
    } else {
        glEnable(kBlend);
        glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
    }
    glUseProgram(gSceneProgram);
    glUniformMatrix4fv(gSceneMvpLocation, 1, 0, mvp);
    glUniform1i(gSceneDepthAtLocation, 0);
    glUniform1i(gSceneTranslucentLocation, 0);
    glUniform1i(gScenePickLocation, 0);
    glActiveTexture(kTexture0);
    glUniform1i(gSceneTextureLocation, 0);
    glBindVertexArray(gFloorVao);
    glBindTexture(kTexture2D, gFloorTexture);

    // Nothing lies below the floor, so it is painted first and hides nothing
    // (the donor's game view): cards and solids take the depth of their hex
    // row's plane, which can lie under the floor along the view ray.
    glDepthMask(0);
    glUniform1i(gSceneTexturedLocation, 0);
    glUniform1f(gSceneCutoffLocation, 0.01f);
    glUniform4f(gSceneColorLocation, 0.0f, 0.0f, 0.0f, 1.0f);
    glDrawArrays(kTriangles, 0, 6);
    glUniform1i(gSceneTexturedLocation, 1);
    glUniform1f(gSceneCutoffLocation, 0.01f);
    glDrawArrays(kTriangles, 6, gFloorVertexCount - 6);
    glDepthMask(1);

    const bool withObjects = withHero && !gObjectDraws.empty() && gObjectVao != 0;
    if (withObjects) {
        // Flat objects lie on the floor in the engine's draw order: later ones
        // paint over earlier ones, and they hide nothing drawn after them.
        glDepthMask(0);
        glUniform1f(gSceneCutoffLocation, 0.5f);
        glBindVertexArray(gObjectVao);
        for (const ObjectDraw& draw : gObjectDraws) {
            if (draw.pickOnly || !draw.flat || draw.tint[0] >= 0.0f) continue;
            glBindTexture(kTexture2D, draw.texture);
            glDrawArrays(kTriangles, draw.firstVertex, 6);
        }
        glDepthMask(1);
    }

    glUniform1i(gSceneDepthAtLocation, 1);
    if (withWalls) {
        // Wall faces are seen from outside only, as in the donor's game view.
        glEnable(kCullFace);
        glUniform1f(gSceneCutoffLocation, 0.5f);
        for (const WallBatch& batch : gWallBatches) {
            glBindVertexArray(batch.vao);
            glBindTexture(kTexture2D, batch.texture);
            glDrawArrays(kTriangles, 0, static_cast<int>(batch.triangleCount * 3));
        }
        glDisable(kCullFace);
    }
    if (withObjects) {
        glUniform1f(gSceneCutoffLocation, 0.5f);
        glBindVertexArray(gObjectVao);
        for (const ObjectDraw& draw : gObjectDraws) {
            if (draw.pickOnly || draw.flat || draw.tint[0] >= 0.0f) continue;
            glBindTexture(kTexture2D, draw.texture);
            glDrawArrays(kTriangles, draw.firstVertex, 6);
        }
    }
    if (withHero && gHeroLoaded) {
        glUniform1f(gSceneCutoffLocation, 0.5f);
        glBindVertexArray(gHeroVao);
        glBindTexture(kTexture2D, gHeroTexture);
        glDrawArrays(kTriangles, 0, 6);
    }
    if (withObjects) {
        glEnable(kBlend);
        glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
        glDepthMask(0);
        glUniform1i(gSceneTranslucentLocation, 1);
        glUniform1f(gSceneCutoffLocation, 0.5f);
        glBindVertexArray(gObjectVao);
        for (const ObjectDraw& draw : gObjectDraws) {
            if (draw.pickOnly || draw.tint[0] < 0.0f) continue;
            glUniform3f(gSceneTintLocation, draw.tint[0], draw.tint[1], draw.tint[2]);
            glUniform1i(gSceneGlassGrayLocation, draw.glassGray ? 1 : 0);
            glBindTexture(kTexture2D, draw.texture);
            glDrawArrays(kTriangles, draw.firstVertex, 6);
        }
        glUniform1i(gSceneTranslucentLocation, 0);
        glDepthMask(1);
        glDisable(kBlend);
    }
    if (withHero) drawRoofs(x, y, width, height);
    glUniform1i(gSceneDepthAtLocation, 0);

    glDisable(kBlend);
    glDisable(kDepthTest);
    glDepthMask(1);
    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace

void drawStaticFloor()
{
    drawStaticPreview(false, false);
}

void drawStaticWallPreview()
{
    drawStaticPreview(true, false);
}

void drawStaticHeroPreview()
{
    drawStaticPreview(true, true);
}

namespace {

// The back buffer as drawn so far into a new PNG (top-down rows).
bool writeBackBufferPng(const std::filesystem::path& path, int drawableWidth, int drawableHeight, std::string* error)
{
    const size_t rowBytes = static_cast<size_t>(drawableWidth) * 4;
    std::vector<unsigned char> bottomUp(rowBytes * static_cast<size_t>(drawableHeight));
    std::vector<unsigned char> topDown(bottomUp.size());
    glPixelStorei(kPackAlignment, 1);
    glReadPixels(0, 0, drawableWidth, drawableHeight, kRGBA, kUnsignedByte, bottomUp.data());
    glPixelStorei(kPackAlignment, kDefaultPackAlignment);
    if (glGetError() != 0) {
        *error = "OpenGL backbuffer read failed";
        return false;
    }
    for (int row = 0; row < drawableHeight; row++) {
        const unsigned char* source = bottomUp.data() + rowBytes * static_cast<size_t>(drawableHeight - 1 - row);
        unsigned char* destination = topDown.data() + rowBytes * static_cast<size_t>(row);
        std::copy(source, source + rowBytes, destination);
    }
    const unsigned encodeError = lodepng::encode(path.string(), topDown,
        static_cast<unsigned>(drawableWidth), static_cast<unsigned>(drawableHeight), LCT_RGBA, 8);
    if (encodeError != 0) {
        std::error_code ec;
        std::filesystem::remove(path, ec); // Remove only output this call may have partially written.
        *error = std::string("PNG encode failed: ") + lodepng_error_text(encodeError);
        return false;
    }
    return true;
}

} // namespace

bool captureFramePng(const std::filesystem::path& path, const unsigned char* rgba, int frameWidth, int frameHeight,
    int pitch, int* width, int* height, std::string* error)
{
    auto fail = [&](const std::string& message) {
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized || rgba == nullptr) return fail("OpenGL renderer is unavailable");
    std::error_code existsError;
    if (std::filesystem::exists(path, existsError)) return fail("snapshot path already exists");
    int drawableWidth = 0;
    int drawableHeight = 0;
    SDL_GL_GetDrawableSize(gWindow, &drawableWidth, &drawableHeight);
    if (drawableWidth <= 0 || drawableHeight <= 0) return fail("OpenGL drawable has no pixels");
    while (glGetError() != 0) {
    }
    // The next present clears and redraws the back buffer; nothing is swapped.
    presentFrame(rgba, frameWidth, frameHeight, pitch);
    std::string message;
    if (!writeBackBufferPng(path, drawableWidth, drawableHeight, &message)) return fail(message);
    if (width != nullptr) *width = drawableWidth;
    if (height != nullptr) *height = drawableHeight;
    if (error != nullptr) error->clear();
    return true;
}

bool captureStaticFloorPng(const std::filesystem::path& path, int* width, int* height, std::string* error)
{
    return captureStaticPreviewPng(path, false, false, width, height, error);
}

bool captureStaticPreviewPng(const std::filesystem::path& path, bool withWalls, bool withHero,
    int* width, int* height, std::string* error)
{
    auto fail = [&](const std::string& message) {
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized || !gFloorLoaded) return fail("static floor preview is not loaded");
    if (withWalls && !gWallsLoaded) return fail("static wall preview is not loaded");
    if (withHero && !gHeroLoaded) return fail("current hero frame is unavailable");
    std::error_code existsError;
    if (std::filesystem::exists(path, existsError)) return fail("snapshot path already exists");
    if (existsError) return fail("cannot inspect snapshot path");

    int drawableWidth = 0;
    int drawableHeight = 0;
    SDL_GL_GetDrawableSize(gWindow, &drawableWidth, &drawableHeight);
    if (drawableWidth <= 0 || drawableHeight <= 0) return fail("OpenGL drawable has no pixels");

    // Read only our preview render. Do not swap or touch game/map state.
    while (glGetError() != 0) {
    }
    drawStaticPreview(withWalls, withHero);
    if (withHero && gSceneOverlayReady) {
        // With what the game draws over it, as presented.
        glViewport(0, 0, drawableWidth, drawableHeight);
        glEnable(kBlend);
        glBlendFunc(1 /* GL_ONE */, kOneMinusSrcAlpha);
        drawQuad(gTextureWidth, gTextureHeight, gTextureWidth * 4, nullptr, 0, 0, gLogicalWidth, gLogicalHeight);
        glDisable(kBlend);
    }
    if (glGetError() != 0) return fail(withWalls
        ? "static wall draw failed before snapshot" : "static floor draw failed before snapshot");
    std::string message;
    if (!writeBackBufferPng(path, drawableWidth, drawableHeight, &message)) return fail(message);
    if (width != nullptr) *width = drawableWidth;
    if (height != nullptr) *height = drawableHeight;
    if (error != nullptr) error->clear();
    return true;
}

} // namespace scene_renderer
} // namespace fallout
