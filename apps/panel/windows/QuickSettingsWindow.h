#pragma once

#include <libsettings/Setting.h>
#include <libutils/OwnPtr.h>
#include <libwidget/Button.h>
#include <libwidget/Window.h>

namespace panel
{

class QuickSettingsWindow : public Window
{
public:
    static constexpr int WIDTH = 320;

    QuickSettingsWindow();

private:
    Button *_wireframe_button = nullptr;

    Button *_theme_skift_dark = nullptr;
    Button *_theme_skift_light = nullptr;
    Button *_theme_ayu_dark = nullptr;
    Button *_theme_ayu_light = nullptr;
    Button *_theme_solarized_dark = nullptr;

    OwnPtr<settings::Setting> _wireframe_setting;
    OwnPtr<settings::Setting> _theme_setting;

    void layout_window();

    void update_theme_buttons(const String &active_theme);
};

} // namespace panel
