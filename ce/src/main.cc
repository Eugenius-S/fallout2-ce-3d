#include "main.h"

#include <lodepng.h>

#include <algorithm>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "art.h"
#include "autorun.h"
#include "character_selector.h"
#include "color.h"
#include "content_config.h"
#include "credits.h"
#include "cycle.h"
#include "db.h"
#include "debug.h"
#include "draw.h"
#include "endgame.h"
#include "game.h"
#include "game_mouse.h"
#include "game_movie.h"
#include "game_sound.h"
#include "input.h"
#include "kb.h"
#include "loadsave.h"
#include "mainmenu.h"
#include "map.h"
#include "mouse.h"
#include "owner_art_viewport.h"
#include "object.h"
#include "palette.h"
#include "platform_compat.h"
#include "preferences.h"
#include "proto.h"
#include "random.h"
#include "scripts.h"
#include "settings.h"
#include "sfall_callbacks.h"
#include "sfall_global_scripts.h"
#include "svga.h"
#include "text_font.h"
#include "window.h"
#include "window_manager.h"
#include "window_manager_private.h"
#include "word_wrap.h"
#include "devtools/devtools.h"
#include "scene/scene_view.h"
#include "worldmap.h"

namespace fallout {

static bool falloutInit(int argc, char** argv);
static int main_reset_system();
static void main_exit_system();
static int _main_load_new(char* fname);
static int main_loadgame_new();
static void main_unload_new();
static void mainParseCommandLineArguments(int argc, char** argv);
static bool mainTryParseDevLoadGameSlot(const char* value, int* slotPtr);
static bool mainLoadOwnerWideArt(const char* assetPath, int width, int height, const OwnerArtViewport& viewport,
    std::vector<unsigned char>& rgba, std::vector<unsigned char>& mask);
static void mainMaskRgbaRect(std::vector<unsigned char>& mask, int width, int height, int x, int y, int rectWidth, int rectHeight);
static void mainHandleDevEndgameRequests();
static void mainRequestDevEndgameIfNeeded();
static void mainRunDevEndgameMovieIfNeeded();
static void mainLoop();
static void mainRunGameLoopAndCleanup();
static void showDeath();
static void _main_death_voiceover_callback();
static int _mainDeathGrabTextFile(const char* fileName, char* dest);
static int _mainDeathWordWrap(char* text, int width, short* beginnings, short* count);

// 0x5194C8 mainMap
static char _mainMap[] = "artemple.map";

// 0x5194D8 main_game_paused
static int _main_game_paused = 0;

// 0x5194E8 main_show_death_scene
static bool _main_show_death_scene = false;

// 0x614838 main_death_voiceover_done
static bool _main_death_voiceover_done;

static int commandLineDevLoadGameSlot = -1;
static bool commandLineDevEndgame = false;
static bool commandLineDevEndgameMovie = false;
static bool commandLineDevNewGame = false;

// 0x48099C
int falloutMain(int argc, char** argv)
{
    if (!devtools::initializeFixedScreenOptions(argc, argv)) {
        return 1;
    }
    if (!autorunMutexCreate()) {
        return 1;
    }

    if (!falloutInit(argc, argv)) {
        return 1;
    }

    mainParseCommandLineArguments(argc, argv);
    devtools::applyBackgroundGlow();
    devtools::applyBackgroundShadows();
    devtools::applyBackgroundZoom();
    devtools::prepareStillSnapshot();

    // Sprite-lighting mod: `--dev-list-maps` prints the maps available in
    // game resources and exits.
    if (devtools::wantsListMaps()) {
        devtools::listMaps();
        main_exit_system();
        autorunMutexClose();
        return 0;
    }

    // SFALL: Allow to skip intro movies
    int skipOpeningMovies = settings.ui.skip_opening_movies;
    if (skipOpeningMovies < 1) {
        gameMoviePlay(MOVIE_IPLOGO, GAME_MOVIE_FADE_IN);
        gameMoviePlay(MOVIE_INTRO, 0);
        gameMoviePlay(MOVIE_CREDITS, 0);
    } else {
        // If the splash is shown but opening movies are skipped, fade it out
        // before the main menu starts its normal fade-in.
        paletteFadeTo(gPaletteBlack);
    }

    if (mainMenuWindowInit() == 0) {
        bool done = false;
        while (!done) {
            keyboardReset();
            if (!backgoundSoundIsPlaying()) {
                _gsound_background_play_level_music(gameSoundGetMusicOverride("main_menu_music", "07desert"), GSOUND_LIMIT_BEFORE);
            }
            mainMenuWindowUnhide(true);

            mouseShowCursor();
            int devLoadGameSlot = commandLineDevLoadGameSlot;
            bool devNewGame = commandLineDevNewGame;
            commandLineDevNewGame = false;
            int mainMenuRc;
            if (devNewGame) {
                mainMenuRc = MAIN_MENU_NEW_GAME;
            } else if (devLoadGameSlot != -1) {
                commandLineDevLoadGameSlot = -1;
                mainMenuRc = MAIN_MENU_LOAD_GAME;
            } else {
                mainMenuRc = mainMenuWindowHandleEvents();
            }
            mouseHideCursor();

            switch (mainMenuRc) {
            case MAIN_MENU_INTRO:
                mainMenuWindowHide(true);
                gameMoviePlay(MOVIE_INTRO, GAME_MOVIE_STOP_MUSIC);
                gameMoviePlay(MOVIE_CREDITS, 0);
                break;
            case MAIN_MENU_NEW_GAME: {
                mainMenuBeginSubscreen();
                if (characterSelectorOpen(devNewGame) == 2) {
                    mainMenuFinishSubscreen();

                    if (!devNewGame) {
                        gameMoviePlay(MOVIE_ELDER, GAME_MOVIE_STOP_MUSIC);
                    }
                    randomSeedPrerandom(-1);
                    devtools::prepareStillSnapshot();

                    // SFALL: Call "before start" event
                    sfallOnBeforeGameStart();

                    // SFALL: Override starting map.
                    char* mapName = nullptr;
                    configGetString(&gContentConfig, CONTENT_CONFIG_START_SECTION, "map", &mapName, nullptr);

                    char* mapNameCopy = compat_strdup(mapName != nullptr ? mapName : _mainMap);
                    _main_load_new(mapNameCopy);
                    free(mapNameCopy);

                    // SFALL: AfterNewGameStartHook.
                    sfall_gl_scr_exec_start_proc();
                    // SFALL: Call "after loading" event
                    sfallOnAfterNewGame();
                    sfallOnAfterGameStarted();
                    gGameLoaded = true;

                    // Sprite-lighting mod: a dev new game travels to the
                    // --dev-map target like a loaded save does.
                    if (devNewGame) {
                        devtools::applyStartupTarget();
                    }

                    mainRunGameLoopAndCleanup();
                    mainMenuWindowInit();
                } else {
                    mainMenuCancelSubscreen();
                }
                break;
            }
            case MAIN_MENU_LOAD_GAME: {
                mainMenuBeginSubscreen();

                // NOTE: Uninline.
                main_loadgame_new();

                if (devLoadGameSlot != -1) {
                    lsgDevSetLoadGameSlot(devLoadGameSlot);
                }
                int loadGameRc = lsgLoadGame(LOAD_SAVE_MODE_FROM_MAIN_MENU);
                if (loadGameRc == -1) {
                    debugPrint("\n ** Error running LoadGame()! **\n");
                }

                if (loadGameRc > 0) {
                    mainMenuFinishSubscreen();
                    // Sprite-lighting mod: travel to the --dev-map target
                    // through the engine's own safe transition.
                    devtools::applyStartupTarget();
                    mainRunGameLoopAndCleanup();
                    mainMenuWindowInit();
                } else {
                    main_unload_new();
                    mainMenuCancelSubscreen();
                    if (loadGameRc < 0) {
                        main_reset_system();
                    }
                }
                break;
            }
            case MAIN_MENU_TIMEOUT:
                debugPrint("Main menu timed-out\n");
                // FALLTHROUGH
            case MAIN_MENU_SCREENSAVER:
                mainMenuWindowHide(true);
                gameMoviePlay(MOVIE_INTRO, GAME_MOVIE_PAUSE_MUSIC);
                break;
            case MAIN_MENU_OPTIONS: {
                mainMenuBeginSubscreen();
                doPreferences(true);
                mainMenuCancelSubscreen();
                break;
            }
            case MAIN_MENU_CREDITS:
                mainMenuWindowHide(true);
                creditsOpen("credits.txt", InterfaceFrameId::Invalid, false);
                break;
            case MAIN_MENU_QUOTES:
                // NOTE: There is a strange cmp at 0x480C50. Both operands are
                // zero, set before the loop and do not modify afterwards. For
                // clarity this condition is omitted.
                mainMenuWindowHide(true);
                creditsOpen("quotes.txt", InterfaceFrameId::Invalid, true);
                break;
            case MAIN_MENU_EXIT:
            case -1:
                done = true;
                mainMenuWindowHide(true);
                mainMenuWindowFree();
                backgroundSoundDelete();
                break;
            case MAIN_MENU_SELFRUN:
                break;
            }
        }
    }

    // NOTE: Uninline.
    main_exit_system();

    autorunMutexClose();

    return 0;
}

// 0x480CC0
static bool falloutInit(int argc, char** argv)
{
    // set flag to 1 to initialize _screen_buffer for WINDOW_TRANSPARENT
    if (gameInitWithOptions("FALLOUT II", false, 0, WINDOW_MANAGER_INIT_FLAG_BUFFERED, argc, argv) == -1) {
        return false;
    }

    return true;
}

static void mainParseCommandLineArguments(int argc, char** argv)
{
    const char* devLoadGamePrefix = "--dev-load-game=";
    size_t devLoadGamePrefixLength = strlen(devLoadGamePrefix);

    for (int arg = 1; arg < argc; arg += 1) {
        if (devtools::parseOption(argv[arg])) {
            continue;
        }
        if (strncmp(argv[arg], devLoadGamePrefix, devLoadGamePrefixLength) == 0) {
            int slot;
            if (mainTryParseDevLoadGameSlot(argv[arg] + devLoadGamePrefixLength, &slot)) {
                commandLineDevLoadGameSlot = slot;
            } else {
                debugPrint("MAIN: invalid --dev-load-game value '%s'\n", argv[arg] + devLoadGamePrefixLength);
            }
        } else if (strcmp(argv[arg], "--dev-new-game") == 0) {
            commandLineDevNewGame = true;
        } else if (strcmp(argv[arg], "--dev-endgame") == 0) {
            commandLineDevEndgame = true;
        } else if (strcmp(argv[arg], "--dev-endgame-movie") == 0) {
            commandLineDevEndgameMovie = true;
        }
    }
}

static bool mainTryParseDevLoadGameSlot(const char* value, int* slotPtr)
{
    if (value == nullptr || slotPtr == nullptr) {
        return false;
    }

    char* end = nullptr;
    long slotNumber = strtol(value, &end, 10);
    if (end == value || *end != '\0' || slotNumber < 1 || slotNumber > lsgGetTotalSlotCount()) {
        return false;
    }

    *slotPtr = static_cast<int>(slotNumber - 1);
    return true;
}

static void mainHandleDevEndgameRequests()
{
    if (commandLineDevEndgame && commandLineDevEndgameMovie) {
        commandLineDevEndgame = false;
        commandLineDevEndgameMovie = false;
        endgamePlaySlideshow();
        endgamePlayMovie();
        return;
    }

    mainRequestDevEndgameIfNeeded();
    mainRunDevEndgameMovieIfNeeded();
}

static void mainRequestDevEndgameIfNeeded()
{
    if (!commandLineDevEndgame) {
        return;
    }

    commandLineDevEndgame = false;
    scriptsRequestEndgame();
}

static void mainRunDevEndgameMovieIfNeeded()
{
    if (!commandLineDevEndgameMovie) {
        return;
    }

    commandLineDevEndgameMovie = false;
    endgamePlayMovie();
}

// NOTE: Inlined.
//
// 0x480D0C
static int main_reset_system()
{
    gameReset();

    return 1;
}

// NOTE: Inlined.
//
// 0x480D18
static void main_exit_system()
{
    backgroundSoundDelete();

    gameExit();
}

// 0x480D4C
static int _main_load_new(char* mapFileName)
{
    _game_user_wants_to_quit = GAME_QUIT_REQUEST_NONE;
    _main_show_death_scene = 0;
    gDude->flags &= ~OBJECT_FLAT;
    objectShow(gDude, nullptr);
    mouseHideCursor();

    int win = windowCreate(0, 0, screenGetWidth(), screenGetHeight(), COLOR_BLACK, WINDOW_MODAL | WINDOW_MOVE_ON_TOP);
    windowRefresh(win);

    colorPaletteLoad("color.pal");
    paletteFadeTo(_cmap);
    mapInit();
    gameMouseSetCursor(MOUSE_CURSOR_NONE);
    mouseShowCursor();
    mapLoadByName(mapFileName);

    // SFALL: Fix the starting position of the player's marker on the world map
    // when starting a new game with a custom starting map.
    City areaIdx;
    if (wmMatchAreaContainingMapIdx(gMapHeader.index, &areaIdx) == 0) {
        if (wmStartWorldPosIsConfigured()) {
            wmSetPartyCurArea(areaIdx);
            wmClearPartyWalking();
        } else {
            wmTeleportToArea(areaIdx);
        }
    }

    wmMapMusicStart();
    paletteFadeTo(gPaletteWhite);
    windowDestroy(win);
    colorPaletteLoad("color.pal");
    paletteFadeTo(_cmap);
    return 0;
}

// NOTE: Inlined.
//
// 0x480DF8
static int main_loadgame_new()
{
    _game_user_wants_to_quit = GAME_QUIT_REQUEST_NONE;
    _main_show_death_scene = 0;

    gDude->flags &= ~OBJECT_FLAT;

    objectShow(gDude, nullptr);
    mouseHideCursor();

    mapInit();

    gameMouseSetCursor(MOUSE_CURSOR_NONE);
    mouseShowCursor();

    return 0;
}

// 0x480E34
static void main_unload_new()
{
    objectHide(gDude, nullptr);
    mapExit();
}

// 0x480E48
static void mainLoop()
{
    bool cursorWasHidden = cursorIsHidden();
    if (cursorWasHidden) {
        mouseShowCursor();
    }

    _main_game_paused = 0;

    scriptsEnable();

    while (_game_user_wants_to_quit == GAME_QUIT_REQUEST_NONE) {
        sharedFpsLimiter.mark();

        int keyCode = inputGetInput();

        // SFALL: MainLoopHook.
        sfall_gl_scr_process_main();

        gameHandleKey(keyCode, false);

        scriptsHandleRequests();

        mapHandleTransition();

        // Sprite-lighting mod: dev-panel pending elevation corrections are
        // applied here - after a completed mapHandleTransition, in the main
        // loop (never from the draw path, which also runs during loads).
        devtools::update();
        scene::updateCameraTurn();
        if (devtools::sceneStartupReady()) {
            scene::syncSettings(devtools::suppressInitialSceneEnable());
        }

        if (_main_game_paused != 0) {
            _main_game_paused = 0;
        }

        if ((gDude->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
            endgameSetupDeathEnding(ENDGAME_DEATH_ENDING_REASON_DEATH);
            _main_show_death_scene = 1;
            _game_user_wants_to_quit = GAME_QUIT_REQUEST_MAIN_MENU;
        }

        renderFpsCounter();
        renderPresent();
        sharedFpsLimiter.throttle();
    }

    scriptsDisable();

    if (cursorWasHidden) {
        mouseHideCursor();
    }
}

static void mainRunGameLoopAndCleanup()
{
    mainHandleDevEndgameRequests();
    mainLoop();
    paletteFadeTo(gPaletteWhite);

    // NOTE: Uninline.
    main_unload_new();

    // NOTE: Uninline.
    main_reset_system();

    if (_main_show_death_scene != 0) {
        showDeath();
        _main_show_death_scene = 0;
    }
}

// 0x48118C
static bool mainLoadOwnerWideArt(const char* assetPath, int width, int height, const OwnerArtViewport& viewport,
    std::vector<unsigned char>& rgba, std::vector<unsigned char>& mask)
{
    unsigned int sourceWidth = 0;
    unsigned int sourceHeight = 0;
    std::vector<unsigned char> source;
    unsigned error = lodepng::decode(source, sourceWidth, sourceHeight, assetPath);
    if (error != 0) {
        std::string gameRelativePath = std::string("game/") + assetPath;
        error = lodepng::decode(source, sourceWidth, sourceHeight, gameRelativePath);
    }
    if (error != 0 || sourceWidth == 0 || sourceHeight == 0 || width <= 0 || height <= 0) {
        debugPrint("MAIN: owner wide art unavailable (%s)\n", lodepng_error_text(error));
        return false;
    }

    int drawWidth;
    int drawHeight;
    if (static_cast<int64_t>(sourceWidth) * viewport.height > static_cast<int64_t>(sourceHeight) * viewport.width) {
        // Cover the screen without leaving a seam from integer truncation.
        drawHeight = viewport.height;
        drawWidth = static_cast<int>((static_cast<int64_t>(viewport.height) * sourceWidth + sourceHeight - 1) / sourceHeight);
    } else {
        drawWidth = viewport.width;
        drawHeight = static_cast<int>((static_cast<int64_t>(viewport.width) * sourceHeight + sourceWidth - 1) / sourceWidth);
    }

    rgba.assign(static_cast<size_t>(width) * height * 4, 0);
    mask.assign(static_cast<size_t>(width) * height, 0);
    int left = viewport.x + (viewport.width - drawWidth) / 2;
    int top = viewport.y + (viewport.height - drawHeight) / 2;
    for (int y = 0; y < drawHeight; y++) {
        int dstY = top + y;
        if (dstY < 0 || dstY >= height) continue;
        int srcY = static_cast<int64_t>(y) * sourceHeight / drawHeight;
        for (int x = 0; x < drawWidth; x++) {
            int dstX = left + x;
            if (dstX < 0 || dstX >= width) continue;
            int srcX = static_cast<int64_t>(x) * sourceWidth / drawWidth;
            size_t sourceOffset = (static_cast<size_t>(srcY) * sourceWidth + srcX) * 4;
            size_t destOffset = (static_cast<size_t>(dstY) * width + dstX) * 4;
            memcpy(rgba.data() + destOffset, source.data() + sourceOffset, 4);
            mask[static_cast<size_t>(dstY) * width + dstX] = 1;
        }
    }
    return true;
}

static void mainMaskRgbaRect(std::vector<unsigned char>& mask, int width, int height, int x, int y, int rectWidth, int rectHeight)
{
    int left = std::max(0, x);
    int top = std::max(0, y);
    int right = std::min(width, x + rectWidth);
    int bottom = std::min(height, y + rectHeight);
    if (left >= right || top >= bottom) return;
    for (int row = top; row < bottom; row++) {
        memset(mask.data() + static_cast<size_t>(row) * width + left, 0, right - left);
    }
}

static void showDeath()
{
    artCacheFlush();
    colorCycleDisable();
    gameMouseSetCursor(MOUSE_CURSOR_NONE);

    bool oldCursorIsHidden = cursorIsHidden();
    if (oldCursorIsHidden) {
        mouseShowCursor();
    }

    int screenWidth = screenGetWidth();
    int screenHeight = screenGetHeight();
    OwnerArtViewport viewport = ownerArtViewport(screenWidth, screenHeight);
    int deathWindowX = 0;
    int deathWindowY = 0;
    int win = windowCreate(deathWindowX,
        deathWindowY,
        screenWidth,
        screenHeight,
        COLOR_FIRST,
        WINDOW_MOVE_ON_TOP);
    if (win != -1) {
        do {
            unsigned char* windowBuffer = windowGetBuffer(win);
            if (windowBuffer == nullptr) {
                break;
            }

            FrmImage backgroundFrmImage;
            if (!backgroundFrmImage.lock(InterfaceFrameId::DeathScene)) {
                break;
            }

            while (mouseGetEvent() != 0) {
                sharedFpsLimiter.mark();

                inputGetInput();

                renderPresent();
                sharedFpsLimiter.throttle();
            }

            keyboardReset();
            inputEventQueueReset();

            colorPaletteLoad("art\\intrface\\death.pal");
            Rect deathFrameBounds = blitBuffer2DCenteredAspectFitHeightIfWider(backgroundFrmImage.getBuffer(),
                Buffer2D(windowBuffer, screenWidth, screenHeight),
                COLOR_BLACK,
                settings.ui.death_screen_size != 0);

            std::vector<unsigned char> deathRgba;
            std::vector<unsigned char> deathMask;
            bool useOwnerDeathArt = mainLoadOwnerWideArt(
                "user-assets/fullscreen-20261028/wallpapers-selected/common/death/death.png",
                screenWidth,
                screenHeight,
                viewport,
                deathRgba,
                deathMask);
            if (useOwnerDeathArt && viewport.x > 0) {
                bufferFill(windowBuffer, viewport.x, screenHeight, screenWidth, COLOR_BLACK);
                int rightX = viewport.x + viewport.width;
                bufferFill(windowBuffer + rightX, screenWidth - rightX, screenHeight, screenWidth, COLOR_BLACK);
            }

            const char* deathFileName = endgameDeathEndingGetFileName();

            if (settings.preferences.subtitles) {
                char text[512];
                if (_mainDeathGrabTextFile(deathFileName, text) == 0) {
                    debugPrint("\n((ShowDeath)): %s\n", text);

                    short beginnings[WORD_WRAP_MAX_COUNT];
                    short count;
                    int deathFrameWidth = rectGetWidth(&deathFrameBounds);
                    int deathFrameHeight = rectGetHeight(&deathFrameBounds);
                    int textMaxWidth = std::min(560, deathFrameWidth - 80);
                    if (textMaxWidth > 0 && _mainDeathWordWrap(text, textMaxWidth, beginnings, &count) == 0) {
                        int textHeight = fontGetLineHeight() * count;
                        int y = std::max(deathFrameBounds.top, deathFrameBounds.top + deathFrameHeight - textHeight - 8);
                        int x = std::max(2, deathFrameBounds.left + (deathFrameWidth - textMaxWidth) / 2);
                        bufferFill(windowBuffer + screenWidth * y + x - 2, textMaxWidth + 4, textHeight + 2, screenWidth, COLOR_FIRST);
                        if (useOwnerDeathArt) {
                            mainMaskRgbaRect(deathMask, screenWidth, screenHeight, x - 2, y, textMaxWidth + 4, textHeight + 2);
                        }
                        unsigned char* p = windowBuffer + screenWidth * y + x;
                        for (int index = 0; index < count; index++) {
                            fontDrawText(p, text + beginnings[index], textMaxWidth, screenWidth, COLOR_WHITE);
                            p += screenWidth * fontGetLineHeight();
                        }
                    }
                }
            }

            if (useOwnerDeathArt) {
                renderSetRgbaBackground(deathRgba.data(), deathMask.data(), screenWidth, screenHeight);
            }

            windowRefresh(win);

            paletteFadeTo(_cmap);

            _main_death_voiceover_done = false;
            speechSetEndCallback(_main_death_voiceover_callback);

            unsigned int delay;
            if (speechLoad(deathFileName, GSOUND_LOAD_NO_PLAY, GSOUND_STREAM, GSOUND_NO_LOOP) == -1) {
                delay = 3000;
            } else {
                delay = UINT_MAX;
            }

            _gsound_speech_play_preloaded();

            // SFALL: Fix the playback of the speech sound file for the death
            // screen.
            inputBlockForTocks(100);

            unsigned int time = getTicks();
            int keyCode;
            do {
                sharedFpsLimiter.mark();

                keyCode = inputGetInput();

                renderPresent();
                sharedFpsLimiter.throttle();
            } while (keyCode == -1 && !_main_death_voiceover_done && getTicksSince(time) < delay);

            speechSetEndCallback(nullptr);

            speechDelete();

            while (mouseGetEvent() != 0) {
                sharedFpsLimiter.mark();

                inputGetInput();

                renderPresent();
                sharedFpsLimiter.throttle();
            }

            if (keyCode == -1) {
                inputPauseForTocks(500);
            }

            paletteFadeTo(gPaletteBlack);
            renderClearRgbaBackground();
            colorPaletteLoad("color.pal");
        } while (0);
        windowDestroy(win);
    }

    if (oldCursorIsHidden) {
        mouseHideCursor();
    }

    gameMouseSetCursor(MOUSE_CURSOR_ARROW);

    colorCycleEnable();
}

// 0x4814A8
static void _main_death_voiceover_callback()
{
    _main_death_voiceover_done = true;
}

// Read endgame subtitle.
//
// 0x4814B4
static int _mainDeathGrabTextFile(const char* fileName, char* dest)
{
    const char* p = strrchr(fileName, '\\');
    if (p == nullptr) {
        return -1;
    }

    char path[COMPAT_MAX_PATH];
    snprintf(path, sizeof(path), "text\\%s\\cuts\\%s%s", settings.system.language.c_str(), p + 1, ".TXT");

    File* stream = fileOpen(path, "rt");
    if (stream == nullptr) {
        return -1;
    }

    while (true) {
        int c = fileReadChar(stream);
        if (c == -1) {
            break;
        }

        if (c == '\n') {
            c = ' ';
        }

        *dest++ = (c & 0xFF);
    }

    fileClose(stream);

    *dest = '\0';

    return 0;
}

// 0x481598
static int _mainDeathWordWrap(char* text, int width, short* beginnings, short* count)
{
    while (true) {
        char* sep = strchr(text, ':');
        if (sep == nullptr) {
            break;
        }

        if (sep - 1 < text) {
            break;
        }
        sep[0] = ' ';
        sep[-1] = ' ';
    }

    if (wordWrap(text, width, beginnings, count) == -1) {
        return -1;
    }

    // TODO: Probably wrong.
    *count -= 1;

    for (int index = 1; index < *count; index++) {
        char* p = text + beginnings[index];
        while (p >= text && *p != ' ') {
            p--;
            beginnings[index]--;
        }

        if (p != nullptr) {
            *p = '\0';
            beginnings[index]++;
        }
    }

    return 0;
}

} // namespace fallout
