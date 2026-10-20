#include "live_objects.h"

#include <cmath>

#include "gl_api.h"
#include "scene_internal.h"
#include "static_scene.h"

namespace fallout {
namespace scene_renderer {

namespace internal {

std::vector<ObjectDraw> gObjectDraws;
unsigned int gObjectVao = 0;
unsigned int gHeroVao = 0;
unsigned int gHeroTexture = 0;
bool gHeroLoaded = false;
int gHeroPickId = -1;

} // namespace internal

using namespace internal;

namespace {

// Live map objects: one textured quad each, rebuilt every presented frame.
// Frame textures are cached by (fid, frame, rotation) across frames.
constexpr size_t kMaxObjectTextures = 4096;
constexpr float kFlatObjectHeight = 0.01f;
std::unordered_map<unsigned long long, unsigned int> gObjectTextures;
std::vector<DepthVertex> gObjectVertices;
unsigned int gObjectVbo = 0;
unsigned int gHeroVbo = 0;
StaticHeroInfo gHeroInfo = {};

unsigned long long objectArtKey(int fid, int frame, int rotation)
{
    return (static_cast<unsigned long long>(static_cast<unsigned int>(fid)) << 32)
        | ((static_cast<unsigned long long>(static_cast<unsigned int>(frame)) & 0xFFFFFF) << 8)
        | (static_cast<unsigned int>(rotation) & 0xFF);
}

void destroyStaticObjectTextures()
{
    if (gContext != nullptr) {
        for (const auto& entry : gObjectTextures) {
            glDeleteTextures(1, &entry.second);
        }
    }
    gObjectTextures.clear();
}

} // namespace

namespace internal {

void faceFlyCamera(SpriteCardVertex corners[4])
{
    double rightX, rightZ;
    if (!flyCameraRight(&rightX, &rightZ)) return;
    // The card's lean cancels out between its top and bottom corners: their
    // middle is the footprint's, and the left edge's middle is half the
    // card's width from it.
    const double middleX = (corners[0].x + corners[1].x + corners[2].x + corners[3].x) / 4.0;
    const double middleZ = (corners[0].z + corners[1].z + corners[2].z + corners[3].z) / 4.0;
    const double leftX = (corners[0].x + corners[1].x) / 2.0 - middleX;
    const double leftZ = (corners[0].z + corners[1].z) / 2.0 - middleZ;
    const double half = std::sqrt(leftX * leftX + leftZ * leftZ);
    for (int index = 0; index < 4; index++) {
        const double side = index < 2 ? -half : half;
        corners[index].x = middleX + rightX * side;
        corners[index].z = middleZ + rightZ * side;
    }
}

// Under the fly camera a door lies on its wall's plane: each corner slides
// along the game camera's ray to it, so that camera would still see it on the
// same pixels.
void onFlyPlane(SpriteCardVertex corners[4], const float plane[3], const float eye[3])
{
    double rightX, rightZ;
    if (!flyCameraRight(&rightX, &rightZ)) return;
    const double normalX = -std::sin(plane[1]);
    const double normalZ = std::cos(plane[1]);
    const double toPlane = normalX * (plane[0] - eye[0]) + normalZ * (plane[2] - eye[2]);
    for (int index = 0; index < 4; index++) {
        const double rayX = corners[index].x - eye[0];
        const double rayY = corners[index].y - eye[1];
        const double rayZ = corners[index].z - eye[2];
        const double along = normalX * rayX + normalZ * rayZ;
        if (std::abs(along) < 1e-9) return;
        const double scale = toPlane / along;
        corners[index].x = eye[0] + rayX * scale;
        corners[index].y = eye[1] + rayY * scale;
        corners[index].z = eye[2] + rayZ * scale;
    }
}

void destroyStaticHero()
{
    if (gContext != nullptr) {
        if (gHeroTexture != 0) glDeleteTextures(1, &gHeroTexture);
        if (gHeroVbo != 0) glDeleteBuffers(1, &gHeroVbo);
        if (gHeroVao != 0) glDeleteVertexArrays(1, &gHeroVao);
    }
    gHeroTexture = 0;
    gHeroVbo = 0;
    gHeroVao = 0;
    gHeroLoaded = false;
    gHeroInfo = {};
}

void destroyStaticObjects()
{
    destroyStaticObjectTextures();
    if (gContext != nullptr) {
        if (gObjectVbo != 0) glDeleteBuffers(1, &gObjectVbo);
        if (gObjectVao != 0) glDeleteVertexArrays(1, &gObjectVao);
    }
    gObjectVbo = 0;
    gObjectVao = 0;
    gObjectVertices.clear();
    gObjectDraws.clear();
}

} // namespace internal

bool updateStaticHero(const StaticHeroFrame& frame, std::string* error)
{
    auto fail = [&](const std::string& message) {
        destroyStaticHero();
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized || !gWallsLoaded || !gHeroProjectionLoaded) {
        return fail("static wall/hero projection is unavailable");
    }
    if (frame.elevation < 0 || frame.elevation > 2 || frame.tile < 0 || frame.tile >= 200 * 200
        || frame.rotation < 0 || frame.rotation >= 6) {
        return fail("hero tile, elevation, or rotation is invalid");
    }
    if (frame.width <= 0 || frame.height <= 0 || frame.width > 2048 || frame.height > 2048
        || frame.objectX < -32768 || frame.objectX > 32768
        || frame.objectY < -32768 || frame.objectY > 32768
        || frame.artXOffset < -32768 || frame.artXOffset > 32768
        || frame.artYOffset < -32768 || frame.artYOffset > 32768
        || frame.rgba == nullptr) {
        return fail("hero frame dimensions or pixels are invalid");
    }
    const unsigned long long expectedRgba = static_cast<unsigned long long>(frame.width)
        * static_cast<unsigned long long>(frame.height) * 4;
    if (expectedRgba != frame.rgbaSize) return fail("hero frame RGBA byte count is inconsistent");

    const SpriteCardArt art = { frame.tile, frame.objectX, frame.objectY, frame.artXOffset, frame.artYOffset,
        frame.width, frame.height };
    SpriteCardVertex corners[4];
    double worldU = 0.0;
    double worldV = 0.0;
    if (!standingSpriteCard(cardProjection(), art, corners, &worldU, &worldV)) {
        return fail("hero card anchor or height is not finite");
    }
    faceFlyCamera(corners);
    const double row = spriteRowPlane(cardProjection(), frame.tile, frame.stack);
    const StaticPreviewCamera camera = staticPreviewCamera(true);
    const DepthVertex topLeft = cardVertex(corners[0], row, camera.eye);
    const DepthVertex bottomLeft = cardVertex(corners[1], row, camera.eye);
    const DepthVertex bottomRight = cardVertex(corners[2], row, camera.eye);
    const DepthVertex topRight = cardVertex(corners[3], row, camera.eye);
    const DepthVertex vertices[] = { topLeft, bottomLeft, bottomRight, topLeft, bottomRight, topRight };

    while (glGetError() != 0) {
    }
    glPixelStorei(kUnpackAlignment, 1);
    if (gHeroVao == 0) glGenVertexArrays(1, &gHeroVao);
    if (gHeroVbo == 0) glGenBuffers(1, &gHeroVbo);
    if (gHeroTexture == 0) glGenTextures(1, &gHeroTexture);
    glBindVertexArray(gHeroVao);
    glBindBuffer(kArrayBuffer, gHeroVbo);
    glBufferData(kArrayBuffer, sizeof(vertices), vertices, kStaticDraw);
    depthVertexLayout();
    glBindTexture(kTexture2D, gHeroTexture);
    glTexImage2D(kTexture2D, 0, kRgba8, frame.width, frame.height, 0, kRGBA, kUnsignedByte, frame.rgba);
    glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
    glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
    glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
    glPixelStorei(kUnpackAlignment, 4);
    glBindVertexArray(0);
    if (glGetError() != 0) return fail("OpenGL failed to upload the hero card");

    gHeroInfo = { frame.tile, frame.fid, frame.frame, frame.rotation, worldU, worldV };
    gHeroPickId = frame.pickId;
    gHeroLoaded = true;
    if (error != nullptr) error->clear();
    return true;
}

void clearStaticHero()
{
    destroyStaticHero();
}

bool staticHeroInfo(StaticHeroInfo* info)
{
    if (info == nullptr || !gHeroLoaded) return false;
    *info = gHeroInfo;
    return true;
}

bool staticObjectArtCached(int fid, int frame, int rotation)
{
    return gObjectTextures.count(objectArtKey(fid, frame, rotation)) != 0;
}

void beginStaticObjects()
{
    gObjectVertices.clear();
    gObjectDraws.clear();
    // Bound the cache; a frame's texture is uploaded again on demand.
    if (gObjectTextures.size() > kMaxObjectTextures) destroyStaticObjectTextures();
}

bool addStaticObject(const StaticObjectFrame& frame, std::string* error)
{
    auto fail = [&](const char* message) {
        if (error != nullptr) *error = message;
        return false;
    };
    if (!gInitialized || !gWallsLoaded || !gHeroProjectionLoaded) return fail("static scene is unavailable");
    if (frame.rotation < 0 || frame.rotation >= 6) return fail("object rotation is invalid");
    if (!staticSoloKeeps(frame.tile)) return true;
    const SpriteCardArt art = { frame.tile, frame.objectX, frame.objectY, frame.artXOffset, frame.artYOffset,
        frame.width, frame.height };
    // Low things lie flat under the fly camera; the game's camera sees them as
    // cards, on their own pixels.
    double flyRightX, flyRightZ;
    const bool flat = frame.flat
        || (staticObjectFlyFlat(frame.tile, frame.fid) && flyCameraRight(&flyRightX, &flyRightZ));
    SpriteCardVertex corners[4];
    if (flat) {
        if (!flatSpriteCard(cardProjection(), art, kFlatObjectHeight, corners)) return fail("flat object card is invalid");
    } else if (!standingSpriteCard(cardProjection(), art, corners, nullptr, nullptr)) {
        return fail("object card is invalid");
    }
    const StaticPreviewCamera camera = staticPreviewCamera(true);
    float plane[3];
    if (flat) {
    } else if (staticObjectPlane(frame.tile, frame.fid, plane)) {
        onFlyPlane(corners, plane, camera.eye);
    } else {
        faceFlyCamera(corners);
    }
    const double row = flat ? 0.0 : spriteRowPlane(cardProjection(), frame.tile, frame.stack);

    const unsigned long long key = objectArtKey(frame.fid, frame.frame, frame.rotation);
    auto cached = gObjectTextures.find(key);
    unsigned int texture = 0;
    if (cached != gObjectTextures.end()) {
        texture = cached->second;
    } else {
        const unsigned long long expectedRgba = static_cast<unsigned long long>(frame.width)
            * static_cast<unsigned long long>(frame.height) * 4;
        if (frame.rgba == nullptr || expectedRgba != frame.rgbaSize) return fail("object frame pixels are missing");
        while (glGetError() != 0) {
        }
        glGenTextures(1, &texture);
        glBindTexture(kTexture2D, texture);
        glPixelStorei(kUnpackAlignment, 1);
        glPixelStorei(kUnpackRowLength, 0);
        glTexImage2D(kTexture2D, 0, kRgba8, frame.width, frame.height, 0, kRGBA, kUnsignedByte, frame.rgba);
        glPixelStorei(kUnpackAlignment, 4);
        glTexParameteri(kTexture2D, kTextureMinFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureMagFilter, kNearest);
        glTexParameteri(kTexture2D, kTextureWrapS, kClampToEdge);
        glTexParameteri(kTexture2D, kTextureWrapT, kClampToEdge);
        if (glGetError() != 0) {
            glDeleteTextures(1, &texture);
            return fail("OpenGL failed to upload an object frame");
        }
        gObjectTextures.emplace(key, texture);
    }

    gObjectDraws.push_back({ texture, static_cast<int>(gObjectVertices.size()), flat,
        { frame.tint[0], frame.tint[1], frame.tint[2] }, frame.glassGray, frame.pickId, frame.pickOnly,
        frame.seeThrough, frame.fullLight ? -1 : frame.tile });
    // A thing standing on a solid moves onto it, and so does its row plane.
    float lift[3] = { 0.0f, 0.0f, 0.0f };
    const bool lifted = staticObjectLift(frame.tile, frame.fid, lift);
    const double liftedRow = row + lift[2];
    const int order[6] = { 0, 1, 2, 0, 2, 3 };
    for (int index : order) {
        SpriteCardVertex corner = corners[index];
        if (lifted) {
            corner.x += lift[0];
            corner.y += lift[1];
            corner.z += lift[2];
        }
        gObjectVertices.push_back(flat
                ? DepthVertex { static_cast<float>(corner.x), static_cast<float>(corner.y), static_cast<float>(corner.z),
                      static_cast<float>(corner.u), static_cast<float>(corner.v), static_cast<float>(corner.x),
                      static_cast<float>(corner.y), static_cast<float>(corner.z) }
                : cardVertex(corner, liftedRow, camera.eye));
    }
    if (error != nullptr) error->clear();
    return true;
}

void endStaticObjects()
{
    if (!gInitialized || gObjectVertices.empty()) return;
    if (gObjectVao == 0) glGenVertexArrays(1, &gObjectVao);
    if (gObjectVbo == 0) glGenBuffers(1, &gObjectVbo);
    glBindVertexArray(gObjectVao);
    glBindBuffer(kArrayBuffer, gObjectVbo);
    glBufferData(kArrayBuffer, static_cast<long long>(gObjectVertices.size() * sizeof(DepthVertex)),
        gObjectVertices.data(), 0x88E8 /* GL_DYNAMIC_DRAW */);
    depthVertexLayout();
    glBindVertexArray(0);
}

void clearStaticObjects()
{
    destroyStaticObjects();
}

unsigned int staticObjectCount()
{
    return static_cast<unsigned int>(gObjectDraws.size());
}

} // namespace scene_renderer
} // namespace fallout
