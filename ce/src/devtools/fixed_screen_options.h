#ifndef FALLOUT_DEVTOOLS_FIXED_SCREEN_OPTIONS_H
#define FALLOUT_DEVTOOLS_FIXED_SCREEN_OPTIONS_H

namespace fallout {
namespace devtools {

struct FixedScreenOptions {
    bool enabled = false;
    int width = 0;
    int height = 0;
    int scale = 0;
};

// Parses --dev-screen=<width>x<height> and --dev-scale=<1..4> from the full
// command line. Both flags require --dev-tools and must be supplied together.
bool parseFixedScreenOptions(int argc, char** argv, FixedScreenOptions* options, const char** error);

} // namespace devtools
} // namespace fallout

#endif
