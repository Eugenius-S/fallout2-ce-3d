#ifndef FALLOUT_SCENE_OPTIONS_PANEL_H
#define FALLOUT_SCENE_OPTIONS_PANEL_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../art.h"
#include "../options_panel_model.h"
#include "../options_settings_model.h"

namespace fallout {
namespace scene {

class OptionsPanel {
public:
    using LabelResolver = const char* (*)(int messageId);
    OptionsPanel();
    ~OptionsPanel();

    bool initialize(int windowId, unsigned char* surface, const unsigned char* background, int backgroundHeight,
        int screenWidth, int screenHeight, int pageX, int pageY);
    unsigned char* contentBuffer();
    unsigned char* footerBuffer();
    int contentHeight() const;
    int rowCount() const;
    int rowTop(int index) const;
    int rowHeight(int index) const;
    int rowButtonX() const;
    int rowButtonWidth() const;
    int sliderTrackY(int index) const;
    int findRowBySettingId(const std::string& id) const;
    std::string settingValueForProbe(const std::string& id) const;
    int scroll() const;
    int maxScroll() const;
    int scrollbarX() const;
    int scrollbarTop() const;
    int scrollbarHeight() const;
    int scrollbarThumbHeight() const;
    int scrollbarThumbTop() const;
    void mouseToPageCoordinates(int& x, int& y) const;
    void setLabelResolver(LabelResolver resolver);
    void setButtonLayout(const std::vector<std::pair<int, int>>& legacyButtons, int speedButton, int speedTop, const std::vector<int>& rowButtons);
    void setScroll(int value);
    void showPanelForSnapshot(bool bottom);
    void draw();
    void refresh();
    bool handleEvent(int eventCode);
    bool handleKey(int keyCode);
    void handleWheel();
    bool commit();
    void rollback();
    void resetToDefaults();

private:
    struct RowLayout {
        int top = 0;
        int height = 0;
    };
    struct GroupLayout {
        std::string title;
        int top = 0;
    };
    void drawRows();
    void buildRowLayout();
    void compose();
    void buildBackgroundTexture();
    unsigned char backgroundTexturePixel(int x, int y) const;
    void updateButtons();
    void showError(const std::string& message);
    bool setRowValue(int index, const SettingValue& value);
    bool stepRow(int index, int direction);
    void dragSlider(int index);
    void dragScrollbar(int y);

    int windowId_ = -1;
    unsigned char* surface_ = nullptr;
    int screenWidth_ = 640;
    int screenHeight_ = 480;
    int pageX_ = 0;
    int pageY_ = 0;
    LabelResolver labelResolver_ = nullptr;
    int contentHeight_ = 480;
    int scroll_ = 0;
    std::vector<unsigned char> content_;
    std::vector<unsigned char> background_;
    std::vector<unsigned char> backgroundTexture_;
    std::unique_ptr<FrmImage> frameImage_;
    std::unique_ptr<FrmImage> scrollbarKnob_;
    std::vector<unsigned char> rowFrameBackground_;
    std::vector<unsigned char> scrollbarGroove_;
    const unsigned char* frame_ = nullptr;
    int frameWidth_ = 0;
    int frameHeight_ = 0;
    std::vector<OptionsPanelRow> rows_;
    std::vector<int> displayOrder_;
    std::vector<RowLayout> rowLayouts_;
    std::vector<GroupLayout> groupLayouts_;
    std::unique_ptr<OptionsSettingsModel> model_;
    std::string error_;
    int selectedRow_ = 0;
    std::vector<std::pair<int, int>> legacyButtons_;
    std::unordered_map<int, int> buttonLefts_;
    std::unordered_map<int, int> buttonWidths_;
    int speedButton_ = -1;
    int speedTop_ = 0;
    std::vector<int> rowButtons_;
};

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_OPTIONS_PANEL_H */
