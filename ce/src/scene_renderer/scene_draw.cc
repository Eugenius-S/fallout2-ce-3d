#include "scene_draw.h"

#include <algorithm>
#include <stdio.h>
#include <vector>

#include <lodepng.h>

#include "context.h"
#include "gl_api.h"
#include "post.h"
#include "../scene/scene_overlay.h"
#include "text_present.h"
#include "scene_internal.h"
#include "shadow.h"
#include "static_scene.h"

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
    const bool flying = withWalls && flyCameraMvp(static_cast<float>(width) / height, mvp);
    if (!flying && (!withWalls || !currentGameView(&view) || !gameViewMatrix(view, mvp))) {
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
    bindStaticMapClipUniforms();
    glUniformMatrix4fv(gSceneMvpLocation, 1, 0, mvp);
    glUniform1i(gSceneDepthAtLocation, 0);
    glUniform3f(gSceneEyeLocation, camera.eye[0], camera.eye[1], camera.eye[2]);
    glUniform1i(gSceneTranslucentLocation, 0);
    glUniform1i(gScenePickLocation, 0);
    glUniform1i(gSceneSeeThroughLocation, 0);
    glUniform1i(gSceneEggPassLocation, 0);
    // The game shows the hero through the walls and scenery he stands behind,
    // inside its egg: they are drawn without the egg's part first, and that
    // part blended over the hero and what lies behind at the end.
    const bool egg = withHero && bindEgg(x, y, width, height);
    const bool wallEgg = egg && withWalls && gWallSeeThroughAny && gWallSeeThroughTexture != 0;
    // The game's light as point lights: the floor and walls at each fragment,
    // sprites at their own hex, roofs by the ambient level.
    const bool lit = withHero && bindLight();
    // The sun's shadow map renders before the scene; the floor and the walls
    // sample it (uShadowOn). It restores the framebuffer and the viewport.
    const bool shadowed = withWalls && bindStaticShadows(mvp);
    glUniform1i(gSceneShadowOnLocation, shadowed ? 1 : 0);
    glUniform1i(gSceneLightModeLocation, 0);
    auto lightSprite = [&](int tile) {
        // tile -1: painted at full light (OBJECT_TRANS_WALL), or not known.
        glUniform1i(gSceneLightModeLocation, lit && tile >= 0 ? 3 : 0);
        if (lit && tile >= 0) setLightAnchor(tile);
    };
    if (withWalls && gWallSeeThroughTexture != 0 && (wallEgg || lit)) {
        flushWallObjectTable();
        glActiveTexture(kTexture0 + 2);
        glBindTexture(kTexture2D, gWallSeeThroughTexture);
        glUniform1i(gSceneSeeThroughObjectsLocation, 2);
    }
    glActiveTexture(kTexture0);
    glUniform1i(gSceneTextureLocation, 0);
    glBindVertexArray(gFloorVao);
    glBindTexture(kTexture2D, gFloorTexture);

    // Nothing lies below the floor, so it is painted first and hides nothing
    // (the donor's game view): cards and solids take the depth of their hex
    // row's plane, which can lie under the floor along the view ray. A fly
    // camera takes each vertex's own depth, so there the floor hides what
    // hangs below the ground: a wall picture's foot reaches under it (071).
    glDepthMask(0);
    glUniform1i(gSceneTexturedLocation, 0);
    glUniform1f(gSceneCutoffLocation, 0.01f);
    glUniform4f(gSceneColorLocation, 0.0f, 0.0f, 0.0f, 1.0f);
    glDrawArrays(kTriangles, 0, 6);
    glDepthMask(flying ? 1 : 0);
    glUniform1i(gSceneTexturedLocation, 1);
    glUniform1f(gSceneCutoffLocation, 0.01f);
    glUniform1i(gSceneLightModeLocation, lit ? 1 : 0);
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
            lightSprite(draw.tile);
            glBindTexture(kTexture2D, draw.texture);
            glDrawArrays(kTriangles, draw.firstVertex, 6);
        }
        glDepthMask(1);
    }

    // Depth anchors are made for the game camera: a fly camera looks from
    // anywhere and takes each vertex's own depth.
    glUniform1i(gSceneDepthAtLocation, flying ? 0 : 1);
    if (withWalls) {
        // Wall faces are seen from outside only, as in the donor's game view.
        glEnable(kCullFace);
        glUniform1f(gSceneCutoffLocation, 0.5f);
        glUniform1i(gSceneSeeThroughLocation, wallEgg ? 1 : 0);
        glUniform1i(gSceneEggPassLocation, wallEgg ? 1 : 0);
        glUniform1i(gSceneLightModeLocation, lit ? 2 : 0);
        for (const WallBatch& batch : gWallBatches) {
            if (batch.flyOnly ? !flying : batch.gameOnly && flying) continue;
            glBindVertexArray(batch.vao);
            glBindTexture(kTexture2D, batch.texture);
            glDrawArrays(kTriangles, 0, static_cast<int>(batch.triangleCount * 3));
        }
        glDisable(kCullFace);
    }
    if (withObjects) {
        glUniform1f(gSceneCutoffLocation, 0.5f);
        glUniform1i(gSceneEggPassLocation, egg ? 1 : 0);
        glBindVertexArray(gObjectVao);
        for (const ObjectDraw& draw : gObjectDraws) {
            if (draw.pickOnly || draw.flat || draw.tint[0] >= 0.0f) continue;
            glUniform1i(gSceneSeeThroughLocation, egg && draw.seeThrough ? 2 : 0);
            lightSprite(draw.tile);
            glBindTexture(kTexture2D, draw.texture);
            glDrawArrays(kTriangles, draw.firstVertex, 6);
        }
    }
    glUniform1i(gSceneSeeThroughLocation, 0);
    glUniform1i(gSceneEggPassLocation, 0);
    if (withHero && gHeroLoaded) {
        StaticHeroInfo hero;
        lightSprite(staticHeroInfo(&hero) ? hero.tile : -1);
        glUniform1f(gSceneCutoffLocation, 0.5f);
        glBindVertexArray(gHeroVao);
        glBindTexture(kTexture2D, gHeroTexture);
        glDrawArrays(kTriangles, 0, 6);
    }
    if (egg) {
        glEnable(kBlend);
        glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
        glDepthMask(0);
        glUniform1i(gSceneEggPassLocation, 2);
        glUniform1f(gSceneCutoffLocation, 0.5f);
        if (wallEgg) {
            glEnable(kCullFace);
            glUniform1i(gSceneSeeThroughLocation, 1);
            glUniform1i(gSceneLightModeLocation, lit ? 2 : 0);
            for (const WallBatch& batch : gWallBatches) {
                if (batch.flyOnly ? !flying : batch.gameOnly && flying) continue;
                glBindVertexArray(batch.vao);
                glBindTexture(kTexture2D, batch.texture);
                glDrawArrays(kTriangles, 0, static_cast<int>(batch.triangleCount * 3));
            }
            glDisable(kCullFace);
        }
        if (withObjects) {
            glUniform1i(gSceneSeeThroughLocation, 2);
            glBindVertexArray(gObjectVao);
            for (const ObjectDraw& draw : gObjectDraws) {
                if (draw.pickOnly || draw.flat || draw.tint[0] >= 0.0f || !draw.seeThrough) continue;
                lightSprite(draw.tile);
                glBindTexture(kTexture2D, draw.texture);
                glDrawArrays(kTriangles, draw.firstVertex, 6);
            }
        }
        glUniform1i(gSceneSeeThroughLocation, 0);
        glUniform1i(gSceneEggPassLocation, 0);
        glDepthMask(1);
        glDisable(kBlend);
    }
    if (withObjects) {
        glEnable(kBlend);
        // The shader gives the blend with its light applied (premultiplied).
        glBlendFunc(kOne, kOneMinusSrcAlpha);
        glDepthMask(0);
        glUniform1i(gSceneTranslucentLocation, 1);
        glUniform1f(gSceneCutoffLocation, 0.5f);
        glBindVertexArray(gObjectVao);
        for (const ObjectDraw& draw : gObjectDraws) {
            if (draw.pickOnly || draw.tint[0] < 0.0f) continue;
            lightSprite(draw.tile);
            glUniform3f(gSceneTintLocation, draw.tint[0], draw.tint[1], draw.tint[2]);
            glUniform1i(gSceneGlassGrayLocation, draw.glassGray ? 1 : 0);
            glBindTexture(kTexture2D, draw.texture);
            glDrawArrays(kTriangles, draw.firstVertex, 6);
        }
        glUniform1i(gSceneTranslucentLocation, 0);
        glDepthMask(1);
        glDisable(kBlend);
    }
    glUniform1i(gSceneLightModeLocation, lit ? 4 : 0);
    glUniform1f(gSceneLightValueLocation, lightAmbient());
    if (withHero && staticSoloKeeps(-1)) drawRoofs(x, y, width, height); // solo: no roofs
    glUniform1i(gSceneLightModeLocation, 0);
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
    // Reuse the prepared sharp pass from the same frame shown live.
    const PreparedSharpTextFrame& text = preparedSharpTextFrame();
    const bool sharpReady = text.active && text.width == frameWidth && text.height == frameHeight
        && uploadSharpTextOverlay(text.overlay.pixels.data(), text.overlay.width,
            text.overlay.height, text.overlay.pitch);
    if (text.active && !sharpReady) disableSharpText("OpenGL upload failed during frame capture");
    // The next present clears and redraws the back buffer; nothing is swapped.
    presentFrame(sharpReady ? text.restoredRgba.data() : rgba,
        frameWidth, frameHeight, sharpReady ? text.rgbaPitch : pitch);
    if (sharpReady && !drawPreparedSharpTextOverlay()) {
        disableSharpText("OpenGL draw failed during frame capture");
        presentFrame(rgba, frameWidth, frameHeight, pitch);
    }
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
    int* width, int* height, std::string* error, bool includeOverlay)
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
    const bool glow = beginSceneCapture();
    drawStaticPreview(withWalls, withHero);
    if (glow) compositeGlow();
    if (includeOverlay && withHero && gSceneOverlayReady && !staticFlyCameraActive()) {
        // With what the game draws over it, as presented.
        glViewport(0, 0, drawableWidth, drawableHeight);
        glEnable(kBlend);
        glBlendFunc(1 /* GL_ONE */, kOneMinusSrcAlpha);
        drawQuad(gTextureWidth, gTextureHeight, gTextureWidth * 4, nullptr, 0, 0, gLogicalWidth, gLogicalHeight);
        glDisable(kBlend);
        const PreparedSharpTextFrame& text = preparedSharpTextFrame();
        if (text.active) {
            const bool uploaded = uploadSharpTextOverlay(text.overlay.pixels.data(), text.overlay.width,
                text.overlay.height, text.overlay.pitch);
            if (!uploaded) disableSharpText("OpenGL upload failed during static preview capture");
            if (uploaded && drawPreparedSharpTextOverlay()) {
                // The sharp pass completed over the cached game overlay.
            } else {
                if (uploaded) disableSharpText("OpenGL draw failed during static preview capture");
                const bool fallbackGlow = beginSceneCapture();
                drawStaticPreview(withWalls, withHero);
                if (fallbackGlow) compositeGlow();
                if (withHero && gSceneOverlayReady) {
                    std::vector<unsigned char> fallback = text.originalRgba;
                    scene::maskOverlay(text.originalIndices.data(), text.indexPitch,
                        fallback.data(), text.rgbaPitch, text.width, text.height);
                    drawSceneOverlay(fallback.data(), text.width, text.height, text.rgbaPitch);
                }
            }
        }
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
