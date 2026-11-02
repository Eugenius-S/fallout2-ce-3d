#include "../../src/devtools/fixed_screen_options.h"
#include "../../src/scene/display_dimensions.h"

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
