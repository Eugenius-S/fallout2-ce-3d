#include "test_run_state.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

#include "../db.h"
#include "../dev_output.h"
#include "../platform_compat.h"
#include "../settings.h"

namespace fallout {
namespace devtools {

namespace {

// The configured patches folder, mounted below the run's own one.
std::string gSharedPatchesPath;
bool gSharedPatchesMounted = false;

} // namespace

void testRunStateRedirectPatches()
{
    const std::filesystem::path gameStatePath = dev_output::gameStateDirectory();
    if (gameStatePath.empty()) {
        return;
    }
    gSharedPatchesPath = settings.system.master_patches_path;
    settingsRedirectMasterPatches(gameStatePath.string());
}

void testRunStateMountSharedPatches()
{
    if (gSharedPatchesPath.empty()) {
        return;
    }
    dbOpen(gSharedPatchesPath.c_str());
    gSharedPatchesMounted = true;
}

bool testRunStateHidesFile(const char* xbasePath, const char* filePath)
{
    if (!gSharedPatchesMounted || compat_stricmp(xbasePath, gSharedPatchesPath.c_str()) != 0) {
        return false;
    }

    char path[COMPAT_MAX_PATH];
    snprintf(path, sizeof(path), "%s", filePath);
    for (char* ch = path; *ch != '\0'; ch++) {
        if (*ch == '/') *ch = '\\';
    }

    const char* extension = strrchr(path, '.');
    if (compat_strnicmp(path, "MAPS\\", 5) == 0 && strchr(path + 5, '\\') == nullptr) {
        return (extension != nullptr && compat_stricmp(extension, ".SAV") == 0)
            || compat_stricmp(path + 5, "AUTOMAP.DB") == 0
            || compat_stricmp(path + 5, "AUTOMAP.TMP") == 0;
    }
    if (compat_strnicmp(path, "proto\\critters\\", 15) == 0 || compat_strnicmp(path, "proto\\items\\", 12) == 0) {
        return extension != nullptr && compat_stricmp(extension, ".pro") == 0;
    }
    return compat_stricmp(path, "worldmap.dat") == 0;
}

} // namespace devtools
} // namespace fallout
