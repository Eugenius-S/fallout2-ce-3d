#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shobjidl.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

std::string findGameRootForUi();

namespace {

constexpr UINT WM_RELEASE_INFO = WM_APP + 1;
constexpr UINT WM_LOG_LINE = WM_APP + 2;
constexpr UINT WM_OPERATION_DONE = WM_APP + 3;
constexpr int ID_LANGUAGE = 1001;
constexpr int ID_GAME_ROOT = 1002;
constexpr int ID_BROWSE = 1003;
constexpr int ID_RUSSIAN = 1004;
constexpr int ID_RP = 1005;
constexpr int ID_AUTO_UPDATE = 1006;
constexpr int ID_PROGRESS = 1007;
constexpr int ID_STATUS = 1008;
constexpr int ID_RELEASE = 1009;
constexpr int ID_NOTES = 1010;
constexpr int ID_LOG = 1011;
constexpr int ID_INSTALL = 1012;
constexpr int ID_ROLLBACK = 1013;
constexpr int ID_PLAY = 1014;
constexpr int ID_LOG_SCROLL = 1015;
constexpr int ID_PATH_HINT = 1016;
constexpr WORD IDB_LAUNCHER_LOGO = 201;

struct UiArgs {
    std::wstring gameRoot;
    std::wstring updateUrl;
    std::wstring releaseInfoUrl;
    std::wstring settingsFile;
    bool noActivate = false;
};

struct ReleaseInfo {
    std::wstring version;
    std::wstring notes;
    bool online = false;
};

HWND g_window = nullptr;
HWND g_language = nullptr;
HWND g_root = nullptr;
HWND g_browse = nullptr;
HWND g_russian = nullptr;
HWND g_rp = nullptr;
HWND g_autoUpdate = nullptr;
HWND g_progress = nullptr;
HWND g_status = nullptr;
HWND g_release = nullptr;
HWND g_notes = nullptr;
HWND g_log = nullptr;
HWND g_install = nullptr;
HWND g_rollback = nullptr;
HWND g_play = nullptr;
HWND g_logScroll = nullptr;
HWND g_pathHint = nullptr;
WNDPROC g_logOriginalProc = nullptr;
HBITMAP g_logo = nullptr;
HFONT g_font = nullptr;
HFONT g_headingFont = nullptr;
HBRUSH g_background = nullptr;
HBRUSH g_editBackground = nullptr;
UiArgs g_args;
std::wstring g_settingsPath;
std::wstring g_uiLanguage;
std::wstring g_releaseVersion;
std::wstring g_releaseNotes;
bool g_autoUpdates = false;
bool g_russianInstall = false;
bool g_logDragging = false;
bool g_busy = false;
bool g_haveRelease = false;
bool g_installed = false;
bool g_operationCompleted = false;
bool g_lastRollback = false;
int g_progressValue = 0;

int logVisibleLines()
{
    RECT rect = {};
    SendMessageW(g_log, EM_GETRECT, 0, reinterpret_cast<LPARAM>(&rect));
    TEXTMETRICW metrics = {};
    HDC dc = GetDC(g_log);
    HGDIOBJ oldFont = SelectObject(dc, g_font);
    GetTextMetricsW(dc, &metrics);
    SelectObject(dc, oldFont);
    ReleaseDC(g_log, dc);
    return metrics.tmHeight > 0 ? std::max(1, static_cast<int>((rect.bottom - rect.top) / metrics.tmHeight)) : 1;
}

void updateLogScroll()
{
    if (g_log == nullptr || g_logScroll == nullptr) return;
    int lines = static_cast<int>(SendMessageW(g_log, EM_GETLINECOUNT, 0, 0));
    int visible = logVisibleLines();
    ShowWindow(g_logScroll, lines > visible ? SW_SHOW : SW_HIDE);
    InvalidateRect(g_logScroll, nullptr, TRUE);
}

LRESULT CALLBACK logEditProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_MOUSEWHEEL) {
        const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        UINT wheelLines = 3;
        SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &wheelLines, 0);
        const int lines = std::max(1, static_cast<int>(wheelLines));
        const int steps = delta / WHEEL_DELTA;
        if (steps != 0) SendMessageW(control, EM_LINESCROLL, 0, -steps * lines);
        updateLogScroll();
        return 0;
    }
    return CallWindowProcW(g_logOriginalProc, control, message, wParam, lParam);
}

LRESULT CALLBACK logScrollProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_PAINT) {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(control, &paint);
        RECT bounds = {};
        GetClientRect(control, &bounds);
        FillRect(dc, &bounds, g_editBackground);
        int lines = static_cast<int>(SendMessageW(g_log, EM_GETLINECOUNT, 0, 0));
        int visible = logVisibleLines();
        int range = std::max(0, lines - visible);
        if (range > 0) {
            int height = bounds.bottom - bounds.top;
            int thumbHeight = std::max(18, height * visible / lines);
            int first = static_cast<int>(SendMessageW(g_log, EM_GETFIRSTVISIBLELINE, 0, 0));
            int top = bounds.top + (height - thumbHeight) * std::min(first, range) / range;
            HBRUSH thumb = CreateSolidBrush(RGB(236, 190, 91));
            HGDIOBJ old = SelectObject(dc, thumb);
            SelectObject(dc, GetStockObject(NULL_PEN));
            RoundRect(dc, bounds.left + 3, top, bounds.right - 3, top + thumbHeight, 8, 8);
            SelectObject(dc, old);
            DeleteObject(thumb);
        }
        EndPaint(control, &paint);
        return 0;
    }
    if (message == WM_MOUSEWHEEL) {
        SendMessageW(g_log, message, wParam, lParam);
        updateLogScroll();
        return 0;
    }
    if (message == WM_LBUTTONDOWN || (message == WM_MOUSEMOVE && g_logDragging) || message == WM_LBUTTONUP) {
        const bool wasDragging = g_logDragging;
        if (message == WM_LBUTTONDOWN) { g_logDragging = true; SetCapture(control); }
        if (message == WM_LBUTTONUP) g_logDragging = false;
        if (g_logDragging || wasDragging) {
            RECT bounds = {};
            GetClientRect(control, &bounds);
            int lines = static_cast<int>(SendMessageW(g_log, EM_GETLINECOUNT, 0, 0));
            int visible = logVisibleLines();
            int range = std::max(0, lines - visible);
            int height = std::max(1, static_cast<int>(bounds.bottom - bounds.top));
            int y = static_cast<int>(GET_Y_LPARAM(lParam)) - bounds.top;
            int first = std::clamp(y * range / height, 0, range);
            int current = static_cast<int>(SendMessageW(g_log, EM_GETFIRSTVISIBLELINE, 0, 0));
            SendMessageW(g_log, EM_LINESCROLL, 0, first - current);
            updateLogScroll();
        }
        if (message == WM_LBUTTONUP) ReleaseCapture();
        return 0;
    }
    return DefWindowProcW(control, message, wParam, lParam);
}

std::wstring utf8ToWide(const std::string& text)
{
    if (text.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 1) return {};
    std::wstring out(static_cast<size_t>(size), L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), size) == 0) return {};
    out.resize(static_cast<size_t>(size - 1));
    return out;
}

std::string wideToUtf8(const std::wstring& text)
{
    if (text.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string out(static_cast<size_t>(size), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), size, nullptr, nullptr) == 0) return {};
    out.resize(static_cast<size_t>(size - 1));
    return out;
}

std::wstring tr(const wchar_t* english, const wchar_t* russian)
{
    return g_uiLanguage == L"ru" ? russian : english;
}

std::wstring appDataSettingsPath()
{
    wchar_t path[MAX_PATH] = {};
    DWORD size = GetEnvironmentVariableW(L"APPDATA", path, MAX_PATH);
    if (size == 0 || size >= MAX_PATH) return L"launcher.ini";
    fs::path dir = fs::path(path) / L"Fallout2Reimagined";
    std::error_code error;
    fs::create_directories(dir, error);
    return (dir / L"launcher.ini").wstring();
}

void saveSettings()
{
    WritePrivateProfileStringW(L"launcher", L"language", g_uiLanguage.c_str(), g_settingsPath.c_str());
    WritePrivateProfileStringW(L"launcher", L"auto_updates", g_autoUpdates ? L"1" : L"0", g_settingsPath.c_str());
    if (g_root != nullptr) {
        wchar_t value[32768] = {};
        GetWindowTextW(g_root, value, static_cast<int>(std::size(value)));
        WritePrivateProfileStringW(L"launcher", L"game_root", value, g_settingsPath.c_str());
    }
}

std::wstring exePath()
{
    wchar_t path[32768] = {};
    DWORD size = GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
    return size == 0 || size >= std::size(path) ? std::wstring() : std::wstring(path, size);
}

std::wstring quote(const std::wstring& value)
{
    std::wstring out = L"\"";
    for (wchar_t ch : value) {
        if (ch == L'"') out += L"\\\"";
        else out += ch;
    }
    out += L'"';
    return out;
}

void postLog(const std::string& line)
{
    if (g_window == nullptr) return;
    auto* text = new std::wstring(utf8ToWide(line));
    if (!PostMessageW(g_window, WM_LOG_LINE, 0, reinterpret_cast<LPARAM>(text))) delete text;
}

DWORD runChild(const std::vector<std::wstring>& arguments)
{
    SECURITY_ATTRIBUTES security = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &security, 0)) {
        postLog("ERROR: could not create the launcher's output pipe");
        return 1;
    }
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    std::wstring command = quote(exePath());
    for (const std::wstring& argument : arguments) command += L" " + quote(argument);
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdOutput = writePipe;
    startup.hStdError = writePipe;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION process = {};
    BOOL started = CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
        nullptr, nullptr, &startup, &process);
    CloseHandle(writePipe);
    if (!started) {
        CloseHandle(readPipe);
        postLog("ERROR: could not start the installer worker");
        return 1;
    }

    std::string pending;
    char buffer[2048];
    DWORD read = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        pending.append(buffer, read);
        size_t newline = 0;
        while ((newline = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, newline);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            postLog(line);
            pending.erase(0, newline + 1);
        }
    }
    if (!pending.empty()) postLog(pending);
    CloseHandle(readPipe);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exitCode;
}

void setStatus(const std::wstring& text)
{
    if (g_status != nullptr) SetWindowTextW(g_status, text.c_str());
}

void refreshStatusFromRelease()
{
    if (g_busy) return;
    if (g_operationCompleted) {
        setStatus(g_lastRollback
            ? tr(L"The previous build was restored.", L"Предыдущая сборка восстановлена.")
            : tr(L"Installation complete. Press Play to start.", L"Установка завершена. Нажмите «Играть»."));
        return;
    }
    wchar_t root[32768] = {};
    GetWindowTextW(g_root, root, static_cast<int>(std::size(root)));
    if (root[0] == L'\0') {
        setStatus(tr(L"Choose the Fallout 2 folder to continue.", L"Выберите папку Fallout 2, чтобы продолжить."));
        return;
    }
    if (!g_haveRelease) {
        setStatus(g_installed
            ? tr(L"Offline: you can play the installed build.", L"Нет сети: можно запустить установленную сборку.")
            : tr(L"Connect to the internet to install the game.", L"Для установки игры нужно подключение к интернету."));
        return;
    }
    if (!g_installed) {
        setStatus(tr(L"Choose Install / Update to install the latest build.", L"Нажмите «Установить / обновить», чтобы установить последнюю сборку."));
        return;
    }
    std::ifstream installed(fs::path(root) / L"version.txt");
    std::string installedVersion;
    if (installed) installedVersion.assign((std::istreambuf_iterator<char>(installed)), std::istreambuf_iterator<char>());
    setStatus(installedVersion == wideToUtf8(g_releaseVersion)
        ? tr(L"Your game is up to date.", L"У вас установлена последняя сборка.")
        : tr(L"A newer build is available.", L"Доступна новая сборка."));
}

void setBusy(bool busy)
{
    g_busy = busy;
    EnableWindow(g_install, !busy && GetWindowTextLengthW(g_root) != 0);
    EnableWindow(g_browse, !busy);
    EnableWindow(g_russian, !busy);
    EnableWindow(g_autoUpdate, !busy);
    EnableWindow(g_rollback, !busy && g_installed);
    EnableWindow(g_play, !busy && g_installed);
    if (busy) {
        g_progressValue = 2;
        InvalidateRect(g_progress, nullptr, TRUE);
    } else if (g_progressValue < 100) {
        g_progressValue = 0;
        InvalidateRect(g_progress, nullptr, TRUE);
    }
}

void appendLog(const std::wstring& line)
{
    int length = GetWindowTextLengthW(g_log);
    SendMessageW(g_log, EM_SETSEL, length, length);
    std::wstring text = line + L"\r\n";
    SendMessageW(g_log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    g_progressValue = std::min(95, g_progressValue + 2);
    InvalidateRect(g_progress, nullptr, TRUE);
    updateLogScroll();
}

void updateLabels()
{
    SetWindowTextW(g_window, tr(L"Fallout 2: Reimagined", L"Fallout 2: Reimagined").c_str());
    SetWindowTextW(GetDlgItem(g_window, ID_GAME_ROOT - 100), tr(L"Game folder", L"Папка игры").c_str());
    SetWindowTextW(g_pathHint, tr(L"Choose the folder that contains master.dat and critter.dat.",
        L"Выберите папку, где находятся master.dat и critter.dat.").c_str());
    SetWindowTextW(g_browse, tr(L"Browse...", L"Обзор...").c_str());
    SetWindowTextW(g_russian, tr(L"Install Russian text from the official RPU release", L"Установить русский текст из официального релиза RPU").c_str());
    SetWindowTextW(g_rp, tr(L"Restoration Project: coming soon", L"Restoration Project: скоро").c_str());
    SetWindowTextW(g_autoUpdate, tr(L"Install updates automatically on launch", L"Устанавливать обновления при запуске").c_str());
    SetWindowTextW(g_install, tr(L"Install / Update", L"Установить / обновить").c_str());
    SetWindowTextW(g_rollback, tr(L"Roll back", L"Откатить").c_str());
    SetWindowTextW(g_play, tr(L"Play", L"Играть").c_str());
    std::wstring release = g_releaseVersion.empty()
        ? tr(L"Checking for the latest version...", L"Проверяем последнюю версию...")
        : (tr(L"Latest build: ", L"Последняя сборка: ") + g_releaseVersion);
    std::wstring notes = g_releaseNotes.empty()
        ? tr(L"Release notes will appear here.", L"Здесь появятся заметки к релизу.")
        : g_releaseNotes;
    SetWindowTextW(g_release, release.c_str());
    SetWindowTextW(g_notes, notes.c_str());
    InvalidateRect(g_window, nullptr, TRUE);
}

void loadSettings()
{
    g_settingsPath = g_args.settingsFile.empty() ? appDataSettingsPath() : g_args.settingsFile;
    wchar_t language[16] = {};
    GetPrivateProfileStringW(L"launcher", L"language", L"", language,
        static_cast<DWORD>(std::size(language)), g_settingsPath.c_str());
    if (language[0] == L'e' || language[0] == L'r') {
        g_uiLanguage = language;
    } else {
        g_uiLanguage = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_RUSSIAN ? L"ru" : L"en";
    }
    g_autoUpdates = GetPrivateProfileIntW(L"launcher", L"auto_updates", 0, g_settingsPath.c_str()) != 0;
    if (g_args.gameRoot.empty()) {
        wchar_t path[32768] = {};
        GetPrivateProfileStringW(L"launcher", L"game_root", L"", path,
            static_cast<DWORD>(std::size(path)), g_settingsPath.c_str());
        if (path[0] != L'\0') g_args.gameRoot = path;
    }
    if (g_args.gameRoot.empty()) {
        // The command-line install mode keeps registry discovery; the GUI
        // never opens an automatic folder picker before the user chooses it.
        g_args.gameRoot.clear();
    }
}

std::vector<std::wstring> workerArguments(bool rollback)
{
    std::vector<std::wstring> args = {L"--game-root", g_args.gameRoot, L"--no-start"};
    if (!g_args.updateUrl.empty()) {
        args.push_back(L"--update-url");
        args.push_back(g_args.updateUrl);
    }
    if (rollback) {
        args = {L"--game-root", g_args.gameRoot, L"--rollback"};
    } else if (g_russianInstall) {
        args.push_back(L"--russian");
    }
    return args;
}

void startOperation(bool rollback)
{
    if (g_busy) return;
    wchar_t path[32768] = {};
    GetWindowTextW(g_root, path, static_cast<int>(std::size(path)));
    g_args.gameRoot = path;
    if (g_args.gameRoot.empty()) {
        setStatus(tr(L"Choose the folder containing master.dat and critter.dat.",
            L"Выберите папку с файлами master.dat и critter.dat."));
        return;
    }
    saveSettings();
    g_operationCompleted = false;
    setBusy(true);
    setStatus(rollback ? tr(L"Restoring the previous build...", L"Восстанавливаем предыдущую сборку...")
                       : tr(L"Installing or updating the game...", L"Устанавливаем или обновляем игру..."));
    std::vector<std::wstring> args = workerArguments(rollback);
    std::thread([args = std::move(args), rollback]() {
        DWORD result = runChild(args);
        PostMessageW(g_window, WM_OPERATION_DONE, result, rollback ? 1 : 0);
    }).detach();
}

void checkReleaseAsync()
{
    std::vector<std::wstring> args = {L"--check-release"};
    if (!g_args.updateUrl.empty()) {
        args.push_back(L"--update-url");
        args.push_back(g_args.updateUrl);
    }
    if (!g_args.releaseInfoUrl.empty()) {
        args.push_back(L"--release-info-url");
        args.push_back(g_args.releaseInfoUrl);
    }
    std::thread([args = std::move(args)]() {
        SECURITY_ATTRIBUTES security = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
        HANDLE readPipe = nullptr;
        HANDLE writePipe = nullptr;
        ReleaseInfo* info = new ReleaseInfo();
        if (!CreatePipe(&readPipe, &writePipe, &security, 0)) {
            if (!PostMessageW(g_window, WM_RELEASE_INFO, 0, reinterpret_cast<LPARAM>(info))) delete info;
            return;
        }
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
        std::wstring command = quote(exePath());
        for (const std::wstring& arg : args) command += L" " + quote(arg);
        STARTUPINFOW startup = {};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_HIDE;
        startup.hStdOutput = writePipe;
        startup.hStdError = writePipe;
        startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        PROCESS_INFORMATION process = {};
        BOOL started = CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
            nullptr, nullptr, &startup, &process);
        CloseHandle(writePipe);
        if (started) {
            std::string output;
            char buffer[2048];
            DWORD read = 0;
            while (ReadFile(readPipe, buffer, sizeof(buffer), &read, nullptr) && read > 0) output.append(buffer, read);
            WaitForSingleObject(process.hProcess, INFINITE);
            DWORD code = 1;
            GetExitCodeProcess(process.hProcess, &code);
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
            auto value = [&output](const std::string& prefix) {
                size_t pos = output.find(prefix);
                if (pos == std::string::npos) return std::string();
                pos += prefix.size();
                size_t end = output.find_first_of("\r\n", pos);
                return output.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
            };
            info->version = utf8ToWide(value("RELEASE_VERSION:"));
            info->notes = utf8ToWide(value("RELEASE_NOTES:"));
            info->online = code == 0 && !info->version.empty();
        }
        CloseHandle(readPipe);
        if (!PostMessageW(g_window, WM_RELEASE_INFO, 0, reinterpret_cast<LPARAM>(info))) delete info;
    }).detach();
}

void launchGame()
{
    wchar_t root[32768] = {};
    GetWindowTextW(g_root, root, static_cast<int>(std::size(root)));
    if (root[0] == L'\0') return;
    fs::path game = fs::path(root) / L"fallout2-ce.exe";
    if (!fs::exists(game)) {
        setStatus(tr(L"Install the game before pressing Play.", L"Сначала установите игру, затем нажмите «Играть»."));
        return;
    }
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    std::wstring command = quote(game.wstring());
    if (!CreateProcessW(game.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, root, &startup, &process)) {
        setStatus(tr(L"Could not start the game.", L"Не удалось запустить игру."));
        return;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    setStatus(tr(L"The game is running.", L"Игра запущена."));
}

void chooseGameFolder()
{
    IFileDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog))) || dialog == nullptr) return;
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    dialog->SetTitle(tr(L"Choose the folder containing master.dat", L"Выберите папку с файлом master.dat").c_str());
    IShellItem* result = nullptr;
    if (SUCCEEDED(dialog->Show(g_window)) && SUCCEEDED(dialog->GetResult(&result)) && result != nullptr) {
        PWSTR path = nullptr;
        if (SUCCEEDED(result->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path != nullptr) {
            fs::path selected(path);
            CoTaskMemFree(path);
            if (fs::exists(selected / L"master.dat") && fs::exists(selected / L"critter.dat")) {
                SetWindowTextW(g_root, selected.wstring().c_str());
                saveSettings();
                EnableWindow(g_install, TRUE);
                std::wstring versionPath = (selected / L"version.txt").wstring();
                std::wstring exe = (selected / L"fallout2-ce.exe").wstring();
                g_installed = fs::exists(exe);
                EnableWindow(g_play, g_installed);
                EnableWindow(g_rollback, g_installed && fs::exists(selected / L"f2reimagined-backups"));
                setStatus(tr(L"Fallout 2 folder selected.", L"Папка Fallout 2 выбрана."));
            } else {
                setStatus(tr(L"This folder does not contain master.dat and critter.dat.",
                    L"В этой папке нет master.dat и critter.dat."));
            }
        }
        result->Release();
    }
    dialog->Release();
}

void createControl(HWND window, const wchar_t* klass, const wchar_t* title, DWORD style,
    int x, int y, int width, int height, int id)
{
    HWND control = CreateWindowExW(0, klass, title, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
    if (control != nullptr && g_font != nullptr) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
}

void createControls(HWND window)
{
    g_font = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_headingFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    createControl(window, L"STATIC", L"Game folder", SS_LEFT, 24, 186, 580, 22, ID_GAME_ROOT - 100);
    SendMessageW(GetDlgItem(window, ID_GAME_ROOT - 100), WM_SETFONT, reinterpret_cast<WPARAM>(g_headingFont), TRUE);
    createControl(window, L"STATIC", L"Choose the folder that contains master.dat and critter.dat.",
        SS_LEFT, 24, 210, 580, 20, ID_PATH_HINT);
    g_pathHint = GetDlgItem(window, ID_PATH_HINT);
    createControl(window, L"EDIT", L"", ES_AUTOHSCROLL, 25, 245, 603, 25, ID_GAME_ROOT);
    g_root = GetDlgItem(window, ID_GAME_ROOT);
    SendMessageW(g_root, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(10, 0));
    createControl(window, L"BUTTON", L"Browse...", BS_OWNERDRAW, 642, 234, 130, 38, ID_BROWSE);
    g_browse = GetDlgItem(window, ID_BROWSE);
    createControl(window, L"BUTTON", L"Install Russian text from the official RPU release",
        BS_OWNERDRAW | BS_NOTIFY, 24, 284, 690, 28, ID_RUSSIAN);
    g_russian = GetDlgItem(window, ID_RUSSIAN);
    createControl(window, L"BUTTON", L"Restoration Project: coming soon", BS_OWNERDRAW,
        24, 316, 690, 28, ID_RP);
    g_rp = GetDlgItem(window, ID_RP);
    EnableWindow(g_rp, FALSE);
    createControl(window, L"BUTTON", L"Install updates automatically on launch",
        BS_OWNERDRAW | BS_NOTIFY, 24, 348, 690, 28, ID_AUTO_UPDATE);
    g_autoUpdate = GetDlgItem(window, ID_AUTO_UPDATE);
    createControl(window, L"STATIC", L"", SS_LEFT, 24, 398, 752, 24, ID_RELEASE);
    g_release = GetDlgItem(window, ID_RELEASE);
    createControl(window, L"STATIC", L"", SS_LEFT | SS_NOPREFIX, 24, 424, 752, 52, ID_NOTES);
    g_notes = GetDlgItem(window, ID_NOTES);
    createControl(window, L"BUTTON", L"", BS_OWNERDRAW, 24, 485, 752, 20, ID_PROGRESS);
    g_progress = GetDlgItem(window, ID_PROGRESS);
    createControl(window, L"STATIC", L"", SS_LEFT, 24, 511, 752, 22, ID_STATUS);
    g_status = GetDlgItem(window, ID_STATUS);
    createControl(window, L"EDIT", L"", ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        30, 544, 726, 78, ID_LOG);
    g_log = GetDlgItem(window, ID_LOG);
    g_logOriginalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_log, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(logEditProc)));
    SetWindowLongPtrW(g_log, GWL_STYLE, GetWindowLongPtrW(g_log, GWL_STYLE) & ~WS_BORDER);
    g_logScroll = CreateWindowExW(0, L"F2CELogScroll", L"", WS_CHILD,
        758, 544, 16, 78, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LOG_SCROLL)),
        GetModuleHandleW(nullptr), nullptr);
    createControl(window, L"BUTTON", L"Install / Update", BS_OWNERDRAW, 24, 660, 200, 36, ID_INSTALL);
    g_install = GetDlgItem(window, ID_INSTALL);
    createControl(window, L"BUTTON", L"Roll back", BS_OWNERDRAW, 236, 660, 150, 36, ID_ROLLBACK);
    g_rollback = GetDlgItem(window, ID_ROLLBACK);
    createControl(window, L"BUTTON", L"Play", BS_OWNERDRAW, 626, 660, 150, 36, ID_PLAY);
    g_play = GetDlgItem(window, ID_PLAY);
    EnableWindow(g_play, FALSE);
    EnableWindow(g_rollback, FALSE);

    HWND languageLabel = CreateWindowExW(0, L"STATIC", L"Language", WS_CHILD | WS_VISIBLE | SS_LEFT,
        614, 18, 150, 24, window, nullptr, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(languageLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
    g_language = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        614, 43, 150, 38, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LANGUAGE)),
        GetModuleHandleW(nullptr), nullptr);
    SendMessageW(g_language, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);

    g_logo = LoadBitmapW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDB_LAUNCHER_LOGO));
    if (!g_args.gameRoot.empty()) SetWindowTextW(g_root, g_args.gameRoot.c_str());
    updateLabels();
}

void paintLogo(HDC dc)
{
    if (g_logo == nullptr) return;
    HDC memory = CreateCompatibleDC(dc);
    HGDIOBJ old = SelectObject(memory, g_logo);
    BITMAP bitmap = {};
    GetObjectW(g_logo, sizeof(bitmap), &bitmap);
    SetStretchBltMode(dc, HALFTONE);
    StretchBlt(dc, 24, 18, 340, 165, memory, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
    SelectObject(memory, old);
    DeleteDC(memory);
}

LRESULT CALLBACK launcherWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_NCCREATE:
        g_window = window;
        return DefWindowProcW(window, message, wParam, lParam);
    case WM_CREATE:
        createControls(window);
        SetTimer(window, 1, 100, nullptr);
        return 0;
    case WM_TIMER:
        updateLogScroll();
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(window, &paint);
        RECT client = {};
        GetClientRect(window, &client);
        FillRect(dc, &client, g_background);
        HPEN fieldPen = CreatePen(PS_SOLID, 1, RGB(140, 112, 69));
        HGDIOBJ oldFieldPen = SelectObject(dc, fieldPen);
        HGDIOBJ oldFieldBrush = SelectObject(dc, g_editBackground);
        RoundRect(dc, 24, 234, 630, 272, 12, 12);
        SelectObject(dc, oldFieldBrush);
        SelectObject(dc, oldFieldPen);
        DeleteObject(fieldPen);
        HPEN logPen = CreatePen(PS_SOLID, 1, RGB(100, 111, 111));
        HBRUSH logBrush = CreateSolidBrush(RGB(8, 13, 19));
        HGDIOBJ oldLogPen = SelectObject(dc, logPen);
        HGDIOBJ oldLogBrush = SelectObject(dc, logBrush);
        RoundRect(dc, 24, 538, 776, 628, 12, 12);
        SelectObject(dc, oldLogBrush);
        SelectObject(dc, oldLogPen);
        DeleteObject(logBrush);
        DeleteObject(logPen);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(140, 112, 69));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
        paintLogo(dc);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        if (reinterpret_cast<HWND>(lParam) == g_log) {
            SetTextColor(dc, RGB(234, 231, 218));
            SetBkColor(dc, RGB(8, 13, 19));
            SetBkMode(dc, OPAQUE);
            return reinterpret_cast<LRESULT>(g_editBackground);
        }
        SetTextColor(dc, RGB(228, 219, 191));
        SetBkColor(dc, RGB(18, 25, 34));
        SetBkMode(dc, OPAQUE);
        return reinterpret_cast<LRESULT>(g_background);
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, RGB(234, 231, 218));
        SetBkColor(dc, RGB(8, 13, 19));
        return reinterpret_cast<LRESULT>(g_editBackground);
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (item->CtlID == ID_RUSSIAN || item->CtlID == ID_RP || item->CtlID == ID_AUTO_UPDATE) {
            const bool enabled = (item->itemState & ODS_DISABLED) == 0;
            const bool checked = item->CtlID == ID_RUSSIAN ? g_russianInstall
                : item->CtlID == ID_AUTO_UPDATE ? g_autoUpdates : false;
            RECT box = item->rcItem;
            box.left += 2;
            box.top += (box.bottom - box.top - 18) / 2;
            box.right = box.left + 18;
            box.bottom = box.top + 18;
            HBRUSH background = CreateSolidBrush(RGB(18, 25, 34));
            FillRect(item->hDC, &item->rcItem, background);
            DeleteObject(background);
            HBRUSH fill = CreateSolidBrush(checked ? RGB(236, 190, 91) : RGB(8, 13, 19));
            HPEN border = CreatePen(PS_SOLID, 1, RGB(140, 112, 69));
            HGDIOBJ oldBrush = SelectObject(item->hDC, fill);
            HGDIOBJ oldPen = SelectObject(item->hDC, border);
            RoundRect(item->hDC, box.left, box.top, box.right, box.bottom, 6, 6);
            SelectObject(item->hDC, oldBrush);
            SelectObject(item->hDC, oldPen);
            DeleteObject(fill);
            DeleteObject(border);
            if (checked) {
                HPEN checkPen = CreatePen(PS_SOLID, 2, RGB(18, 25, 34));
                oldPen = SelectObject(item->hDC, checkPen);
                MoveToEx(item->hDC, box.left + 4, box.top + 9, nullptr);
                LineTo(item->hDC, box.left + 8, box.top + 13);
                LineTo(item->hDC, box.left + 15, box.top + 5);
                SelectObject(item->hDC, oldPen);
                DeleteObject(checkPen);
            }
            wchar_t label[160] = {};
            GetWindowTextW(item->hwndItem, label, static_cast<int>(std::size(label)));
            RECT labelRect = item->rcItem;
            labelRect.left = box.right + 9;
            SetBkMode(item->hDC, TRANSPARENT);
            SetTextColor(item->hDC, enabled ? RGB(228, 219, 191) : RGB(95, 99, 101));
            DrawTextW(item->hDC, label, -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            return TRUE;
        }
        if (item->CtlID == ID_PROGRESS) {
            RECT r = item->rcItem;
            HBRUSH bg = CreateSolidBrush(RGB(32, 39, 47));
            HBRUSH fg = CreateSolidBrush(RGB(155, 180, 105));
            HGDIOBJ oldBrush = SelectObject(item->hDC, bg);
            HGDIOBJ oldPen = SelectObject(item->hDC, GetStockObject(NULL_PEN));
            RoundRect(item->hDC, r.left, r.top, r.right, r.bottom, 12, 12);
            if (g_progressValue > 0) {
                SelectObject(item->hDC, fg);
                const int width = (r.right - r.left) * g_progressValue / 100;
                RoundRect(item->hDC, r.left, r.top, r.left + width, r.bottom, 12, 12);
            }
            SelectObject(item->hDC, oldBrush);
            SelectObject(item->hDC, oldPen);
            DeleteObject(bg);
            DeleteObject(fg);
            return TRUE;
        }
        if (item->CtlID != ID_LANGUAGE && item->CtlID != ID_BROWSE && item->CtlID != ID_INSTALL && item->CtlID != ID_ROLLBACK && item->CtlID != ID_PLAY) break;
        bool enabled = (item->itemState & ODS_DISABLED) == 0;
        COLORREF color = item->CtlID == ID_LANGUAGE ? RGB(8, 13, 19)
            : (enabled ? RGB(48, 58, 62) : RGB(34, 38, 41));
        HBRUSH brush = CreateSolidBrush(color);
        HPEN borderPen = CreatePen(PS_SOLID, 1, item->CtlID == ID_LANGUAGE ? RGB(140, 112, 69) : RGB(100, 111, 111));
        RECT rect = item->rcItem;
        HGDIOBJ oldBrush = SelectObject(item->hDC, brush);
        HGDIOBJ oldPen = SelectObject(item->hDC, borderPen);
        RoundRect(item->hDC, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
        SelectObject(item->hDC, oldBrush);
        SelectObject(item->hDC, oldPen);
        DeleteObject(brush);
        DeleteObject(borderPen);
        SetBkMode(item->hDC, TRANSPARENT);
        SetTextColor(item->hDC, enabled ? RGB(236, 210, 139) : RGB(125, 126, 121));
        wchar_t label[128] = {};
        if (item->CtlID == ID_LANGUAGE) {
            wcscpy_s(label, g_uiLanguage == L"ru" ? L"Русский" : L"English");
            RECT labelRect = item->rcItem;
            labelRect.left += 8;
            labelRect.right -= 20;
            DrawTextW(item->hDC, label, -1, &labelRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            POINT arrow[] = {{item->rcItem.right - 15, item->rcItem.top + 15},
                {item->rcItem.right - 7, item->rcItem.top + 15},
                {item->rcItem.right - 11, item->rcItem.top + 20}};
            HBRUSH arrowBrush = CreateSolidBrush(RGB(236, 210, 139));
            HGDIOBJ oldArrowBrush = SelectObject(item->hDC, arrowBrush);
            Polygon(item->hDC, arrow, static_cast<int>(std::size(arrow)));
            SelectObject(item->hDC, oldArrowBrush);
            DeleteObject(arrowBrush);
        } else {
            GetWindowTextW(item->hwndItem, label, static_cast<int>(std::size(label)));
            DrawTextW(item->hDC, label, -1, &item->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_GAME_ROOT && HIWORD(wParam) == EN_CHANGE) {
            EnableWindow(g_install, !g_busy && GetWindowTextLengthW(g_root) != 0);
        } else if (LOWORD(wParam) == ID_LANGUAGE && HIWORD(wParam) == BN_CLICKED) {
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING | (g_uiLanguage == L"en" ? MF_CHECKED : 0), 1, L"English");
            AppendMenuW(menu, MF_STRING | (g_uiLanguage == L"ru" ? MF_CHECKED : 0), 2, L"Русский");
            RECT rect = {};
            GetWindowRect(g_language, &rect);
            int choice = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, rect.left, rect.bottom, 0, window, nullptr);
            DestroyMenu(menu);
            if (choice == 1 || choice == 2) {
                g_uiLanguage = choice == 2 ? L"ru" : L"en";
                saveSettings();
                updateLabels();
                refreshStatusFromRelease();
                InvalidateRect(g_language, nullptr, TRUE);
            }
        } else if (LOWORD(wParam) == ID_BROWSE) {
            chooseGameFolder();
        } else if (LOWORD(wParam) == ID_AUTO_UPDATE && HIWORD(wParam) == BN_CLICKED) {
            g_autoUpdates = !g_autoUpdates;
            InvalidateRect(g_autoUpdate, nullptr, TRUE);
            saveSettings();
        } else if (LOWORD(wParam) == ID_RUSSIAN && HIWORD(wParam) == BN_CLICKED) {
            g_russianInstall = !g_russianInstall;
            InvalidateRect(g_russian, nullptr, TRUE);
        } else if (LOWORD(wParam) == ID_INSTALL && HIWORD(wParam) == BN_CLICKED) {
            startOperation(false);
        } else if (LOWORD(wParam) == ID_ROLLBACK && HIWORD(wParam) == BN_CLICKED) {
            startOperation(true);
        } else if (LOWORD(wParam) == ID_PLAY && HIWORD(wParam) == BN_CLICKED) {
            launchGame();
        }
        return 0;
    case WM_RELEASE_INFO: {
        std::unique_ptr<ReleaseInfo> info(reinterpret_cast<ReleaseInfo*>(lParam));
        g_releaseVersion = info->version;
        g_releaseNotes = info->notes;
        g_haveRelease = info->online;
        updateLabels();
        wchar_t root[32768] = {};
        GetWindowTextW(g_root, root, static_cast<int>(std::size(root)));
        if (root[0] != L'\0') {
            fs::path gameRoot(root);
            g_installed = fs::exists(gameRoot / L"fallout2-ce.exe");
            EnableWindow(g_play, g_installed);
            EnableWindow(g_rollback, g_installed && fs::exists(gameRoot / L"f2reimagined-backups"));
            EnableWindow(g_install, true);
            if (g_busy || g_operationCompleted) return 0;
            std::ifstream installed(gameRoot / L"version.txt");
            std::string installedVersion;
            if (installed) installedVersion.assign((std::istreambuf_iterator<char>(installed)), std::istreambuf_iterator<char>());
            if (!g_haveRelease) {
                setStatus(g_installed ? tr(L"Offline: you can play the installed build.", L"Нет сети: можно запустить установленную сборку.")
                                      : tr(L"Connect to the internet to install the game.", L"Для установки игры нужно подключение к интернету."));
            } else if (g_autoUpdates && g_installed && installedVersion != wideToUtf8(g_releaseVersion)) {
                startOperation(false);
            } else if (!g_installed) {
                setStatus(tr(L"Choose Install / Update to install the latest build.", L"Нажмите «Установить / обновить», чтобы установить последнюю сборку."));
            } else if (installedVersion != wideToUtf8(g_releaseVersion)) {
                setStatus(tr(L"A newer build is available.", L"Доступна новая сборка."));
            } else {
                setStatus(tr(L"Your game is up to date.", L"У вас установлена последняя сборка."));
            }
        } else {
            setStatus(tr(L"Choose the Fallout 2 folder to continue.", L"Выберите папку Fallout 2, чтобы продолжить."));
        }
        return 0;
    }
    case WM_LOG_LINE: {
        std::unique_ptr<std::wstring> line(reinterpret_cast<std::wstring*>(lParam));
        appendLog(*line);
        return 0;
    }
    case WM_OPERATION_DONE: {
        DWORD result = static_cast<DWORD>(wParam);
        bool rollback = lParam != 0;
        setBusy(false);
        if (result == 0) {
            g_operationCompleted = true;
            g_lastRollback = rollback;
            g_progressValue = 100;
            InvalidateRect(g_progress, nullptr, TRUE);
            setStatus(rollback ? tr(L"The previous build was restored.", L"Предыдущая сборка восстановлена.")
                               : tr(L"Installation complete. Press Play to start.", L"Установка завершена. Нажмите «Играть»."));
            g_installed = true;
            EnableWindow(g_play, TRUE);
            EnableWindow(g_rollback, fs::exists(fs::path(g_args.gameRoot) / L"f2reimagined-backups"));
        } else {
            setStatus(tr(L"The operation failed. See the log below.", L"Операция не выполнена. Подробности ниже."));
        }
        saveSettings();
        if (result == 0) checkReleaseAsync();
        return 0;
    }
    case WM_DESTROY:
        saveSettings();
        if (g_logo != nullptr) DeleteObject(g_logo);
        if (g_font != nullptr) DeleteObject(g_font);
        if (g_headingFont != nullptr) DeleteObject(g_headingFont);
        if (g_background != nullptr) DeleteObject(g_background);
        if (g_editBackground != nullptr) DeleteObject(g_editBackground);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

UiArgs parseUiArgs(int argc, char** argv)
{
    UiArgs args;
    for (int i = 1; i < argc; i++) {
        std::string key = argv[i];
        if (key == "--no-activate") {
            args.noActivate = true;
            continue;
        }
        if ((key == "--game-root" || key == "--update-url" || key == "--release-info-url" || key == "--settings-file") && i + 1 < argc) {
            std::wstring value = utf8ToWide(argv[++i]);
            if (key == "--game-root") args.gameRoot = value;
            else if (key == "--update-url") args.updateUrl = value;
            else if (key == "--release-info-url") args.releaseInfoUrl = value;
            else args.settingsFile = value;
        }
    }
    return args;
}

} // namespace

int runLauncherUi(int argc, char** argv)
{
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    g_args = parseUiArgs(argc, argv);
    loadSettings();
    if (g_args.gameRoot.empty()) {
        const std::string detected = findGameRootForUi();
        if (!detected.empty()) g_args.gameRoot = utf8ToWide(detected);
    }
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&controls);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    g_background = CreateSolidBrush(RGB(18, 25, 34));
    g_editBackground = CreateSolidBrush(RGB(8, 13, 19));
    WNDCLASSW wc = {};
    wc.lpfnWndProc = launcherWindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"Fallout2ReimaginedInstaller";
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = g_background;
    RegisterClassW(&wc);
    WNDCLASSW scrollClass = {};
    scrollClass.lpfnWndProc = logScrollProc;
    scrollClass.hInstance = GetModuleHandleW(nullptr);
    scrollClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32649));
    scrollClass.hbrBackground = g_editBackground;
    scrollClass.lpszClassName = L"F2CELogScroll";
    RegisterClassW(&scrollClass);
    RECT rect = {0, 0, 800, 720};
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    HWND window = CreateWindowExW(g_args.noActivate ? WS_EX_NOACTIVATE : 0, wc.lpszClassName, L"Fallout 2: Reimagined",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, wc.hInstance, nullptr);
    if (window == nullptr) {
        CoUninitialize();
        return 1;
    }
    g_window = window;
    ShowWindow(window, g_args.noActivate ? SW_SHOWNOACTIVATE : SW_SHOW);
    UpdateWindow(window);

    // Existing installs are discovered without opening the owner's game or
    // writing to it. The UI asks before it replaces the engine files.
    if (g_args.gameRoot.empty()) {
        wchar_t stored[32768] = {};
        GetPrivateProfileStringW(L"launcher", L"game_root", L"", stored,
            static_cast<DWORD>(std::size(stored)), g_settingsPath.c_str());
        if (stored[0] != L'\0' && fs::exists(fs::path(stored) / L"master.dat")
            && fs::exists(fs::path(stored) / L"critter.dat")) {
            g_args.gameRoot = stored;
        }
    }
    if (!g_args.gameRoot.empty()) {
        SetWindowTextW(g_root, g_args.gameRoot.c_str());
        g_installed = fs::exists(fs::path(g_args.gameRoot) / L"fallout2-ce.exe");
        EnableWindow(g_play, g_installed);
    }
    saveSettings();
    checkReleaseAsync();

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    CoUninitialize();
    return 0;
}
