// Paths for the files our build writes. The game folder root keeps only the
// game's own files: dev snapshots land in shots/dev/<date>/, the classic
// screenshot in shots/game/, our logs in logs/.
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

} // namespace dev_output

#endif /* DEV_OUTPUT_H */
