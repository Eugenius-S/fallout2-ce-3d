#include "fixed_screen_options.h"

#include <stdlib.h>
#include <string.h>

namespace fallout {
namespace devtools {

namespace {

bool parsePositiveInteger(const char* value, const char** end, int* result)
{
    if (value == nullptr || *value < '0' || *value > '9') return false;
    char* parsedEnd = nullptr;
    const long parsed = strtol(value, &parsedEnd, 10);
    if (parsedEnd == value || parsed < 1 || parsed > 100000) return false;
    *end = parsedEnd;
    *result = static_cast<int>(parsed);
    return true;
}

} // namespace

bool parseFixedScreenOptions(int argc, char** argv, FixedScreenOptions* options, const char** error)
{
    if (options == nullptr || error == nullptr) return false;
    *options = {};
    *error = nullptr;

    bool devTools = false;
    bool hasScreen = false;
    bool hasScale = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dev-tools") == 0) {
            devTools = true;
            continue;
        }
        if (strcmp(argv[i], "--dev-screen") == 0) {
            *error = "invalid --dev-screen (expected --dev-screen=WIDTHxHEIGHT)";
            return false;
        }
        if (strcmp(argv[i], "--dev-scale") == 0) {
            *error = "invalid --dev-scale (expected --dev-scale=1..4)";
            return false;
        }

        constexpr char screenPrefix[] = "--dev-screen=";
        if (strncmp(argv[i], screenPrefix, sizeof(screenPrefix) - 1) == 0) {
            if (hasScreen) {
                *error = "duplicate --dev-screen";
                return false;
            }
            hasScreen = true;
            const char* value = argv[i] + sizeof(screenPrefix) - 1;
            const char* end = nullptr;
            if (!parsePositiveInteger(value, &end, &options->width) || *end != 'x'
                || !parsePositiveInteger(end + 1, &end, &options->height) || *end != '\0') {
                *error = "invalid --dev-screen (expected WIDTHxHEIGHT)";
                return false;
            }
            continue;
        }

        constexpr char scalePrefix[] = "--dev-scale=";
        if (strncmp(argv[i], scalePrefix, sizeof(scalePrefix) - 1) == 0) {
            if (hasScale) {
                *error = "duplicate --dev-scale";
                return false;
            }
            hasScale = true;
            const char* value = argv[i] + sizeof(scalePrefix) - 1;
            const char* end = nullptr;
            if (!parsePositiveInteger(value, &end, &options->scale) || *end != '\0'
                || options->scale < 1 || options->scale > 4) {
                *error = "invalid --dev-scale (expected 1..4)";
                return false;
            }
        }
    }

    if (!hasScreen && !hasScale) return true;
    if (!devTools) {
        *error = "--dev-screen and --dev-scale require --dev-tools";
        return false;
    }
    if (!hasScreen || !hasScale) {
        *error = "--dev-screen and --dev-scale must be used together";
        return false;
    }
    if (options->width < 640 || options->width > 7680 || options->height < 480 || options->height > 4320) {
        *error = "--dev-screen is outside supported bounds (640x480..7680x4320)";
        return false;
    }
    if (options->width % options->scale != 0 || options->height % options->scale != 0
        || options->width / options->scale < 640 || options->height / options->scale < 480) {
        *error = "--dev-scale is incompatible with --dev-screen";
        return false;
    }

    options->enabled = true;
    return true;
}

} // namespace devtools
} // namespace fallout
