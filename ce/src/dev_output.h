// Paths for the files our build writes. The game folder root keeps only the
// game's own files: dev snapshots land in shots/dev/<date>/, the classic
// screenshot in shots/game/, our logs in logs/.
//
// F2CE_DEV_OUT=DIR (a test run's own folder): all of those, the stray dumps
// and the game's own state (saved maps, automap, world map, protos) go under
// DIR instead, so test games running side by side never meet. Unset, every
// path is the one the game folder always had.
#ifndef DEV_OUTPUT_H
#define DEV_OUTPUT_H

#include <filesystem>

namespace dev_output
{
// Returns the folder for the given kind and creates it on demand. kind is
// "logs" for our log files, "shots/dev" for dev snapshots (pass the capture
// date as "YYYY-MM-DD") and "shots/game" for the classic screenshot. An
// empty result means the folder could not be created; appending a file name
// to it still degrades to the bare game-folder path the callers wrote to
// before.
std::filesystem::path outputDirectory(const char* kind, const char* date = nullptr);

// A file that used to land in the game folder root: under F2CE_DEV_OUT when
// it is set, the bare name otherwise.
std::filesystem::path outputFile(const char* name);

// The folder that stands in for master_patches when F2CE_DEV_OUT is set
// (DIR/data, created on demand); empty when it is not.
std::filesystem::path gameStateDirectory();

} // namespace dev_output

#endif /* DEV_OUTPUT_H */
