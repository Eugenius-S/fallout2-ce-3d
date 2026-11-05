#ifndef FALLOUT_CURSOR_SENSITIVITY_H_
#define FALLOUT_CURSOR_SENSITIVITY_H_

#include <algorithm>
#include <cmath>
#include <utility>

namespace fallout {

// Reconstructs a software cursor from absolute OS pointer samples while
// preserving fractional motion at sensitivities below 1.0.
class AbsoluteCursorSensitivity {
public:
    void reset() { initialized_ = false; }

    std::pair<int, int> update(int physicalX, int physicalY, double sensitivity, int maxX, int maxY)
    {
        if (!initialized_) {
            physicalX_ = physicalX;
            physicalY_ = physicalY;
            cursorX_ = physicalX;
            cursorY_ = physicalY;
            initialized_ = true;
        } else {
            cursorX_ += (physicalX - physicalX_) * sensitivity;
            cursorY_ += (physicalY - physicalY_) * sensitivity;
            physicalX_ = physicalX;
            physicalY_ = physicalY;
        }

        cursorX_ = std::clamp(cursorX_, 0.0, static_cast<double>(std::max(maxX, 0)));
        cursorY_ = std::clamp(cursorY_, 0.0, static_cast<double>(std::max(maxY, 0)));
        return { static_cast<int>(std::lround(cursorX_)), static_cast<int>(std::lround(cursorY_)) };
    }

    void sync(int cursorX, int cursorY)
    {
        cursorX_ = cursorX;
        cursorY_ = cursorY;
    }

private:
    bool initialized_ = false;
    int physicalX_ = 0;
    int physicalY_ = 0;
    double cursorX_ = 0.0;
    double cursorY_ = 0.0;
};

} // namespace fallout

#endif
