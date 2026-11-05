// Windows launcher for Fallout 2: Reimagined (task 091).
//
// One small console exe a player runs against a clean Fallout 2 (Steam, GOG
// or a plain copy).  First run: find the game, install our build from the
// rolling `latest` GitHub release and make the 3D scene data with the packed
// exporter.  Later runs: pull a newer build when the release moved, then
// start the game.  The player's fallout2.cfg, saves and the original
// fallout2.exe are never touched.  No network: the installed game starts as
// it is.
//
// Only Windows system libraries are used (winhttp, bcrypt, advapi32, shell).

#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <shobjidl.h>

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr const char* kZipAsset = "fallout2-ce-3d-win64.zip";
constexpr const char* kDefaultUpdateBase =
    "https://github.com/Eugenius-S/f2-reimagined/releases/download/latest";
constexpr const char* kDefaultReleaseInfoUrl =
    "https://api.github.com/repos/Eugenius-S/f2-reimagined/releases/tags/latest";
constexpr const char* kRpuLatestApiUrl =
    "https://api.github.com/repos/BGforgeNet/Fallout2_Restoration_Project/releases/latest";
constexpr const char* kGameExe = "fallout2-ce.exe";
constexpr const char* kCfg = "fallout2.cfg";
constexpr const char* kVersionTxt = "version.txt";
constexpr const char* kExporterExe = "scene-exporter.exe";
constexpr const char* kDefaultMaps = "klamath";

struct Options {
    std::string gameRoot;
    std::string updateBase = kDefaultUpdateBase;
    std::string releaseInfoUrl = kDefaultReleaseInfoUrl;
    std::string maps = kDefaultMaps;
    bool noStart = false;
    bool forceExport = false;
    bool checkRelease = false;
    bool rollback = false;
    bool installRussian = false;
    std::vector<std::string> gameArgs;
};

void log(const char* text)
{
    std::printf("%s\n", text);
    std::fflush(stdout);
}

void logf(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    log(buffer);
}

[[noreturn]] void die(const std::string& message)
{
    logf("ERROR: %s", message.c_str());
    std::exit(1);
}

std::string trim(const std::string& text)
{
    size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return std::string();
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string wideToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return std::string();
    std::string out(static_cast<size_t>(size), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, out.data(), size, nullptr, nullptr) == 0) return std::string();
    out.resize(static_cast<size_t>(size - 1));
    return out;
}

std::wstring utf8ToWide(const std::string& text)
{
    if (text.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 1) return std::wstring();
    std::wstring out(static_cast<size_t>(size), L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), size) == 0) return std::wstring();
    out.resize(static_cast<size_t>(size - 1));
    return out;
}

// --- hashing -------------------------------------------------------------

std::string hashToHex(const unsigned char digest[32])
{
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (int i = 0; i < 32; i++) {
        out.push_back(digits[digest[i] >> 4]);
        out.push_back(digits[digest[i] & 0xF]);
    }
    return out;
}

bool sha256Data(const unsigned char* data, size_t size, unsigned char digest[32])
{
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) == 0;
    if (ok) ok = BCryptHashData(hash, const_cast<PUCHAR>(data), static_cast<ULONG>(size), 0) == 0;
    if (ok) ok = BCryptFinishHash(hash, digest, 32, 0) == 0;
    if (hash != nullptr) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return ok;
}

bool sha256File(const fs::path& path, std::string* hex)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::vector<unsigned char> contents((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    unsigned char digest[32];
    if (!sha256Data(contents.data(), contents.size(), digest)) return false;
    *hex = hashToHex(digest);
    return true;
}

// One hash over every file under the directory (paths and contents); saves
// the player has are caught whatever their names.
bool sha256Tree(const fs::path& root, std::string* hex)
{
    std::vector<fs::path> files;
    std::error_code error;
    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, error), end;
         !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error)) files.push_back(it->path());
    }
    std::sort(files.begin(), files.end());
    unsigned char digest[32];
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) == 0;
    auto feed = [&hash, &ok](const void* data, size_t size) {
        if (ok) ok = BCryptHashData(hash, const_cast<PUCHAR>(static_cast<const unsigned char*>(data)),
                        static_cast<ULONG>(size), 0) == 0;
    };
    char none = 0;
    if (files.empty()) feed(&none, 1);
    for (const fs::path& file : files) {
        std::string relative = wideToUtf8(fs::relative(file, root).generic_wstring());
        feed(relative.data(), relative.size());
        feed(&none, 1);
        std::ifstream in(file, std::ios::binary);
        if (!in) {
            ok = false;
            break;
        }
        char block[65536];
        while (in.read(block, sizeof(block)) || in.gcount() > 0) {
            feed(block, static_cast<size_t>(in.gcount()));
            if (!in) break;
        }
        feed(&none, 1);
    }
    if (ok) ok = BCryptFinishHash(hash, digest, 32, 0) == 0;
    if (hash != nullptr) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!ok) return false;
    *hex = hashToHex(digest);
    return true;
}

// --- HTTP ----------------------------------------------------------------

struct Url {
    std::wstring scheme;
    std::wstring host;
    int port = 443;
    std::wstring path;
    bool https = true;
};

bool crackUrlWide(const std::wstring& wide, Url* url)
{
    URL_COMPONENTS parts = {};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = (DWORD)-1;
    parts.dwHostNameLength = (DWORD)-1;
    parts.dwUrlPathLength = (DWORD)-1;
    parts.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(wide.c_str(), 0, 0, &parts)) return false;
    url->scheme.assign(parts.lpszScheme, parts.dwSchemeLength);
    url->host.assign(parts.lpszHostName, parts.dwHostNameLength);
    url->port = parts.nPort;
    url->path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.lpszExtraInfo != nullptr && parts.dwExtraInfoLength > 0) {
        url->path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    }
    url->https = url->scheme == L"https";
    return true;
}

// A loopback release server must not travel through the system proxy.
DWORD proxyFor(const Url& url)
{
    if (url.host == L"127.0.0.1" || url.host == L"localhost" || url.host == L"::1") {
        return WINHTTP_ACCESS_TYPE_NO_PROXY;
    }
    return WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
}

// GET one url; the failure reason lands in errorText when set.
bool httpGet(const std::string& urlString, std::string* body, const char** errorText)
{
    Url url;
    if (!crackUrlWide(utf8ToWide(urlString), &url)) {
        *errorText = "cannot parse url";
        return false;
    }
    *errorText = nullptr;
    HINTERNET session = WinHttpOpen(L"fallout2-ce-3d-launcher", proxyFor(url),
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) {
        *errorText = "WinHttpOpen failed";
        return false;
    }
    WinHttpSetTimeouts(session, 10000, 15000, 30000, 300000);
    HINTERNET connect = WinHttpConnect(session, url.host.c_str(), url.port, 0);
    if (connect == nullptr) {
        *errorText = "cannot connect";
        WinHttpCloseHandle(session);
        return false;
    }
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", url.path.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, url.https ? WINHTTP_FLAG_SECURE : 0);
    bool ok = request != nullptr;
    if (!ok) {
        *errorText = "cannot open request";
    } else {
        ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (!ok) *errorText = "send failed";
    }
    if (ok) {
        ok = WinHttpReceiveResponse(request, nullptr);
        if (!ok) *errorText = "receive failed";
    }
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (ok) {
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
        ok = status == 200;
        if (!ok) *errorText = "http status is not 200";
    }
    if (ok) {
        DWORD available = 0;
        while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
            std::vector<char> chunk(available);
            DWORD read = 0;
            if (!WinHttpReadData(request, chunk.data(), available, &read) || read == 0) break;
            body->append(chunk.data(), read);
        }
        ok = !body->empty();
        if (!ok) *errorText = "empty response";
    }
    if (request != nullptr) WinHttpCloseHandle(request);
    if (connect != nullptr) WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return ok;
}

bool httpDownload(const std::string& urlString, const fs::path& destination, const char** errorText)
{
    Url url;
    if (!crackUrlWide(utf8ToWide(urlString), &url)) {
        *errorText = "cannot parse url";
        return false;
    }
    *errorText = nullptr;
    HINTERNET session = WinHttpOpen(L"fallout2-ce-3d-launcher", proxyFor(url),
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) {
        *errorText = "WinHttpOpen failed";
        return false;
    }
    WinHttpSetTimeouts(session, 10000, 15000, 30000, 300000);
    HINTERNET connect = WinHttpConnect(session, url.host.c_str(), url.port, 0);
    HINTERNET request = nullptr;
    bool ok = connect != nullptr;
    if (ok) {
        request = WinHttpOpenRequest(connect, L"GET", url.path.c_str(), nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, url.https ? WINHTTP_FLAG_SECURE : 0);
        ok = request != nullptr;
        if (!ok) *errorText = "cannot open request";
    }
    if (ok) {
        ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (!ok) *errorText = "send failed";
    }
    if (ok) {
        ok = WinHttpReceiveResponse(request, nullptr);
        if (!ok) *errorText = "receive failed";
    }
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (ok) {
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
        ok = status == 200;
        if (!ok) *errorText = "http status is not 200";
    }
    std::ofstream out;
    if (ok) {
        out.open(destination, std::ios::binary | std::ios::trunc);
        ok = static_cast<bool>(out);
        if (!ok) *errorText = "cannot open the destination file";
    }
    if (ok) {
        DWORD available = 0;
        size_t total = 0;
        while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
            std::vector<char> chunk(available);
            DWORD read = 0;
            if (!WinHttpReadData(request, chunk.data(), available, &read) || read == 0) break;
            out.write(chunk.data(), read);
            total += read;
            if (total / (1024 * 1024) != (total - read) / (1024 * 1024)) {
                logf("  ... %zu MB", total / (1024 * 1024));
            }
        }
        out.close();
        ok = total > 0;
        if (!ok) *errorText = "empty download";
        else logf("  downloaded %zu bytes", total);
    }
    if (request != nullptr) WinHttpCloseHandle(request);
    if (connect != nullptr) WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return ok;
}

// --- finding the game ----------------------------------------------------

bool looksLikeFallout2(const fs::path& root)
{
    std::error_code error;
    return fs::exists(root / "master.dat", error) && fs::exists(root / "critter.dat", error);
}

std::string readRegistryString(HKEY hive, const char* key, const char* value)
{
    char buffer[MAX_PATH * 2] = {};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    LSTATUS status = RegGetValueA(hive, key, value, RRF_RT_REG_SZ, &type, buffer, &size);
    if (status != ERROR_SUCCESS) return std::string();
    return trim(std::string(buffer));
}

// A Steam library folder list lives in steamapps/libraryfolders.vdf; the
// entries this launcher needs are plain "path" lines.
std::vector<std::string> steamLibraries(const std::string& steamRoot)
{
    std::vector<std::string> out;
    std::ifstream file(fs::path(steamRoot) / "steamapps" / "libraryfolders.vdf");
    if (!file) return out;
    std::string line;
    while (std::getline(file, line)) {
        size_t key = line.find("\"path\"");
        if (key == std::string::npos) continue;
        size_t open = line.find('"', key + 6);
        if (open == std::string::npos) continue;
        size_t close = line.find('"', open + 1);
        if (close == std::string::npos) continue;
        std::string path = line.substr(open + 1, close - open - 1);
        size_t pos = 0;
        while ((pos = path.find("\\\\", pos)) != std::string::npos) path.replace(pos, 2, "\\");
        if (!path.empty()) out.push_back(path);
    }
    return out;
}

std::string pickFolder()
{
    std::string chosen;
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return chosen;
    IFileDialog* dialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))
        && dialog != nullptr) {
        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        dialog->SetTitle(L"Pick your Fallout 2 folder (the one with master.dat)");
        IShellItem* result = nullptr;
        if (SUCCEEDED(dialog->Show(nullptr)) && SUCCEEDED(dialog->GetResult(&result)) && result != nullptr) {
            PWSTR path = nullptr;
            if (SUCCEEDED(result->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path != nullptr) {
                chosen = wideToUtf8(path);
                CoTaskMemFree(path);
            }
            result->Release();
        }
        dialog->Release();
    }
    CoUninitialize();
    return chosen;
}

// Returns an empty string when the player cancelled.
std::string findGameRoot(bool allowPicker)
{
    std::vector<std::string> candidates;

    // GOG: the installer writes the game folder here.
    std::string gog = readRegistryString(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\WOW6432Node\\GOG.com\\Games\\1207658924", "path");
    if (!gog.empty()) {
        logf("registry: GOG Fallout 2 at %s", gog.c_str());
        candidates.push_back(gog);
    }

    // Steam: the primary library plus every folder of libraryfolders.vdf.
    std::string steam = readRegistryString(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath");
    if (!steam.empty()) {
        std::vector<std::string> libraries = steamLibraries(steam);
        libraries.push_back(steam);
        for (const std::string& library : libraries) {
            std::string candidate = (fs::path(library) / "steamapps" / "common" / "Fallout 2").string();
            candidates.push_back(candidate);
        }
    }

    for (const std::string& candidate : candidates) {
        if (looksLikeFallout2(candidate)) {
            logf("game found: %s", candidate.c_str());
            return candidate;
        }
    }

    if (!allowPicker) return std::string();
    log("the game was not found automatically; pick the Fallout 2 folder in the dialog");
    for (;;) {
        std::string picked = pickFolder();
        if (picked.empty()) return std::string();
        if (looksLikeFallout2(picked)) {
            logf("game found: %s", picked.c_str());
            return picked;
        }
        log("master.dat and critter.dat are not in that folder; pick the Fallout 2 folder again");
    }
}

// --- install and update --------------------------------------------------

// Starts the process and waits for it; false only when it never started.
// The exit code lands in exitCode: the game may leave by any door, the
// launcher still reports its own state afterwards.
bool runTool(const std::wstring& command, const std::wstring& workingDirectory, DWORD* exitCode = nullptr)
{
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    std::wstring mutableCommand = command;
    BOOL created = CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, 0, nullptr,
        workingDirectory.c_str(), &startup, &process);
    if (!created) return false;
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess);
    if (exitCode != nullptr) *exitCode = code;
    return true;
}

bool unpackZip(const fs::path& zip, const fs::path& stage)
{
    char systemDirectory[MAX_PATH] = {};
    GetSystemDirectoryA(systemDirectory, MAX_PATH);
    fs::path tar = fs::path(systemDirectory) / "tar.exe";
    if (fs::exists(tar)) {
        std::wstring command = L"\"" + tar.wstring() + L"\" -xf \"" + zip.wstring() + L"\" -C \""
            + stage.wstring() + L"\"";
        if (runTool(command, stage.wstring())) return true;
        log("tar.exe failed; falling back to Expand-Archive");
    }
    std::wstring command = L"powershell -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive "
        L"-LiteralPath '" + zip.wstring() + L"' -DestinationPath '" + stage.wstring() + L"' -Force\"";
    return runTool(command, stage.wstring());
}

bool copyIfPresent(const fs::path& stage, const char* name, const fs::path& destination, bool overwrite,
    const char* note)
{
    fs::path source = stage / name;
    if (!fs::exists(source)) {
        logf("  %s: not in the package, skipped", name);
        return false;
    }
    std::error_code error;
    fs::copy_file(source, destination / name,
        overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none, error);
    if (error) {
        logf("  %s: copy failed (%s)", name, error.message().c_str());
        return false;
    }
    logf("  %s: %s", name, note);
    return true;
}

const std::vector<std::string>& buildFiles()
{
    static const std::vector<std::string> names = {kGameExe, "ce.dat", kExporterExe, kVersionTxt};
    return names;
}

bool backupInstalledBuild(const fs::path& root, const std::string& version)
{
    std::string backupVersion = version.empty() ? "manual" : version;
    for (char& ch : backupVersion) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' || ch == '-' || ch == '_')) ch = '_';
    }
    if (backupVersion.size() > 100) backupVersion.resize(100);
    fs::path backup = root / "f2reimagined-backups" / backupVersion;
    std::error_code error;
    fs::create_directories(backup, error);
    if (error) {
        logf("cannot create build backup: %s", error.message().c_str());
        return false;
    }
    bool savedAny = false;
    for (const std::string& name : buildFiles()) {
        fs::path source = root / name;
        if (!fs::is_regular_file(source, error)) continue;
        fs::copy_file(source, backup / name, fs::copy_options::overwrite_existing, error);
        if (error) {
            logf("cannot back up %s: %s", name.c_str(), error.message().c_str());
            return false;
        }
        savedAny = true;
    }
    if (!savedAny) {
        log("no installed build files were available to back up");
        return false;
    }
    if (!fs::exists(backup / kVersionTxt)) {
        std::ofstream versionFile(backup / kVersionTxt, std::ios::binary | std::ios::trunc);
        versionFile << backupVersion;
        if (!versionFile) return false;
    }
    logf("backed up build %s to f2reimagined-backups", version.c_str());
    return true;
}

bool rollbackInstalledBuild(const fs::path& root)
{
    std::string currentVersion;
    {
        std::ifstream file(root / kVersionTxt);
        if (file) currentVersion = trim(std::string((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()));
    }
    fs::path backupRoot = root / "f2reimagined-backups";
    fs::path selected;
    std::string selectedVersion;
    fs::file_time_type selectedTime = fs::file_time_type::min();
    std::error_code error;
    if (!fs::is_directory(backupRoot, error)) {
        log("no previous build backup is available");
        return false;
    }
    for (fs::directory_iterator it(backupRoot, error), end; !error && it != end; it.increment(error)) {
        if (!it->is_directory(error)) continue;
        std::ifstream versionFile(it->path() / kVersionTxt);
        if (!versionFile) continue;
        std::string version = trim(std::string((std::istreambuf_iterator<char>(versionFile)),
            std::istreambuf_iterator<char>()));
        if (version.empty() || version == currentVersion) continue;
        fs::file_time_type time = fs::last_write_time(it->path(), error);
        if (error) { error.clear(); continue; }
        if (selected.empty() || time > selectedTime) {
            selected = it->path();
            selectedVersion = version;
            selectedTime = time;
        }
    }
    if (selected.empty()) {
        log("no build different from the installed version is available for rollback");
        return false;
    }
    for (const std::string& name : buildFiles()) {
        fs::path source = selected / name;
        if (!fs::is_regular_file(source, error)) continue;
        fs::copy_file(source, root / name, fs::copy_options::overwrite_existing, error);
        if (error) {
            logf("rollback failed for %s: %s", name.c_str(), error.message().c_str());
            return false;
        }
    }
    logf("restored previous build %s", selectedVersion.c_str());
    return true;
}

bool setLanguage(const fs::path& root, const char* language)
{
    fs::path path = root / kCfg;
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        log("cannot set the game language: fallout2.cfg is missing");
        return false;
    }
    std::string input((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::string output;
    bool inSystem = false;
    bool found = false;
    size_t begin = 0;
    while (begin < input.size()) {
        size_t end = input.find('\n', begin);
        if (end == std::string::npos) end = input.size(); else end++;
        std::string line = input.substr(begin, end - begin);
        std::string content = line;
        while (!content.empty() && (content.back() == '\r' || content.back() == '\n')) content.pop_back();
        if (!content.empty() && content[0] == '[') inSystem = content == "[system]";
        if (inSystem && content.rfind("language=", 0) == 0) {
            std::string ending = line.substr(content.size());
            line = std::string("language=") + language + ending;
            found = true;
        }
        output += line;
        begin = end;
    }
    if (!found) {
        log("cannot set the game language: [system].language was not found");
        return false;
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(output.data(), static_cast<std::streamsize>(output.size()));
    if (!out) {
        log("cannot write fallout2.cfg");
        return false;
    }
    return true;
}

std::string jsonStringValue(const std::string& json, const char* key);

bool enableRussianMod(const fs::path& root)
{
    fs::path mods = root / "mods";
    fs::create_directories(mods);
    fs::path orderPath = mods / "mods_order.txt";
    std::string input;
    {
        std::ifstream in(orderPath, std::ios::binary);
        if (in) input.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (lower.find("rpu_russian.dat") == std::string::npos) {
        std::string output;
        std::stringstream lines(input);
        std::string line;
        bool inserted = false;
        while (std::getline(lines, line)) {
            output += line + "\r\n";
            std::string normalized = trim(line);
            std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (!inserted && normalized == "rpu.dat") {
                output += "rpu_russian.dat\r\n";
                inserted = true;
            }
        }
        if (!inserted) output = "rpu_russian.dat\r\n" + output;
        std::ofstream out(orderPath, std::ios::binary | std::ios::trunc);
        out.write(output.data(), static_cast<std::streamsize>(output.size()));
        if (!out) {
            log("cannot update mods/mods_order.txt");
            return false;
        }
    }
    return setLanguage(root, "russian");
}

bool installRussianTranslation(const fs::path& root)
{
    fs::path translation = root / "mods" / "rpu_russian.dat";
    if (!fs::exists(translation)) {
        const char* networkError = nullptr;
        std::string release;
        if (!httpGet(kRpuLatestApiUrl, &release, &networkError)) {
            logf("cannot reach the official RPU release (%s)",
                networkError != nullptr ? networkError : "network error");
            return false;
        }
        std::string tag = jsonStringValue(release, "tag_name");
        if (tag.empty()) {
            log("the official RPU release did not provide a version tag");
            return false;
        }
        std::string filename = "rpu_" + tag + "_translations.zip";
        std::string url = "https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/download/"
            + tag + "/" + filename;
        char tempPath[MAX_PATH] = {};
        GetTempPathA(MAX_PATH, tempPath);
        fs::path work = fs::path(tempPath) / ("f2ce3d-russian-" + std::to_string(GetCurrentProcessId()));
        std::error_code error;
        fs::remove_all(work, error);
        fs::create_directories(work / "stage", error);
        if (error) {
            logf("cannot create translation staging folder: %s", error.message().c_str());
            return false;
        }
        fs::path zip = work / filename;
        logf("downloading the official Russian translation from RPU %s", tag.c_str());
        if (!httpDownload(url, zip, &networkError) || !unpackZip(zip, work / "stage")) {
            logf("the Russian translation download failed (%s)",
                networkError != nullptr ? networkError : "unpack failed");
            fs::remove_all(work, error);
            return false;
        }
        fs::path source;
        for (fs::recursive_directory_iterator it(work / "stage", error), end; !error && it != end; it.increment(error)) {
            if (it->is_regular_file() && it->path().filename() == "rpu_russian.dat") {
                source = it->path();
                break;
            }
        }
        if (source.empty()) {
            log("the official translation archive does not contain rpu_russian.dat");
            fs::remove_all(work, error);
            return false;
        }
        fs::create_directories(translation.parent_path(), error);
        fs::copy_file(source, translation, fs::copy_options::overwrite_existing, error);
        bool copySucceeded = !error;
        fs::remove_all(work, error);
        if (!copySucceeded) {
            log("failed to install the Russian translation file");
            return false;
        }
        log("  installed mods/rpu_russian.dat from the official RPU release");
    } else {
        log("  kept the existing mods/rpu_russian.dat");
    }
    if (!enableRussianMod(root)) return false;
    log("  game language set to Russian; mods/mods_order.txt updated");
    return true;
}

// --- command line --------------------------------------------------------

// The release base may come with a trailing slash; the asset names do not.
std::string joinUrl(const std::string& base, const char* name)
{
    std::string out = base;
    while (!out.empty() && out.back() == '/') out.pop_back();
    out += "/";
    out += name;
    return out;
}

std::string jsonStringValue(const std::string& json, const char* key)
{
    std::string needle = std::string("\"") + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return std::string();
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return std::string();
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return std::string();
    std::string value;
    for (size_t i = pos + 1; i < json.size(); i++) {
        char ch = json[i];
        if (ch == '"') return value;
        if (ch == '\\' && i + 1 < json.size()) {
            char escaped = json[++i];
            switch (escaped) {
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            case '\\': value.push_back('\\'); break;
            case '"': value.push_back('"'); break;
            default: value.push_back(escaped); break;
            }
        } else {
            value.push_back(ch);
        }
    }
    return std::string();
}

std::string shortNotes(const std::string& body)
{
    std::stringstream lines(body);
    std::string line;
    std::string result;
    int kept = 0;
    while (std::getline(lines, line) && kept < 2) {
        line = trim(line);
        while (!line.empty() && (line[0] == '#' || line[0] == '*' || line[0] == '-' || line[0] == ' ')) {
            line.erase(line.begin());
        }
        line = trim(line);
        if (line.empty()) continue;
        if (!result.empty()) result += " | ";
        result += line;
        kept++;
    }
    return result;
}

bool fetchReleaseInfo(const std::string& updateBase, const std::string& releaseInfoUrl,
    std::string* version, std::string* notes)
{
    const char* error = nullptr;
    std::string versionBody;
    if (!httpGet(joinUrl(updateBase, kVersionTxt), &versionBody, &error)) return false;
    *version = trim(versionBody);
    notes->clear();
    if (!releaseInfoUrl.empty()) {
        std::string response;
        if (httpGet(releaseInfoUrl, &response, &error)) {
            *notes = shortNotes(jsonStringValue(response, "body"));
        }
    }
    return !version->empty();
}

Options parseOptions(int argc, char** argv)
{
    Options options;
    bool passthrough = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (passthrough) {
            options.gameArgs.push_back(arg);
        } else if (arg == "--") {
            passthrough = true;
        } else if (arg == "--help" || arg == "-h") {
            log("fallout2-ce-3d-launcher: installs Fallout 2: Reimagined onto a clean Fallout 2 and keeps");
            log("it updated from the rolling `latest` GitHub release, then starts the game.");
            log("");
            log("  --game-root <dir>   the Fallout 2 folder (default: registry lookup, then a picker)");
            log("  --update-url <url>  where version.txt and the zip live (default: the latest release)");
            log("  --release-info-url <url>  release API for the version notes");
            log("  --maps <list>       maps the exporter covers, comma separated (default: klamath)");
            log("  --force-export      rebuild the 3D scene data even if it looks current");
            log("  --no-start          do everything except starting the game");
            log("  --rollback          restore the most recent backed-up game build");
            log("  --russian           install the official RPU Russian text translation");
            log("  --check-release     print the latest version and release notes");
            log("  -- <args>           arguments handed over to the game");
            std::exit(0);
        } else if (arg == "--game-root" && i + 1 < argc) {
            options.gameRoot = argv[++i];
        } else if (arg == "--update-url" && i + 1 < argc) {
            options.updateBase = argv[++i];
        } else if (arg == "--release-info-url" && i + 1 < argc) {
            options.releaseInfoUrl = argv[++i];
        } else if (arg == "--maps" && i + 1 < argc) {
            options.maps = argv[++i];
        } else if (arg == "--no-start") {
            options.noStart = true;
        } else if (arg == "--force-export") {
            options.forceExport = true;
        } else if (arg == "--rollback") {
            options.rollback = true;
        } else if (arg == "--russian") {
            options.installRussian = true;
        } else if (arg == "--check-release") {
            options.checkRelease = true;
        } else {
            die("unknown option: " + arg + " (see --help)");
        }
    }
    return options;
}

std::vector<std::string> splitMaps(const std::string& maps)
{
    std::vector<std::string> out;
    std::stringstream stream(maps);
    std::string item;
    while (std::getline(stream, item, ',')) {
        std::string name = trim(item);
        if (!name.empty()) out.push_back(name);
    }
    return out;
}

} // namespace

std::string findGameRootForUi()
{
    return findGameRoot(false);
}

int runLauncherUi(int argc, char** argv);

int runConsole(Options options)
{
    log("fallout2-ce-3d-launcher");
    log("");

    if (options.checkRelease) {
        std::string version;
        std::string notes;
        if (!fetchReleaseInfo(options.updateBase, options.releaseInfoUrl, &version, &notes)) {
            log("release check failed");
            return 1;
        }
        logf("RELEASE_VERSION:%s", version.c_str());
        logf("RELEASE_NOTES:%s", notes.c_str());
        return 0;
    }

    // --- the game folder ---
    fs::path root;
    if (!options.gameRoot.empty()) {
        root = fs::path(utf8ToWide(options.gameRoot));
        logf("game root (from --game-root): %s", options.gameRoot.c_str());
        if (!looksLikeFallout2(root)) die("master.dat and critter.dat are not in --game-root");
    } else {
        std::string found = findGameRoot(true);
        if (found.empty()) die("no Fallout 2 folder given");
        root = found;
    }

    if (options.rollback) return rollbackInstalledBuild(root) ? 0 : 1;

    bool installedGamePresent = fs::exists(root / kGameExe);
    std::string installedVersion;
    {
        std::ifstream file(root / kVersionTxt);
        if (file) installedVersion = trim(std::string((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()));
    }
    if (!installedVersion.empty()) {
        logf("installed build: %s", installedVersion.c_str());
    } else if (installedGamePresent) {
        log("installed build: an older manual install (no version.txt)");
    } else {
        log("installed build: none, fresh install");
    }

    // --- what does the release hold? ---
    std::string remoteVersion;
    const char* networkError = nullptr;
    std::string versionUrl = joinUrl(options.updateBase, kVersionTxt);
    if (!httpGet(versionUrl, &remoteVersion, &networkError)) {
        remoteVersion.clear();
        logf("cannot reach the release (%s: %s)", versionUrl.c_str(),
            networkError != nullptr ? networkError : "network error");
    } else {
        remoteVersion = trim(remoteVersion);
        logf("release build: %s", remoteVersion.c_str());
    }

    if (remoteVersion.empty()) {
        if (installedVersion.empty() && !installedGamePresent) {
            die("first install needs the network: the release is unreachable");
        }
        log("offline: starting the installed game as it is");
    }

    bool freshInstall = !installedGamePresent;
    bool updated = false;
    if (!remoteVersion.empty() && remoteVersion != installedVersion) {
        // --- download and unpack ---
        logf("%s: %s -> %s", installedVersion.empty() ? "installing" : "updating",
            installedVersion.empty() ? "none" : installedVersion.c_str(), remoteVersion.c_str());
        char tempPath[MAX_PATH] = {};
        GetTempPathA(MAX_PATH, tempPath);
        fs::path work = fs::path(tempPath) / ("f2ce3d-" + std::to_string(GetCurrentProcessId()));
        std::error_code error;
        fs::remove_all(work, error);
        fs::create_directories(work, error);
        fs::path zip = work / kZipAsset;
        fs::path stage = work / "stage";
        fs::create_directories(stage, error);

        const char* downloadError = nullptr;
        std::string zipUrl = joinUrl(options.updateBase, kZipAsset);
        logf("downloading %s", zipUrl.c_str());
        if (!httpDownload(zipUrl, zip, &downloadError)) {
            fs::remove_all(work, error);
            die(std::string("download failed: ") + (downloadError != nullptr ? downloadError : "network error"));
        }
        log("unpacking");
        if (!unpackZip(zip, stage)) {
            fs::remove_all(work, error);
            die("unpack failed");
        }
        if (!fs::exists(stage / kGameExe)) {
            fs::remove_all(work, error);
            die("the package does not hold the game exe");
        }

        if (installedGamePresent && !backupInstalledBuild(root,
            installedVersion.empty() ? std::string("manual") : installedVersion)) {
            fs::remove_all(work, error);
            die("cannot back up the installed build before updating");
        }

        // --- into the game folder; the player's things stay theirs ---
        log("installing into the game folder:");
        copyIfPresent(stage, kGameExe, root, true, "installed");
        copyIfPresent(stage, "ce.dat", root, true, "installed");
        copyIfPresent(stage, kExporterExe, root, true, "installed");
        if (fs::exists(root / kCfg)) {
            log("  fallout2.cfg: kept (the player's file is never overwritten)");
        } else {
            copyIfPresent(stage, kCfg, root, false, "installed (default)");
        }
        {
            std::ofstream file(root / kVersionTxt, std::ios::binary | std::ios::trunc);
            file << remoteVersion;
        }
        logf("  version.txt: %s", remoteVersion.c_str());
        fs::remove_all(work, error);
        updated = true;
        installedVersion = remoteVersion;
    } else if (!remoteVersion.empty()) {
        logf("the installed build matches the release: nothing to pull");
    }

    // --- 3D scene data, always made on this machine from this game ---
    std::vector<std::string> maps = splitMaps(options.maps);
    std::string mapList;
    for (size_t i = 0; i < maps.size(); i++) {
        if (i != 0) mapList += ",";
        mapList += maps[i];
    }
    fs::path exporter = root / kExporterExe;
    if (!fs::exists(exporter)) {
        log("scene-exporter.exe is missing; the game will start in 2D");
    } else {
        bool needExport = options.forceExport || freshInstall || updated;
        if (!needExport) {
            // The exporter owns the map groups: "klamath" is its name, only
            // it knows which scene directories that covers.  Exit 0 = every
            // base scene exists, anything else = make them.
            std::string command = "\"" + exporter.string() + "\" --game-root \"" + root.string()
                + "\" --maps " + mapList + " --check";
            DWORD checkCode = 1;
            runTool(utf8ToWide(command), root.wstring(), &checkCode);
            needExport = checkCode != 0;
        }
        if (needExport) {
            std::string command = "\"" + exporter.string() + "\" --game-root \"" + root.string()
                + "\" --maps " + mapList;
            logf("making the 3D scene data for: %s", mapList.c_str());
            if (!runTool(utf8ToWide(command), root.wstring())) {
                log("the exporter failed; the game will start in 2D");
            }
        } else {
            log("the 3D scene data is current");
        }
    }

    if (options.installRussian && !installRussianTranslation(root)) {
        die("the Russian translation could not be installed");
    }

    if (options.noStart) {
        log("done (--no-start)");
        return 0;
    }

    // --- hashes of the player's things, before the run ---
    std::string cfgBefore = "<missing>";
    std::string savesBefore;
    if (fs::exists(root / kCfg)) sha256File(root / kCfg, &cfgBefore);
    sha256Tree(root / "data" / "savegame", &savesBefore);
    logf("fallout2.cfg sha256 before: %s", cfgBefore.c_str());
    logf("data/savegame sha256 before: %s", savesBefore.c_str());

    // --- start the game ---
    fs::path game = root / kGameExe;
    if (!fs::exists(game)) die("the game exe is not installed");
    std::string command = "\"" + game.string() + "\"";
    for (const std::string& argument : options.gameArgs) {
        command += " ";
        if (argument.find(' ') != std::string::npos) command += "\"" + argument + "\"";
        else command += argument;
    }
    logf("starting %s", game.string().c_str());
    DWORD gameExitCode = 0;
    if (!runTool(utf8ToWide(command), root.wstring(), &gameExitCode)) {
        die("the game could not be started");
    }
    logf("the game exited with code %lu", static_cast<unsigned long>(gameExitCode));

    // --- and after: must be byte for byte the same ---
    std::string cfgAfter = "<missing>";
    std::string savesAfter;
    if (fs::exists(root / kCfg)) sha256File(root / kCfg, &cfgAfter);
    sha256Tree(root / "data" / "savegame", &savesAfter);
    logf("fallout2.cfg sha256 after:  %s (%s)", cfgAfter.c_str(),
        cfgAfter == cfgBefore ? "unchanged" : "CHANGED");
    logf("data/savegame sha256 after:  %s (%s)", savesAfter.c_str(),
        savesAfter == savesBefore ? "unchanged" : "CHANGED");
    log("bye");
    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 1 || (argc >= 2 && std::string(argv[1]) == "--ui")) {
        return runLauncherUi(argc, argv);
    }
    return runConsole(parseOptions(argc, argv));
}
