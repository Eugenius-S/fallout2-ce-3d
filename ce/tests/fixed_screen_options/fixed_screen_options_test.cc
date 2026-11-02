#include "../../src/devtools/fixed_screen_options.h"
#include "../../src/scene/display_dimensions.h"
#include "../../src/scene/tile_display_swap.h"
#include "../../src/scene/iso_view_swap.h"

#include <initializer_list>
#include <string>
#include <vector>

using fallout::devtools::FixedScreenOptions;
using fallout::devtools::parseFixedScreenOptions;

static bool parse(std::initializer_list<const char*> arguments, FixedScreenOptions* options, const char** error)
{
    std::vector<std::string> values;
    for (const char* argument : arguments) values.emplace_back(argument);
    std::vector<char*> argv;
    for (std::string& value : values) argv.push_back(value.data());
    return parseFixedScreenOptions(static_cast<int>(argv.size()), argv.data(), options, error);
}

int main()
{
    FixedScreenOptions options;
    const char* error = nullptr;

    int width = 0, height = 0;
    if (!fallout::scene::displayLogicalDimensions(1920, 1080, 1.4, &width, &height)
        || width != 1371 || height != 771) return 18;
    if (!fallout::scene::displayLogicalDimensions(2560, 1440, 2.0, &width, &height)
        || width != 1280 || height != 720) return 19;
    if (fallout::scene::displayLogicalDimensions(1920, 1080, 3.0, &width, &height)
        || width != 1280 || height != 720) return 20;
    unsigned char oldBuffer = 0, newBuffer = 0;
    unsigned char* active = &oldBuffer;
    int tileWidth = 1371, tileHeight = 671, tilePitch = 1371;
    fallout::Rect bounds = { 0, 0, 1370, 670 };
    fallout::scene::TileDisplaySwap tileSwap;
    if (!tileSwap.prepare(&newBuffer, 1280, 620, 1280)
        || !tileSwap.exchange(active, tileWidth, tileHeight, tilePitch, bounds)
        || active != &newBuffer || tileWidth != 1280 || bounds.bottom != 619) return 21;
    if (!tileSwap.exchange(active, tileWidth, tileHeight, tilePitch, bounds)
        || active != &oldBuffer || tileWidth != 1371 || bounds.bottom != 670) return 22;
    fallout::scene::IsoViewSwap isoSwap;
    fallout::Rect newBounds = { 0, 0, 1279, 619 };
    if (!isoSwap.prepare(&newBuffer, newBounds) || !isoSwap.exchange(active, bounds)
        || active != &newBuffer || bounds.right != 1279) return 23;
    if (!isoSwap.exchange(active, bounds) || active != &oldBuffer || bounds.right != 1370) return 24;
    fallout::scene::TextViewSwap textSwap;
    if (!textSwap.prepare(&newBuffer, 1280, 620) || !textSwap.exchange(active, tileWidth, tileHeight)
        || active != &newBuffer || tileWidth != 1280) return 25;
    if (!textSwap.exchange(active, tileWidth, tileHeight) || active != &oldBuffer || tileWidth != 1371) return 26;

    if (!parse({ "game" }, &options, &error) || options.enabled) return 1;
    if (!parse({ "game", "--dev-tools", "--dev-screen=1920x1080", "--dev-scale=2" }, &options, &error)
        || !options.enabled || options.width != 1920 || options.height != 1080 || options.scale != 2 || options.scaleFactor != 2.0) return 2;
    if (parse({ "game", "--dev-tools", "--dev-screen=bad", "--dev-scale=2" }, &options, &error) || error == nullptr) return 3;
    if (parse({ "game", "--dev-screen=1920x1080", "--dev-scale=2" }, &options, &error)) return 4;
    if (parse({ "game", "--dev-tools", "--dev-screen=1920x1080" }, &options, &error)) return 5;
    if (parse({ "game", "--dev-tools", "--dev-scale=2" }, &options, &error)) return 6;
    if (parse({ "game", "--dev-tools", "--dev-screen", "--dev-scale=2" }, &options, &error)) return 7;
    if (parse({ "game", "--dev-tools", "--dev-screen=1920x1080", "--dev-scale" }, &options, &error)) return 8;
    if (parse({ "game", "--dev-tools", "--dev-screen=639x480", "--dev-scale=1" }, &options, &error)) return 9;
    if (parse({ "game", "--dev-tools", "--dev-screen=1920x1080", "--dev-scale=4" }, &options, &error)) return 10;
    if (parse({ "game", "--dev-tools", "--dev-screen=1920x1080junk", "--dev-scale=2" }, &options, &error)) return 11;
    if (parse({ "game", "--dev-tools", "--dev-screen=1920x1080", "--dev-scale=2x" }, &options, &error)) return 12;
    if (parse({ "game", "--dev-tools", "--dev-screen=1920x1080", "--dev-scale=2", "--dev-scale=1" }, &options, &error)) return 13;
    if (parse({ "game", "--dev-tools", "--dev-screen=1920x1080", "--dev-screen=1280x720", "--dev-scale=2" }, &options, &error)) return 14;
    if (!parse({ "game", "--dev-tools", "--dev-screen=2560x1440", "--dev-scale-factor=1.4" }, &options, &error)
        || !options.enabled || options.scaleFactor != 1.4 || options.scale != 1) return 15;
    if (parse({ "game", "--dev-tools", "--dev-screen=1920x1080", "--dev-scale-factor=3.0" }, &options, &error)) return 16;
    if (parse({ "game", "--dev-tools", "--dev-screen=2560x1440", "--dev-scale-factor=3.1" }, &options, &error)) return 17;
    return 0;
}
