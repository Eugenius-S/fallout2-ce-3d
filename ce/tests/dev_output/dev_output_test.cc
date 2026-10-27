#include "../../src/dev_output.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

static void setOutput(const char* value)
{
#ifdef _WIN32
    _putenv_s("F2CE_DEV_OUT", value != nullptr ? value : "");
#else
    if (value != nullptr) {
        setenv("F2CE_DEV_OUT", value, 1);
    } else {
        unsetenv("F2CE_DEV_OUT");
    }
#endif
}

int main()
{
    std::error_code error;
    const fs::path work = fs::temp_directory_path() / "f2ce-dev-output-test";
    fs::remove_all(work, error);
    fs::create_directories(work);
    fs::current_path(work);

    // Unset: the bare game-folder paths the callers always wrote to.
    setOutput(nullptr);
    if (dev_output::outputDirectory("logs") != fs::path("logs") || !fs::is_directory(work / "logs")) {
        std::fprintf(stderr, "logs moved without F2CE_DEV_OUT\n");
        return 1;
    }
    if (dev_output::outputDirectory("shots/dev", "2026-10-27") != fs::path("shots/dev") / "2026-10-27") {
        std::fprintf(stderr, "dated snapshot folder moved without F2CE_DEV_OUT\n");
        return 2;
    }
    if (dev_output::outputFile("shadowmap-dump.pgm") != fs::path("shadowmap-dump.pgm")
        || !dev_output::gameStateDirectory().empty()) {
        std::fprintf(stderr, "root files or game state moved without F2CE_DEV_OUT\n");
        return 3;
    }

    // Set: everything lands under the run's folder.
    const fs::path run = work / "run";
    setOutput(run.string().c_str());
    if (dev_output::outputDirectory("logs") != run / "logs" || !fs::is_directory(run / "logs")) {
        std::fprintf(stderr, "logs not under F2CE_DEV_OUT\n");
        return 4;
    }
    if (dev_output::outputDirectory("shots/game") != run / "shots/game") {
        std::fprintf(stderr, "classic shots not under F2CE_DEV_OUT\n");
        return 5;
    }
    if (dev_output::outputFile("debug.log") != run / "debug.log") {
        std::fprintf(stderr, "root file not under F2CE_DEV_OUT\n");
        return 6;
    }
    if (dev_output::gameStateDirectory() != run / "data" || !fs::is_directory(run / "data")) {
        std::fprintf(stderr, "game state not under F2CE_DEV_OUT\n");
        return 7;
    }

    // An empty value counts as unset.
    setOutput("");
    if (dev_output::outputDirectory("logs") != fs::path("logs") || !dev_output::gameStateDirectory().empty()) {
        std::fprintf(stderr, "empty F2CE_DEV_OUT moved the output\n");
        return 8;
    }

    fs::current_path(fs::temp_directory_path());
    fs::remove_all(work, error);
    return 0;
}
