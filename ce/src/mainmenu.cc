#include "mainmenu.h"

#include <lodepng.h>

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "art.h"
#include "color.h"
#include "content_config.h"
#include "debug.h"
#include "delay.h"
#include "draw.h"
#include "font_manager.h"
#include "game.h"
#include "game_sound.h"
#include "geometry.h"
#include "input.h"
#include "kb.h"
#include "mouse.h"
#include "owner_art_viewport.h"
#include "palette.h"
#include "platform_compat.h"
#include "preferences.h"
#include "settings.h"
#include "svga.h"
#include "text_font.h"
#include "version.h"
#include "window_manager.h"

#include "platform/git_version.h"

#include <algorithm>
#include <math.h>
#include <stdint.h>

#include <vector>

namespace fallout {

#define MAIN_MENU_LOGICAL_WIDTH 640
#define MAIN_MENU_LOGICAL_HEIGHT 480
#define MAIN_MENU_BUTTON_WIDTH 26
#define MAIN_MENU_BUTTON_HEIGHT 26
#define MAIN_MENU_PANEL_OFFSET_X 16
#define MAIN_MENU_PANEL_OFFSET_Y 15
#define MAIN_MENU_BUTTON_X 30
#define MAIN_MENU_BUTTON_Y 19
#define MAIN_MENU_BUTTON_Y_STEP 41
#define MAIN_MENU_BUTTON_LABEL_X 126
#define MAIN_MENU_BUTTON_LABEL_Y 20
#define MAIN_MENU_FOOTER_LEFT_X 15
#define MAIN_MENU_FOOTER_RIGHT_MARGIN 25
#define MAIN_MENU_COPYRIGHT_Y 460
#define MAIN_MENU_VERSION_Y 440
#define MAIN_MENU_BUILD_HASH_Y 450
#define MAIN_MENU_BUILD_DATE_Y 460
constexpr int mainMenuOverlayWindowWidth = 640;
constexpr int mainMenuOverlayWindowHeight = 480;
constexpr int mainMenuOverlayDimTargetIntensity = 0x8000;
constexpr int mainMenuOverlayDimClearIntensity = 0x10000;
constexpr int mainMenuOverlayDimAnimationSteps = 5;
constexpr int mainMenuOverlayDimFrameDelay = 18;

typedef enum MainMenuButton {
    MAIN_MENU_BUTTON_INTRO,
    MAIN_MENU_BUTTON_NEW_GAME,
    MAIN_MENU_BUTTON_LOAD_GAME,
    MAIN_MENU_BUTTON_OPTIONS,
    MAIN_MENU_BUTTON_CREDITS,
    MAIN_MENU_BUTTON_EXIT,
    MAIN_MENU_BUTTON_COUNT,
} MainMenuButton;

enum class MenuArt {
    Vanilla,
    Hires,
};

static int main_menu_fatal_error();
static void main_menu_play_sound(const char* fileName);

// 0x5194F0 main_window
static int gMainMenuWindow = -1;

// 0x5194F4 main_window_buf
static unsigned char* gMainMenuWindowBuffer = nullptr;

// 0x519504 in_main_menu
static bool _in_main_menu = false;

// 0x519508 main_menu_created
static bool gMainMenuWindowInitialized = false;

static bool gMainMenuExitRequested = false;

// 0x51950C main_menu_timeout
static unsigned int gMainMenuScreensaverDelay = 120000;

struct MainMenuButtonSpec {
    int keyCode;
    int returnValue;
    int labelMessage;
};

// 0x519510 button_values / 0x519528 return_values
static const MainMenuButtonSpec mainMenuButtonSpecs[MAIN_MENU_BUTTON_COUNT] = {
    { KEY_LOWERCASE_I, MAIN_MENU_INTRO, 9 },
    { KEY_LOWERCASE_N, MAIN_MENU_NEW_GAME, 10 },
    { KEY_LOWERCASE_L, MAIN_MENU_LOAD_GAME, 11 },
    { KEY_LOWERCASE_O, MAIN_MENU_OPTIONS, 12 },
    { KEY_LOWERCASE_C, MAIN_MENU_CREDITS, 13 },
    { KEY_LOWERCASE_E, MAIN_MENU_EXIT, 14 },
};

// 0x614840 buttons
static int gMainMenuButtons[MAIN_MENU_BUTTON_COUNT];

// 0x614858 main_menu_is_hidden
static bool gMainMenuWindowHidden;
static int mainMenuOverlayCount = 0;
static bool mainMenuOverlayBackgroundEnabled = false;
static int mainMenuSubscreenBackdrop = -1;

static FrmImage mainMenuBackgroundFrmImage;
static FrmImage mainMenuButtonPanelFrmImage;
static FrmImage mainMenuButtonNormalFrmImage;
static FrmImage mainMenuButtonPressedFrmImage;
static std::vector<unsigned char> mainMenuScaledButtonData;
static std::vector<unsigned char> mainMenuOverlayDimmedBackup;
static std::vector<unsigned char> mainMenuRgbaBackground;
static std::vector<unsigned char> mainMenuRgbaScreenBackground;
static std::vector<unsigned char> mainMenuRgbaBackgroundMask;
static int mainMenuRgbaBackgroundWidth = 0;
static int mainMenuRgbaBackgroundHeight = 0;
static int mainMenuRgbaScreenWidth = 0;
static int mainMenuRgbaScreenHeight = 0;
static bool mainMenuHasRgbaBackground = false;
static void mainMenuSetButtonsEnabled(bool enabled);
static void mainMenuApplyOverlayDim();
static void mainMenuRemoveOverlayDim();
static void mainMenuSetOverlayDimIntensity(int intensity);
static void mainMenuAnimateOverlayDim(int startIntensity, int endIntensity);

struct MainMenuLayout {
    int screenWidth;
    int screenHeight;
    int viewportX;
    int viewportY;
    int viewportWidth;
    int viewportHeight;
    int backgroundX;
    int backgroundY;
    int backgroundWidth;
    int backgroundHeight;
    float scaleX;
    float scaleY;
    float scale;
    MenuArt art;
    bool scaleControls;
};

struct MainMenuOffsets {
    int menuX;
    int menuY;
    int creditsX;
    int creditsY;
};

static void mainMenuComputeAspectFit(int srcWidth, int srcHeight, int& outX, int& outY, int& outWidth, int& outHeight);
static bool mainMenuShouldAspectFit(int backgroundWidth, int backgroundHeight);
static bool mainMenuShouldUseVanillaArtForLayout(int backgroundWidth, int backgroundHeight);
static bool mainMenuLoadArt();
static MainMenuLayout mainMenuBuildLayout();
static void mainMenuDrawBackground(const MainMenuLayout& layout);
static MainMenuOffsets mainMenuReadOffsets(const MainMenuLayout& layout);
static void mainMenuDrawPanel(const MainMenuLayout& layout, const MainMenuOffsets& offsets);
static Point mainMenuTransformPoint(const MainMenuLayout& layout, int x, int y);
static Size mainMenuTransformSize(const MainMenuLayout& layout, int width, int height);
static int mainMenuScaleX(const MainMenuLayout& layout, int value);
static int mainMenuScaleY(const MainMenuLayout& layout, int value);
static int mainMenuGetAnchoredY(const MainMenuLayout& layout, int value);
static int mainMenuGetAnchoredRightX(const MainMenuLayout& layout, int rightMargin, int width);
static bool mainMenuShouldEnableOverlayBackground(const MainMenuLayout& layout);
static void mainMenuDrawBuildInfo(const MainMenuLayout& layout, const MainMenuOffsets& offsets);
static bool mainMenuCreateButtons(const MainMenuLayout& layout, const MainMenuOffsets& offsets);
static void mainMenuDrawButtonLabels(const MainMenuLayout& layout, const MainMenuOffsets& offsets);
static bool mainMenuWindowIsOverlayActive();
static void mainMenuWindowEnterOverlay();
static void mainMenuWindowLeaveOverlay();
static void mainMenuWindowShowOverlayDim();
static void mainMenuMaskBackground(int x, int y, int width, int height);
static void mainMenuEnableRgbaBackground();
static void mainMenuPrepareRgbaBackground(const MainMenuLayout& layout);

static void mainMenuComputeAspectFit(int srcWidth, int srcHeight, int& outX, int& outY, int& outWidth, int& outHeight)
{
    int dstWidth = screenGetWidth();
    int dstHeight = screenGetHeight();
    outWidth = dstWidth;
    outHeight = dstHeight;

    if (mainMenuHasRgbaBackground) {
        // Keep owner art proportional and crop its center to fill the screen.
        if (static_cast<int64_t>(dstWidth) * srcHeight > static_cast<int64_t>(dstHeight) * srcWidth) {
            outHeight = static_cast<int>((static_cast<int64_t>(dstWidth) * srcHeight + srcWidth - 1) / srcWidth);
        } else {
            outWidth = static_cast<int>((static_cast<int64_t>(dstHeight) * srcWidth + srcHeight - 1) / srcHeight);
        }
        outX = (dstWidth - outWidth) / 2;
        outY = (dstHeight - outHeight) / 2;
        return;
    } else if (static_cast<int64_t>(dstHeight) * srcWidth > static_cast<int64_t>(dstWidth) * srcHeight) {
        outHeight = static_cast<int>(static_cast<int64_t>(dstWidth) * srcHeight / srcWidth);
    } else {
        outWidth = static_cast<int>(static_cast<int64_t>(dstHeight) * srcWidth / srcHeight);
    }

    outX = (dstWidth - outWidth) / 2;
    outY = (dstHeight - outHeight) / 2;
}

static void mainMenuMaskBackground(int x, int y, int width, int height)
{
    if (!mainMenuHasRgbaBackground || width <= 0 || height <= 0) {
        return;
    }

    int left = std::max(0, x);
    int top = std::max(0, y);
    int right = std::min(mainMenuRgbaScreenWidth, x + width);
    int bottom = std::min(mainMenuRgbaScreenHeight, y + height);
    if (left >= right || top >= bottom) {
        return;
    }
    for (int row = top; row < bottom; row++) {
        memset(mainMenuRgbaBackgroundMask.data() + static_cast<size_t>(row) * mainMenuRgbaScreenWidth + left,
            0,
            right - left);
    }
}

static void mainMenuEnableRgbaBackground()
{
    if (mainMenuHasRgbaBackground) {
        renderSetRgbaBackground(mainMenuRgbaScreenBackground.data(), mainMenuRgbaBackgroundMask.data(),
            mainMenuRgbaScreenWidth, mainMenuRgbaScreenHeight);
    } else {
        renderClearRgbaBackground();
    }
}

static void mainMenuPrepareRgbaBackground(const MainMenuLayout& layout)
{
    if (!mainMenuHasRgbaBackground) {
        return;
    }

    mainMenuRgbaScreenWidth = layout.screenWidth;
    mainMenuRgbaScreenHeight = layout.screenHeight;
    size_t pixelCount = static_cast<size_t>(layout.screenWidth) * layout.screenHeight;
    mainMenuRgbaScreenBackground.assign(pixelCount * 4, 0);
    mainMenuRgbaBackgroundMask.assign(pixelCount, 0);

    for (int y = 0; y < layout.backgroundHeight; y++) {
        int dstY = layout.backgroundY + y;
        if (dstY < 0 || dstY >= layout.screenHeight) continue;
        int srcY = static_cast<int64_t>(y) * mainMenuRgbaBackgroundHeight / layout.backgroundHeight;
        for (int x = 0; x < layout.backgroundWidth; x++) {
            int dstX = layout.backgroundX + x;
            if (dstX < 0 || dstX >= layout.screenWidth) continue;
            int srcX = static_cast<int64_t>(x) * mainMenuRgbaBackgroundWidth / layout.backgroundWidth;
            size_t srcOffset = (static_cast<size_t>(srcY) * mainMenuRgbaBackgroundWidth + srcX) * 4;
            size_t dstOffset = (static_cast<size_t>(dstY) * layout.screenWidth + dstX) * 4;
            memcpy(mainMenuRgbaScreenBackground.data() + dstOffset, mainMenuRgbaBackground.data() + srcOffset, 4);
            mainMenuRgbaBackgroundMask[static_cast<size_t>(dstY) * layout.screenWidth + dstX] = 1;
        }
    }
}

static bool mainMenuShouldAspectFit(int backgroundWidth, int backgroundHeight)
{
    return mainMenuHasRgbaBackground || settings.ui.main_menu_scale_mode != 0
        || backgroundWidth > screenGetWidth() || backgroundHeight > screenGetHeight();
}

static bool mainMenuShouldUseVanillaArtForLayout(int backgroundWidth, int backgroundHeight)
{
    int x, y, width, height;
    if (!mainMenuShouldAspectFit(backgroundWidth, backgroundHeight)) {
        return false;
    }

    mainMenuComputeAspectFit(backgroundWidth, backgroundHeight, x, y, width, height);
    return width == MAIN_MENU_LOGICAL_WIDTH && height == MAIN_MENU_LOGICAL_HEIGHT;
}

static bool mainMenuLoadArt()
{
    mainMenuHasRgbaBackground = false;
    mainMenuRgbaBackground.clear();
    mainMenuRgbaScreenBackground.clear();
    mainMenuRgbaBackgroundMask.clear();
    mainMenuRgbaBackgroundWidth = 0;
    mainMenuRgbaBackgroundHeight = 0;
    mainMenuRgbaScreenWidth = 0;
    mainMenuRgbaScreenHeight = 0;

    unsigned int backgroundWidth = 0;
    unsigned int backgroundHeight = 0;
    unsigned error = lodepng::decode(mainMenuRgbaBackground, backgroundWidth, backgroundHeight,
        "user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/art__intrface__hr_mainmenu__d0_f0_21-9.png");
    if (error != 0) {
        error = lodepng::decode(mainMenuRgbaBackground, backgroundWidth, backgroundHeight,
            "game/user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/art__intrface__hr_mainmenu__d0_f0_21-9.png");
    }
    if (error != 0) {
        error = lodepng::decode(mainMenuRgbaBackground, backgroundWidth, backgroundHeight,
            "user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/hr_mainmenu.png");
        if (error != 0) {
            error = lodepng::decode(mainMenuRgbaBackground, backgroundWidth, backgroundHeight,
                "game/user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/hr_mainmenu.png");
        }
    }
    if (error == 0 && backgroundWidth > 0 && backgroundHeight > 0
        && backgroundWidth <= SHRT_MAX && backgroundHeight <= SHRT_MAX) {
        mainMenuHasRgbaBackground = true;
        mainMenuRgbaBackgroundWidth = static_cast<int>(backgroundWidth);
        mainMenuRgbaBackgroundHeight = static_cast<int>(backgroundHeight);
        if (!mainMenuButtonPanelFrmImage.lock(InterfaceFrmId("HR_MENU_BG.FRM"))) {
            debugPrint("MAINMENU: failed to load hires HR_MENU_BG.FRM; using the owner background without a panel\n");
        }
    } else {
        debugPrint("MAINMENU: owner widescreen background unavailable (%s)\n", lodepng_error_text(error));
    }

    bool canUseHiresArt = screenGetWidth() != MAIN_MENU_LOGICAL_WIDTH || screenGetHeight() != MAIN_MENU_LOGICAL_HEIGHT;
    if (!mainMenuHasRgbaBackground && canUseHiresArt && mainMenuBackgroundFrmImage.lock(InterfaceFrmId("HR_MAINMENU.FRM"))) {
        if (mainMenuShouldUseVanillaArtForLayout(mainMenuBackgroundFrmImage.getWidth(), mainMenuBackgroundFrmImage.getHeight())) {
            // use Vanilla art if not scaling to reduce artifacts
            mainMenuBackgroundFrmImage.unlock();
        } else {
            // for highres main menu art, use separate panel art
            if (!mainMenuButtonPanelFrmImage.lock(InterfaceFrmId("HR_MENU_BG.FRM"))) {
                debugPrint("MAINMENU: failed to load hires HR_MENU_BG.FRM, falling back to vanilla mainmenu.frm\n");
                mainMenuBackgroundFrmImage.unlock();
            }
        }
    }

    if (!mainMenuBackgroundFrmImage.isLocked()) {
        if (!mainMenuBackgroundFrmImage.lock(InterfaceFrameId::MainMenuBackgroundImage)) {
            debugPrint("MAINMENU: failed to load vanilla mainmenu.frm\n");
            return false;
        }
    }

    if (!mainMenuButtonNormalFrmImage.lock(InterfaceFrameId::MainMenuButtonUp)) {
        return false;
    }

    if (!mainMenuButtonPressedFrmImage.lock(InterfaceFrameId::MainMenuButtonDown)) {
        return false;
    }

    return true;
}

static MainMenuLayout mainMenuBuildLayout()
{
    MainMenuLayout layout {};
    layout.screenWidth = screenGetWidth();
    layout.screenHeight = screenGetHeight();
    OwnerArtViewport viewport = ownerArtViewport(layout.screenWidth, layout.screenHeight);
    if (mainMenuHasRgbaBackground) {
        viewport = { 0, 0, layout.screenWidth, layout.screenHeight };
    }
    layout.viewportX = viewport.x;
    layout.viewportY = viewport.y;
    layout.viewportWidth = viewport.width;
    layout.viewportHeight = viewport.height;
    int backgroundWidth = mainMenuHasRgbaBackground ? mainMenuRgbaBackgroundWidth : mainMenuBackgroundFrmImage.getWidth();
    int backgroundHeight = mainMenuHasRgbaBackground ? mainMenuRgbaBackgroundHeight : mainMenuBackgroundFrmImage.getHeight();
    layout.art = mainMenuHasRgbaBackground || backgroundWidth != MAIN_MENU_LOGICAL_WIDTH || backgroundHeight != MAIN_MENU_LOGICAL_HEIGHT
        ? MenuArt::Hires
        : MenuArt::Vanilla;

    bool aspectFit = mainMenuShouldAspectFit(backgroundWidth, backgroundHeight);
    if (aspectFit) {
        mainMenuComputeAspectFit(backgroundWidth, backgroundHeight, layout.backgroundX, layout.backgroundY, layout.backgroundWidth, layout.backgroundHeight);
    } else {
        layout.backgroundWidth = backgroundWidth;
        layout.backgroundHeight = backgroundHeight;
        layout.backgroundX = (layout.screenWidth - layout.backgroundWidth) / 2;
        layout.backgroundY = (layout.screenHeight - layout.backgroundHeight) / 2;
    }

    layout.scaleX = layout.backgroundWidth / static_cast<float>(MAIN_MENU_LOGICAL_WIDTH);
    layout.scaleY = layout.backgroundHeight / static_cast<float>(MAIN_MENU_LOGICAL_HEIGHT);

    bool scaleButtonsAndText = false;
    configGetBool(&gContentConfig, CONTENT_CONFIG_MAIN_MENU_SECTION, "scale_buttons_and_text", &scaleButtonsAndText, false);
    layout.scaleControls = layout.art == MenuArt::Vanilla || settings.ui.main_menu_scale_mode >= 2 || scaleButtonsAndText;
    layout.scale = layout.scaleControls ? layout.scaleY : 1.0f;
    return layout;
}

static void mainMenuDrawBackground(const MainMenuLayout& layout)
{
    Buffer2D dest(gMainMenuWindowBuffer, layout.screenWidth, layout.screenHeight);
    bufferFill2D(dest, COLOR_FIRST);

    if (mainMenuHasRgbaBackground) {
        for (int y = 0; y < layout.screenHeight; y++) {
            for (int x = 0; x < layout.screenWidth; x++) {
                const unsigned char* pixel = mainMenuRgbaScreenBackground.data()
                    + (static_cast<size_t>(y) * layout.screenWidth + x) * 4;
                if (pixel[3] == 0) continue;
                int rgb555Index = ((pixel[0] >> 3) << 10) | ((pixel[1] >> 3) << 5) | (pixel[2] >> 3);
                gMainMenuWindowBuffer[y * layout.screenWidth + x] = _colorTable[rgb555Index];
            }
        }
        return;
    }

    if (layout.backgroundWidth == mainMenuBackgroundFrmImage.getWidth()
        && layout.backgroundHeight == mainMenuBackgroundFrmImage.getHeight()) {
        blitBuffer2D(mainMenuBackgroundFrmImage.getBuffer(), dest, layout.backgroundX, layout.backgroundY);
    } else {
        blitBuffer2DScaled(mainMenuBackgroundFrmImage.getBuffer(),
            0,
            0,
            mainMenuBackgroundFrmImage.getWidth(),
            mainMenuBackgroundFrmImage.getHeight(),
            dest,
            layout.backgroundX,
            layout.backgroundY,
            layout.backgroundWidth,
            layout.backgroundHeight);
    }
}

static MainMenuOffsets mainMenuReadOffsets(const MainMenuLayout& layout)
{
    MainMenuOffsets offsets {};
    configGetInt(&gContentConfig, CONTENT_CONFIG_MAIN_MENU_SECTION, "offset_x", &offsets.menuX, 0);
    configGetInt(&gContentConfig, CONTENT_CONFIG_MAIN_MENU_SECTION, "offset_y", &offsets.menuY, 0);
    configGetInt(&gContentConfig, CONTENT_CONFIG_MAIN_MENU_SECTION, "credits_offset_x", &offsets.creditsX, 0);
    configGetInt(&gContentConfig, CONTENT_CONFIG_MAIN_MENU_SECTION, "credits_offset_y", &offsets.creditsY, 0);

    offsets.menuX = mainMenuScaleX(layout, offsets.menuX);
    offsets.menuY = mainMenuScaleY(layout, offsets.menuY);

    // Match HRP's extra compensation when the hires background is scaled but
    // the buttons/panel stay at their original size.  It uses this funny quadratic scaling to place the menu
    if (layout.art == MenuArt::Hires && !layout.scaleControls) {
        if (layout.viewportWidth != MAIN_MENU_LOGICAL_WIDTH) {
            float diff = layout.viewportWidth / static_cast<float>(MAIN_MENU_LOGICAL_WIDTH);
            offsets.menuX += static_cast<int>(lround(8.0f * diff * diff));
        }
        if (layout.screenHeight != MAIN_MENU_LOGICAL_HEIGHT) {
            float diff = layout.screenHeight / static_cast<float>(MAIN_MENU_LOGICAL_HEIGHT);
            offsets.menuY += static_cast<int>(lround(6.0f * diff * diff));
        }
    }

    if (mainMenuHasRgbaBackground) {
        offsets.menuX -= layout.backgroundX - layout.viewportX;
        offsets.menuY -= layout.backgroundY - layout.viewportY;
    }

    Size buttonSize = mainMenuTransformSize(layout, MAIN_MENU_BUTTON_WIDTH, MAIN_MENU_BUTTON_HEIGHT);
    Point firstButton = mainMenuTransformPoint(layout, MAIN_MENU_BUTTON_X, MAIN_MENU_BUTTON_Y);
    Point lastButton = mainMenuTransformPoint(layout, MAIN_MENU_BUTTON_X,
        MAIN_MENU_BUTTON_Y + (MAIN_MENU_BUTTON_COUNT - 1) * MAIN_MENU_BUTTON_Y_STEP);
    int firstX = firstButton.x + offsets.menuX;
    int firstY = firstButton.y + offsets.menuY;
    int lastY = lastButton.y + offsets.menuY;
    if (firstX < 0) offsets.menuX -= firstX;
    if (firstX + buttonSize.width > layout.screenWidth) offsets.menuX -= firstX + buttonSize.width - layout.screenWidth;
    if (firstY < 0) offsets.menuY -= firstY;
    if (lastY + buttonSize.height > layout.screenHeight) offsets.menuY -= lastY + buttonSize.height - layout.screenHeight;

    return offsets;
}

static void mainMenuDrawPanel(const MainMenuLayout& layout, const MainMenuOffsets& offsets)
{
    if (layout.art != MenuArt::Hires || !mainMenuButtonPanelFrmImage.isLocked()) {
        return;
    }

    int panelOffsetX = MAIN_MENU_PANEL_OFFSET_X;
    int panelOffsetY = MAIN_MENU_PANEL_OFFSET_Y;
    configGetInt(&gContentConfig, CONTENT_CONFIG_MAIN_MENU_SECTION, "main_menu_panel_offset_x", &panelOffsetX, MAIN_MENU_PANEL_OFFSET_X);
    configGetInt(&gContentConfig, CONTENT_CONFIG_MAIN_MENU_SECTION, "main_menu_panel_offset_y", &panelOffsetY, MAIN_MENU_PANEL_OFFSET_Y);

    Size panelSize = mainMenuTransformSize(layout, mainMenuButtonPanelFrmImage.getWidth(), mainMenuButtonPanelFrmImage.getHeight());
    Point panelOrigin = mainMenuTransformPoint(layout, panelOffsetX, panelOffsetY);

    blitBuffer2DScaledTrans(mainMenuButtonPanelFrmImage.getBuffer(),
        Buffer2D(gMainMenuWindowBuffer, layout.screenWidth, layout.screenHeight),
        panelOrigin.x + offsets.menuX,
        panelOrigin.y + offsets.menuY,
        panelSize.width,
        panelSize.height);
    mainMenuMaskBackground(panelOrigin.x + offsets.menuX, panelOrigin.y + offsets.menuY, panelSize.width, panelSize.height);
}

static Point mainMenuTransformPoint(const MainMenuLayout& layout, int x, int y)
{
    return {
        layout.backgroundX + (layout.scaleControls ? static_cast<int>(lround(x * layout.scale)) : x),
        layout.backgroundY + (layout.scaleControls ? static_cast<int>(lround(y * layout.scale)) : y),
    };
}

static Size mainMenuTransformSize(const MainMenuLayout& layout, int width, int height)
{
    return {
        std::max(1, static_cast<int>(lround(width * layout.scale))),
        std::max(1, static_cast<int>(lround(height * layout.scale))),
    };
}

static int mainMenuScaleX(const MainMenuLayout& layout, int value)
{
    return static_cast<int>(lround(value * layout.scaleX));
}

static int mainMenuScaleY(const MainMenuLayout& layout, int value)
{
    return static_cast<int>(lround(value * layout.scaleY));
}

static int mainMenuGetAnchoredY(const MainMenuLayout& layout, int value)
{
    int bottom = mainMenuHasRgbaBackground
        ? layout.viewportY + layout.viewportHeight
        : layout.backgroundY + layout.backgroundHeight;
    return bottom + value - MAIN_MENU_LOGICAL_HEIGHT;
}

static int mainMenuGetAnchoredRightX(const MainMenuLayout& layout, int rightMargin, int width)
{
    if (mainMenuHasRgbaBackground) {
        return layout.viewportX + layout.viewportWidth - rightMargin - width;
    }
    return layout.backgroundX + layout.backgroundWidth - rightMargin - width;
}

static bool mainMenuShouldEnableOverlayBackground(const MainMenuLayout& layout)
{
    return settings.ui.main_menu_overlay_subscreens
        && (layout.backgroundWidth > mainMenuOverlayWindowWidth
            || layout.backgroundHeight > mainMenuOverlayWindowHeight);
}

static void mainMenuDrawBuildInfo(const MainMenuLayout& layout, const MainMenuOffsets& offsets)
{
    MessageListItem msg;

    // SFALL: Allow to change font color/flags of copyright/version text
    //        It's the last byte ('3C' by default) that picks the colour used. The first byte supplies additional flags for this option
    //        0x010000 - change the color for version string only
    //        0x020000 - underline text (only for the version string)
    //        0x040000 - monospace font (only for the version string)
    ColorWithFlags fontSettings = COLOR_YELLOW_2 | DRAW_TEXT_FLAG_NONE;
    ColorWithFlags fontSettingsSFall = COLOR_FIRST | DRAW_TEXT_FLAG_NONE;
    configGetEnum<ColorWithFlags>(&gContentConfig, CONTENT_CONFIG_MAIN_MENU_SECTION, "font_color", &fontSettingsSFall, COLOR_FIRST | DRAW_TEXT_FLAG_NONE);
    if (fontSettingsSFall && !(fontSettingsSFall & DRAW_TEXT_FLAG_SHADOWED)) {
        fontSettings = static_cast<Color>(fontSettingsSFall & COLOR_LAST) | DRAW_TEXT_FLAG_NONE;
    }

    msg.num = 20;
    if (messageListGetItem(&gMiscMessageList, &msg)) {
        windowDrawText(gMainMenuWindow,
            msg.text,
            0,
            (mainMenuHasRgbaBackground ? layout.viewportX : layout.backgroundX) + offsets.creditsX + MAIN_MENU_FOOTER_LEFT_X,
            mainMenuGetAnchoredY(layout, offsets.creditsY + MAIN_MENU_COPYRIGHT_Y),
            fontSettings | (DRAW_TEXT_FLAG_NO_BG | DRAW_TEXT_FLAG_OVERFLOW));
    }

    if (fontSettingsSFall) {
        fontSettings = fontSettingsSFall;
    }

    fontSettings &= ~DRAW_TEXT_FLAG_SHADOWED;
    ColorWithFlags versionFontSettings = fontSettings;
    char version[VERSION_MAX];
    versionGetVersion(version, sizeof(version));
    int len = fontGetStringWidth(version);
    windowDrawText(gMainMenuWindow, version, 0, mainMenuGetAnchoredRightX(layout, MAIN_MENU_FOOTER_RIGHT_MARGIN, len), mainMenuGetAnchoredY(layout, MAIN_MENU_VERSION_Y), versionFontSettings | (DRAW_TEXT_FLAG_NO_BG | DRAW_TEXT_FLAG_OVERFLOW));

    char commitHash[VERSION_MAX] = "BUILD HASH: ";
    strcat(commitHash, _BUILD_HASH);
    len = fontGetStringWidth(commitHash);
    windowDrawText(gMainMenuWindow, commitHash, 0, mainMenuGetAnchoredRightX(layout, MAIN_MENU_FOOTER_RIGHT_MARGIN, len), mainMenuGetAnchoredY(layout, MAIN_MENU_BUILD_HASH_Y), versionFontSettings | (DRAW_TEXT_FLAG_NO_BG | DRAW_TEXT_FLAG_OVERFLOW));

    char buildDate[VERSION_MAX] = "DATE: ";
    strcat(buildDate, _BUILD_DATE);
    len = fontGetStringWidth(buildDate);
    windowDrawText(gMainMenuWindow, buildDate, 0, mainMenuGetAnchoredRightX(layout, MAIN_MENU_FOOTER_RIGHT_MARGIN, len), mainMenuGetAnchoredY(layout, MAIN_MENU_BUILD_DATE_Y), versionFontSettings | (DRAW_TEXT_FLAG_NO_BG | DRAW_TEXT_FLAG_OVERFLOW));

    mainMenuMaskBackground(0, layout.screenHeight - 70, layout.screenWidth, 70);
}

static bool mainMenuCreateButtons(const MainMenuLayout& layout, const MainMenuOffsets& offsets)
{
    Size buttonSize = mainMenuTransformSize(layout, MAIN_MENU_BUTTON_WIDTH, MAIN_MENU_BUTTON_HEIGHT);
    int buttonWidth = buttonSize.width;
    int buttonHeight = buttonSize.height;
    unsigned char* buttonNormalData;
    unsigned char* buttonPressedData;
    if (buttonWidth == MAIN_MENU_BUTTON_WIDTH && buttonHeight == MAIN_MENU_BUTTON_HEIGHT) {
        buttonNormalData = mainMenuButtonNormalFrmImage.getData();
        buttonPressedData = mainMenuButtonPressedFrmImage.getData();
    } else {
        mainMenuScaledButtonData.assign(static_cast<size_t>(buttonWidth) * buttonHeight * 2, 0);
        buttonNormalData = mainMenuScaledButtonData.data();
        buttonPressedData = buttonNormalData + buttonWidth * buttonHeight;

        blitBuffer2DScaledTrans(mainMenuButtonNormalFrmImage.getBuffer(), Buffer2D(buttonNormalData, buttonWidth, buttonHeight), 0, 0, buttonWidth, buttonHeight);
        blitBuffer2DScaledTrans(mainMenuButtonPressedFrmImage.getBuffer(), Buffer2D(buttonPressedData, buttonWidth, buttonHeight), 0, 0, buttonWidth, buttonHeight);
    }

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        Point buttonPosition = mainMenuTransformPoint(layout, MAIN_MENU_BUTTON_X, MAIN_MENU_BUTTON_Y + index * MAIN_MENU_BUTTON_Y_STEP);
        int buttonX = buttonPosition.x + offsets.menuX;
        int buttonY = buttonPosition.y + offsets.menuY;
        if (buttonX < 0 || buttonY < 0 || buttonX + buttonWidth > layout.screenWidth || buttonY + buttonHeight > layout.screenHeight) {
            return false;
        }
        gMainMenuButtons[index] = buttonCreate(gMainMenuWindow,
            buttonX,
            buttonY,
            buttonWidth,
            buttonHeight,
            -1,
            -1,
            1111,
            mainMenuButtonSpecs[index].keyCode,
            buttonNormalData,
            buttonPressedData,
            nullptr,
            BUTTON_FLAG_TRANSPARENT);
        if (gMainMenuButtons[index] == -1) {
            return false;
        }

        mainMenuMaskBackground(buttonX, buttonY, buttonWidth, buttonHeight);

        buttonSetMask(gMainMenuButtons[index], buttonNormalData);
    }

    return true;
}

static void mainMenuDrawButtonLabels(const MainMenuLayout& layout, const MainMenuOffsets& offsets)
{
    MessageListItem msg;
    ColorWithFlags fontSettings = COLOR_YELLOW_2 | DRAW_TEXT_FLAG_NONE;
    // Unset reads as 0: COLOR_BLACK is a real palette index (228 on the
    // vanilla palette), so using it as the sentinel made the guard below
    // always true and the labels came out black instead of yellow.
    ColorWithFlags fontSettingsSFall = static_cast<ColorWithFlags>(0);
    configGetEnum<ColorWithFlags>(&gContentConfig, CONTENT_CONFIG_MAIN_MENU_SECTION, "big_font_color", &fontSettingsSFall);
    if (fontSettingsSFall) {
        fontSettings = static_cast<Color>(fontSettingsSFall & COLOR_LAST) | DRAW_TEXT_FLAG_NONE;
    }

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        msg.num = mainMenuButtonSpecs[index].labelMessage;
        if (!messageListGetItem(&gMiscMessageList, &msg)) {
            continue;
        }

        int len = fontGetStringWidth(msg.text);
        Point labelPosition = mainMenuTransformPoint(layout, MAIN_MENU_BUTTON_LABEL_X, MAIN_MENU_BUTTON_LABEL_Y + index * MAIN_MENU_BUTTON_Y_STEP);
        if (!layout.scaleControls) {
            int labelY = labelPosition.y + offsets.menuY;
            int labelX = labelPosition.x + offsets.menuX - len / 2;
            if (labelY < 0 || labelY + fontGetLineHeight() > layout.screenHeight || len > layout.screenWidth) {
                continue;
            }
            labelX = std::clamp(labelX, 0, layout.screenWidth - len);
            fontDrawText(gMainMenuWindowBuffer + labelY * layout.screenWidth + labelX,
                msg.text,
                layout.screenWidth - labelX,
                layout.screenWidth,
                fontSettings);
        } else {
            int scaledLen = interfaceFontGetStringWidthScaled(msg.text, fontSettings, layout.scale);
            interfaceFontDrawTextScaled2D(Buffer2D(gMainMenuWindowBuffer, layout.screenWidth, layout.screenHeight),
                labelPosition.x + offsets.menuX - scaledLen / 2,
                labelPosition.y + offsets.menuY,
                msg.text,
                fontSettings,
                layout.scale);
        }

        mainMenuMaskBackground(labelPosition.x + offsets.menuX - len / 2,
            labelPosition.y + offsets.menuY - 2,
            len + 4,
            fontGetLineHeight() + 4);
    }
}

// 0x481650 main_menu_create
int mainMenuWindowInit()
{
    if (gMainMenuWindowInitialized) {
        return 0;
    }

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        gMainMenuButtons[index] = -1;
    }

    colorPaletteLoad("color.pal");

    gMainMenuWindow = windowCreate(0,
        0,
        screenGetWidth(),
        screenGetHeight(),
        COLOR_FIRST,
        WINDOW_HIDDEN | WINDOW_MOVE_ON_TOP);
    if (gMainMenuWindow == -1) {
        return main_menu_fatal_error();
    }

    gMainMenuWindowBuffer = windowGetBuffer(gMainMenuWindow);

    if (!mainMenuLoadArt()) {
        return main_menu_fatal_error();
    }

    MainMenuLayout layout = mainMenuBuildLayout();
    mainMenuPrepareRgbaBackground(layout);
    mainMenuOverlayBackgroundEnabled = mainMenuShouldEnableOverlayBackground(layout);
    MainMenuOffsets offsets = mainMenuReadOffsets(layout);

    mainMenuDrawBackground(layout);
    mainMenuDrawPanel(layout, offsets);

    int oldFont = fontGetCurrent();
    fontSetCurrent(100);
    mainMenuDrawBuildInfo(layout, offsets);

    if (!mainMenuCreateButtons(layout, offsets)) {
        return main_menu_fatal_error();
    }

    fontSetCurrent(104);
    mainMenuDrawButtonLabels(layout, offsets);

    fontSetCurrent(oldFont);
    mainMenuMaskBackground(std::max(0, layout.backgroundX),
        layout.backgroundY + static_cast<int>(layout.backgroundHeight * 0.2f),
        std::min(layout.screenWidth, static_cast<int>(layout.backgroundWidth * 0.28f)),
        static_cast<int>(layout.backgroundHeight * 0.56f));
    mainMenuBackgroundFrmImage.unlock();
    mainMenuButtonPanelFrmImage.unlock();

    gMainMenuWindowInitialized = true;
    gMainMenuWindowHidden = true;
    mainMenuOverlayCount = 0;
    mainMenuOverlayDimmedBackup.clear();

    return 0;
}

static void mainMenuDestroySubscreenBackdrop()
{
    if (mainMenuSubscreenBackdrop != -1) {
        windowDestroy(mainMenuSubscreenBackdrop);
        mainMenuSubscreenBackdrop = -1;
    }
}

// 0x481968 main_menu_destroy
void mainMenuWindowFree()
{
    renderClearRgbaBackground();
    mainMenuDestroySubscreenBackdrop();

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        if (gMainMenuButtons[index] != -1) {
            buttonDestroy(gMainMenuButtons[index]);
            gMainMenuButtons[index] = -1;
        }
    }

    mainMenuScaledButtonData.clear();
    mainMenuOverlayDimmedBackup.clear();
    mainMenuButtonPanelFrmImage.unlock();
    mainMenuButtonPressedFrmImage.unlock();
    mainMenuButtonNormalFrmImage.unlock();
    mainMenuBackgroundFrmImage.unlock();

    if (gMainMenuWindow != -1) {
        windowDestroy(gMainMenuWindow);
    }

    gMainMenuWindowInitialized = false;
    gMainMenuWindow = -1;
    gMainMenuWindowBuffer = nullptr;
    gMainMenuWindowHidden = true;
    mainMenuOverlayCount = 0;
    mainMenuOverlayBackgroundEnabled = false;
}

// 0x481A00 main_menu_hide
void mainMenuWindowHide(bool animate)
{
    if (!gMainMenuWindowInitialized) {
        return;
    }

    if (gMainMenuWindowHidden) {
        return;
    }

    soundContinueAll();
    renderClearRgbaBackground();

    if (animate) {
        paletteFadeTo(gPaletteBlack);
        soundContinueAll();
    }

    windowHide(gMainMenuWindow);
    touch_set_touchscreen_mode(false);
    mainMenuOverlayDimmedBackup.clear();
    mainMenuOverlayCount = 0;

    gMainMenuWindowHidden = true;
}

// 0x481A48 main_menu_show
void mainMenuWindowUnhide(bool animate)
{
    if (!gMainMenuWindowInitialized) {
        return;
    }

    if (!gMainMenuWindowHidden) {
        return;
    }

    windowShow(gMainMenuWindow);
    mainMenuEnableRgbaBackground();
    touch_set_touchscreen_mode(true);

    if (animate) {
        colorPaletteLoad("color.pal");
        paletteFadeTo(_cmap);
    }

    gMainMenuWindowHidden = false;
}

static void mainMenuWindowEnterOverlay()
{
    if (!gMainMenuWindowInitialized || gMainMenuWindowHidden) {
        return;
    }

    if (mainMenuOverlayCount == 0) {
        mainMenuSetButtonsEnabled(false);
        renderClearRgbaBackground();
    }

    mainMenuOverlayCount += 1;
}

static void mainMenuWindowLeaveOverlay()
{
    if (!gMainMenuWindowInitialized || gMainMenuWindowHidden) {
        mainMenuOverlayCount = 0;
        return;
    }

    assert(mainMenuOverlayCount > 0);
    if (mainMenuOverlayCount <= 0) {
        mainMenuOverlayCount = 0;
        return;
    }

    mainMenuOverlayCount -= 1;
    if (mainMenuOverlayCount == 0) {
        mainMenuRemoveOverlayDim();
        mainMenuSetButtonsEnabled(true);
        mainMenuEnableRgbaBackground();
        touch_set_touchscreen_mode(true);
    }
}

static bool mainMenuWindowIsOverlayActive()
{
    return gMainMenuWindowInitialized && !gMainMenuWindowHidden && mainMenuOverlayCount > 0;
}

static void mainMenuWindowShowOverlayDim()
{
    if (!mainMenuWindowIsOverlayActive() || !mainMenuOverlayDimmedBackup.empty()) {
        return;
    }

    mainMenuApplyOverlayDim();
}

void mainMenuBeginSubscreen()
{
    if (mainMenuOverlayBackgroundEnabled) {
        mainMenuWindowEnterOverlay();
        return;
    }

    mainMenuWindowHide(true);
    if (mainMenuSubscreenBackdrop == -1) {
        // Palette index 0 in the root window is grey, so cover it around 640x480 dialogs.
        mainMenuSubscreenBackdrop = windowCreate(0, 0, screenGetWidth(), screenGetHeight(), COLOR_BLACK, 0);
        if (mainMenuSubscreenBackdrop != -1) {
            windowRefresh(mainMenuSubscreenBackdrop);
        }
    }
}

void mainMenuCancelSubscreen()
{
    if (mainMenuWindowIsOverlayActive()) {
        mainMenuWindowLeaveOverlay();
    }

    mainMenuDestroySubscreenBackdrop();
}

void mainMenuFinishSubscreen()
{
    if (mainMenuWindowIsOverlayActive()) {
        mainMenuWindowHide(false);
    }

    mainMenuWindowFree();
}

int mainMenuSubscreenWindowFlags(int defaultFlags, int overlayFlags)
{
    return mainMenuWindowIsOverlayActive() ? overlayFlags : defaultFlags;
}

void mainMenuShowSubscreen(bool animate)
{
    if (mainMenuWindowIsOverlayActive()) {
        renderPresent();
        mainMenuWindowShowOverlayDim();
    } else if (animate) {
        colorPaletteLoad("color.pal");
        paletteFadeTo(_cmap);
    }
}

void mainMenuFadeOutForMenuReturn(bool animate)
{
    if (!animate || mainMenuWindowIsOverlayActive()) {
        return;
    }

    paletteFadeTo(gPaletteBlack);
}

void mainMenuFadeOutForGameStart(bool animate)
{
    if (!animate) {
        return;
    }

    paletteFadeTo(gPaletteBlack);
}

// 0x481AA8 main_menu_is_enabled
int _main_menu_is_enabled()
{
    return 1;
}

// 0x481AEC main_menu_loop
int mainMenuWindowHandleEvents()
{
    _in_main_menu = true;

    bool oldCursorIsHidden = cursorIsHidden();
    if (oldCursorIsHidden) {
        mouseShowCursor();
    }

    unsigned int tick = getTicks();

    int rc = -1;
    while (rc == -1) {
        if (gMainMenuExitRequested) {
            gMainMenuExitRequested = false;
            _game_user_wants_to_quit = GAME_QUIT_REQUEST_EXIT;
        }

        sharedFpsLimiter.mark();

        int keyCode = inputGetInput();

        for (int buttonIndex = 0; buttonIndex < MAIN_MENU_BUTTON_COUNT; buttonIndex++) {
            int buttonKeyCode = mainMenuButtonSpecs[buttonIndex].keyCode;
            if (keyCode == buttonKeyCode || keyCode == toupper(buttonKeyCode)) {
                // NOTE: Uninline.
                main_menu_play_sound("nmselec1");

                rc = mainMenuButtonSpecs[buttonIndex].returnValue;

                if (buttonIndex == MAIN_MENU_BUTTON_CREDITS && (gPressedPhysicalKeys[SDL_SCANCODE_RSHIFT] != KEY_STATE_UP || gPressedPhysicalKeys[SDL_SCANCODE_LSHIFT] != KEY_STATE_UP)) {
                    rc = MAIN_MENU_QUOTES;
                }

                break;
            }
        }

        if (rc == -1) {
            if (keyCode == KEY_CTRL_R) {
                rc = MAIN_MENU_SELFRUN;
                continue;
            } else if (keyCode == KEY_PLUS || keyCode == KEY_EQUAL) {
                brightnessIncrease();
            } else if (keyCode == KEY_MINUS || keyCode == KEY_UNDERSCORE) {
                brightnessDecrease();
            } else if (keyCode == KEY_UPPERCASE_D || keyCode == KEY_LOWERCASE_D) {
                rc = MAIN_MENU_SCREENSAVER;
                continue;
            } else if (keyCode == 1111) {
                if (!(mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT)) {
                    // NOTE: Uninline.
                    main_menu_play_sound("nmselec0");
                }
                continue;
            }
        }

        if (keyCode == KEY_ESCAPE || _game_user_wants_to_quit == GAME_QUIT_REQUEST_EXIT) {
            rc = MAIN_MENU_EXIT;

            // NOTE: Uninline.
            main_menu_play_sound("nmselec1");
            break;
        } else if (_game_user_wants_to_quit == GAME_QUIT_REQUEST_MAIN_MENU) {
            _game_user_wants_to_quit = GAME_QUIT_REQUEST_NONE;
        } else {
            if (getTicksSince(tick) >= gMainMenuScreensaverDelay) {
                rc = MAIN_MENU_TIMEOUT;
            }
        }

        renderPresent();
        sharedFpsLimiter.throttle();
    }

    if (oldCursorIsHidden) {
        mouseHideCursor();
    }

    _in_main_menu = false;

    return rc;
}

void mainMenuRequestExit()
{
    gMainMenuExitRequested = true;
}

// NOTE: Inlined.
//
// 0x481C88 main_menu_fatal_error
static int main_menu_fatal_error()
{
    mainMenuWindowFree();

    return -1;
}

// NOTE: Inlined.
//
// 0x481C94 main_menu_play_sound
static void main_menu_play_sound(const char* fileName)
{
    soundPlayFile(fileName);
}

static void mainMenuSetButtonsEnabled(bool enabled)
{
    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        if (gMainMenuButtons[index] == -1) {
            continue;
        }

        if (enabled) {
            buttonEnable(gMainMenuButtons[index]);
        } else {
            buttonDisable(gMainMenuButtons[index]);
        }
    }
}

static void mainMenuApplyOverlayDim()
{
    if (gMainMenuWindow == -1 || gMainMenuWindowBuffer == nullptr) {
        return;
    }

    int width = windowGetWidth(gMainMenuWindow);
    int height = windowGetHeight(gMainMenuWindow);
    int size = width * height;

    mainMenuOverlayDimmedBackup.assign(gMainMenuWindowBuffer, gMainMenuWindowBuffer + size);
    mainMenuAnimateOverlayDim(mainMenuOverlayDimClearIntensity, mainMenuOverlayDimTargetIntensity);
}

static void mainMenuRemoveOverlayDim()
{
    if (gMainMenuWindow == -1 || gMainMenuWindowBuffer == nullptr || mainMenuOverlayDimmedBackup.empty()) {
        return;
    }

    int width = windowGetWidth(gMainMenuWindow);
    int height = windowGetHeight(gMainMenuWindow);
    int size = width * height;
    if (static_cast<int>(mainMenuOverlayDimmedBackup.size()) != size) {
        mainMenuOverlayDimmedBackup.clear();
        return;
    }

    bool cursorWasHidden = cursorIsHidden();
    if (cursorWasHidden) {
        mouseShowCursor();
    }

    mainMenuAnimateOverlayDim(mainMenuOverlayDimTargetIntensity, mainMenuOverlayDimClearIntensity);

    if (cursorWasHidden) {
        mouseHideCursor();
    }

    memcpy(gMainMenuWindowBuffer, mainMenuOverlayDimmedBackup.data(), size);
    mainMenuOverlayDimmedBackup.clear();
    windowRefresh(gMainMenuWindow);
}

static void mainMenuSetOverlayDimIntensity(int intensity)
{
    if (gMainMenuWindow == -1 || gMainMenuWindowBuffer == nullptr || mainMenuOverlayDimmedBackup.empty()) {
        return;
    }

    int width = windowGetWidth(gMainMenuWindow);
    int height = windowGetHeight(gMainMenuWindow);
    _dark_trans_buf_to_buf(mainMenuOverlayDimmedBackup.data(), width, height, width, gMainMenuWindowBuffer, 0, 0, width, intensity);
    windowRefresh(gMainMenuWindow);
    renderPresent();
}

static void mainMenuAnimateOverlayDim(int startIntensity, int endIntensity)
{
    for (int step = 0; step < mainMenuOverlayDimAnimationSteps; step++) {
        _GNW95_process_message();
        _mouse_info();

        int intensity = startIntensity + (endIntensity - startIntensity) * (step + 1) / mainMenuOverlayDimAnimationSteps;
        mainMenuSetOverlayDimIntensity(intensity);

        if (step + 1 < mainMenuOverlayDimAnimationSteps) {
            delay_ms(mainMenuOverlayDimFrameDelay);
        }
    }
}

} // namespace fallout
