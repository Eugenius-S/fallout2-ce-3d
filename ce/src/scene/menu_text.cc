#include "menu_text.h"

#include <cstring>
#include <utility>

namespace fallout {
namespace scene {
namespace {

// Source is UTF-8 with BOM so MSVC and other compilers read the catalog
// identically. The existing pixel and sharp-text fonts consume CP1251.
std::string cp1251(const char* utf8)
{
    std::string result;
    const auto* bytes = reinterpret_cast<const unsigned char*>(utf8);
    while (*bytes != 0) {
        unsigned int code = *bytes++;
        if (code >= 0xC0 && code < 0xE0 && *bytes != 0) {
            code = ((code & 0x1F) << 6) | (*bytes++ & 0x3F);
        }
        if (code < 0x80) result += static_cast<char>(code);
        else if (code >= 0x410 && code <= 0x44F) result += static_cast<char>(code - 0x410 + 0xC0);
        else if (code == 0x401) result += static_cast<char>(0xA8);
        else if (code == 0x451) result += static_cast<char>(0xB8);
        else result += '?';
    }
    return result;
}

bool russian(const std::string& language)
{
    constexpr char expected[] = "russian";
    if (language.size() != sizeof(expected) - 1) return false;
    for (std::size_t i = 0; i < language.size(); ++i) {
        char value = language[i];
        if (value >= 'A' && value <= 'Z') value += 'a' - 'A';
        if (value != expected[i]) return false;
    }
    return true;
}

using Translation = std::pair<const char*, std::string>;
const std::vector<Translation>& catalog()
{
    static const std::vector<Translation> strings = {
        { "fly camera landed", cp1251(u8"полёт завершён") },
        { "fly camera landed: the 3D scene is not shown", cp1251(u8"полёт завершён: 3D-сцена не показана") },
        { "Enable 3D scene", cp1251(u8"Включить 3D-сцену") },
        { "Show the 3D world view where a scene is available.", cp1251(u8"Показывать мир в 3D там, где доступна сцена.") },
        { "Sharp interface text", cp1251(u8"Чёткий текст интерфейса") },
        { "Draw small interface text with a high-resolution font.", cp1251(u8"Рисовать мелкий текст интерфейса шрифтом высокого разрешения.") },
        { "Clean map edges", cp1251(u8"Чёткие границы карты") },
        { "Cut the 3D scene to the map boundary, leaving black beyond it.", cp1251(u8"Обрезать 3D-сцену по границам карты. За границами остаётся чёрный фон.") },
        { "Sun", cp1251(u8"Солнце") },
        { "A sun that follows game time on maps under the sky: low and warm in the morning and evening, none at night or underground.", cp1251(u8"Солнце следует игровому времени: утром и вечером свет низкий и тёплый. Ночью и под землёй солнца нет.") },
        { "Sun shadows", cp1251(u8"Тени от солнца") },
        { "Buildings and big things cast sharp sun shadows on the floor and the walls in the 3D view.", cp1251(u8"Здания и крупные объекты отбрасывают чёткие тени на пол и стены 3D-сцены.") },
        { "Shadow edge softness", cp1251(u8"Мягкость краёв теней") },
        { "How wide, in game pixels, a shadow's edge softens over; the shadow itself stays crisp.", cp1251(u8"Ширина смягчения края тени в игровых пикселях. Сама тень остаётся чёткой.") },
        { "Glow", cp1251(u8"Свечение") },
        { "Things that shine in Fallout 2 (fire, monitors, signs) bleed light softly in the 3D view.", cp1251(u8"Огонь, мониторы, вывески и другие источники света получают мягкое свечение в 3D.") },
        { "Glow strength", cp1251(u8"Сила свечения") },
        { "How far the glow of shining things spreads and brightens.", cp1251(u8"Насколько далеко распространяется свечение и насколько оно яркое.") },
        { "Camera rotation (experimental)", cp1251(u8"Поворот камеры (опытный)") },
        { "[ and ] turn the 3D camera in steps of 45 degrees, limited to 45 degrees either way. A turned view is for looking only: the game takes no clicks until the camera is back.", cp1251(u8"Клавиши [ и ] поворачивают камеру на 45 градусов, не дальше 45 градусов в каждую сторону. Пока камера повёрнута, можно только смотреть: верните её назад, чтобы управлять игрой мышью.") },
        { "Unlimited camera rotation (own risk)", cp1251(u8"Полный оборот камеры (на свой риск)") },
        { "Allow a full camera turn at your own risk. Beyond 45 degrees, missing backs and imperfect corners may be visible.", cp1251(u8"Разрешить полный оборот камеры на свой риск. За пределами 45 градусов могут быть видны отсутствующие задние стороны и неровные углы.") },
        { "Light", cp1251(u8"Освещение") },
        { "Light the 3D scene with the game's light.", cp1251(u8"Освещать 3D-сцену игровым светом.") },
        { "Camera zoom", cp1251(u8"Масштаб камеры") },
        { "How much the 3D view is zoomed. Kept between sessions.", cp1251(u8"Масштаб 3D-вида. Сохраняется между запусками игры.") },
        { "Camera zoom limit (smallest)", cp1251(u8"Минимальный масштаб") },
        { "The smallest 3D view zoom the mouse wheel reaches.", cp1251(u8"Минимальный масштаб 3D-вида при прокрутке колеса мыши.") },
        { "Camera zoom limit (largest)", cp1251(u8"Максимальный масштаб") },
        { "The largest 3D view zoom the mouse wheel reaches.", cp1251(u8"Максимальный масштаб 3D-вида при прокрутке колеса мыши.") },
        { "3D OPTIONS", cp1251(u8"НАСТРОЙКИ 3D") },
        { "General", cp1251(u8"Основные") },
        { "Lighting", cp1251(u8"Освещение") },
        { "Camera", cp1251(u8"Камера") },
        { "Other", cp1251(u8"Прочее") },
        { "On", cp1251(u8"Вкл") },
        { "Off", cp1251(u8"Выкл") },
        { "DEV TOOLS  (Alt+D close)", cp1251(u8"ИНСТРУМЕНТЫ (Alt+D: закрыть)") },
        { "map: %s e%d (want e%d)", cp1251(u8"карта: %s, %d>%d") },
        { "render: %s  light: off", cp1251(u8"рендер: %s  свет: выкл") },
        { "filter: %s_", cp1251(u8"поиск: %s_") },
        { "(no maps match)", cp1251(u8"(карты не найдены)") },
        { "elev:", cp1251(u8"ур.:") },
        { "<prev", cp1251(u8"<назад") },
        { "next>", cp1251(u8"далее>") },
        { "GO", cp1251(u8"ИДТИ") },
        { "SNAP", cp1251(u8"СНИМОК") },
        { "(none)", cp1251(u8"(нет)") },
        { "dev tools ready", cp1251(u8"инструменты готовы") },
        { "panel opened", cp1251(u8"панель открыта") },
        { "panel closed", cp1251(u8"панель закрыта") },
        { "no map selected", cp1251(u8"карта не выбрана") },
        { "elevation changed", cp1251(u8"уровень изменён") },
        { "3D setting is unavailable", cp1251(u8"настройка 3D недоступна") },
        { "transition rejected", cp1251(u8"переход отклонён") },
        { "the fly camera needs the 3D scene", cp1251(u8"для полёта нужна 3D-сцена") },
        { "fly camera: WASD, mouse, Q/E down/up, Shift fast, Ctrl slow; Ctrl+F12 or Esc to land", cp1251(u8"Полёт: WASD, мышь, Q/E вниз/вверх; Shift быстрее, Ctrl медленнее; Esc: выход") },
        { "refused: combat/dialog/movie", cp1251(u8"недоступно во время боя, диалога или видео") },
        { "already on this elevation", cp1251(u8"этот уровень уже открыт") },
        { "3D scene on (Ctrl+F10 to return)", cp1251(u8"3D-сцена включена (Ctrl+F10: назад)") },
        { "wall and hero preview on (Ctrl+F10 to return)", cp1251(u8"стены и герой (Ctrl+F10: назад)") },
        { "static wall preview on (Ctrl+F10 to return)", cp1251(u8"просмотр стен (Ctrl+F10: назад)") },
        { "static floor preview on (Ctrl+F10 to return)", cp1251(u8"просмотр пола (Ctrl+F10: назад)") },
        { "no such elevation: %d", cp1251(u8"нет такого уровня: %d") },
        { "elevation %d absent on this map", cp1251(u8"уровня %d нет на этой карте") },
        { "no free tile on elevation %d", cp1251(u8"на уровне %d нет свободного места") },
        { "moving to elevation %d", cp1251(u8"переход на уровень %d") },
        { "no such map: %s", cp1251(u8"нет такой карты: %s") },
        { "elevation %d absent on %s", cp1251(u8"уровня %d нет на карте %s") },
        { "cannot read map header: %s", cp1251(u8"не удалось прочитать карту: %s") },
        { "traveling to %s e%d", cp1251(u8"переход на %s, уровень %d") },
        { "window shape switch needs tools/dev-launch.ps1", cp1251(u8"для смены окна нужен tools/dev-launch.ps1") },
        { "window shape switch could not start the launcher", cp1251(u8"не удалось запустить смену окна") },
        { "window shape switch is Windows only", cp1251(u8"смена окна доступна только в Windows") },
        { "snapshot:", cp1251(u8"снимок:") },
        { "snapshot failed:", cp1251(u8"ошибка снимка:") },
        { "snapshot: no framebuffer", cp1251(u8"снимок: нет буфера кадра") },
        { "snapshot: clock unavailable", cp1251(u8"снимок: время недоступно") },
        { "snapshot: no unique filename available", cp1251(u8"снимок: нет свободного имени файла") },
        { "snapshot: no screenshot filename available", cp1251(u8"снимок: нет свободного имени файла") },
        { "snapshot: invalid framebuffer row pitch", cp1251(u8"снимок: неверный шаг строки кадра") },
        { "snapshot: vanilla PNG write failed", cp1251(u8"снимок: не удалось записать PNG") },
        { "snapshot: metadata write failed", cp1251(u8"снимок: не удалось записать метаданные") },
    };
    return strings;
}

} // namespace

const char* menuText(const char* english, const std::string& language)
{
    if (english == nullptr) return "";
    for (const auto& item : catalog()) {
        // Also recognize our Russian bytes so repeated settings initialization
        // can return to English without losing the original fallback.
        if (std::strcmp(english, item.first) == 0 || item.second == english) {
            return russian(language) ? item.second.c_str() : item.first;
        }
    }
    return english;
}

bool hasMenuTranslation(const char* english)
{
    if (english == nullptr || *english == 0) return false;
    for (const auto& item : catalog()) {
        if (std::strcmp(english, item.first) == 0) return !item.second.empty();
    }
    return false;
}

std::string menuStatusText(const std::string& status, const std::string& language)
{
    const char* translated = menuText(status.c_str(), language);
    if (translated != status.c_str()) return translated;
    const auto colon = status.find(':');
    if (colon == std::string::npos) return status;
    const std::string prefix = status.substr(0, colon + 1);
    return std::string(menuText(prefix.c_str(), language)) + status.substr(colon + 1);
}

void localizeMenuDescriptors(std::vector<SettingDescriptor>& descriptors, const std::string& language)
{
    for (auto& descriptor : descriptors) {
        if (descriptor.section != "3d") continue;
        descriptor.fallbackLabel = menuText(descriptor.fallbackLabel.c_str(), language);
        descriptor.fallbackDescription = menuText(descriptor.fallbackDescription.c_str(), language);
    }
}

} // namespace scene
} // namespace fallout
