#include "dev_output.h"

#include <system_error>

namespace dev_output
{

std::filesystem::path outputDirectory(const char* kind, const char* date)
{
    std::filesystem::path directory(kind);
    if (date != nullptr) {
        directory /= date;
    }
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        return {};
    }
    return directory;
}

} // namespace dev_output
