#include "dev_output.h"

#include <cstdlib>
#include <system_error>

namespace dev_output
{

namespace
{

std::filesystem::path outputRoot()
{
    const char* root = std::getenv("F2CE_DEV_OUT");
    if (root == nullptr || root[0] == '\0') {
        return {};
    }
    return std::filesystem::absolute(std::filesystem::path(root));
}

} // namespace

std::filesystem::path outputDirectory(const char* kind, const char* date)
{
    const std::filesystem::path root = outputRoot();
    std::filesystem::path directory = root / kind;
    if (date != nullptr) {
        directory /= date;
    }
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        // A test run's files still stay in its own folder.
        return root;
    }
    return directory;
}

std::filesystem::path outputFile(const char* name)
{
    const std::filesystem::path root = outputRoot();
    if (root.empty()) {
        return name;
    }
    std::error_code error;
    std::filesystem::create_directories(root, error);
    return root / name;
}

std::filesystem::path gameStateDirectory()
{
    const std::filesystem::path root = outputRoot();
    if (root.empty()) {
        return {};
    }
    const std::filesystem::path directory = root / "data";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    return directory;
}

} // namespace dev_output
