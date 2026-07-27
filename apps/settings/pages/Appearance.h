#pragma once

#include <libsettings/Setting.h>
#include <libutils/OwnPtr.h>
#include <libutils/Vector.h>
#include <libwidget/Button.h>
#include <libwidget/Container.h>

namespace settings
{

class MainWindow;

class AppearancePage : public Container
{
public:
    AppearancePage(Widget *parent, MainWindow *window);

private:
    Button *_wireframe_button = nullptr;

    Button *_theme_dark = nullptr;
    Button *_theme_light = nullptr;

    Button *_transparency_none = nullptr;
    Button *_transparency_slight = nullptr;
    Button *_transparency_full = nullptr;

    Vector<Button *> _wallpaper_buttons{};
    Vector<String> _wallpaper_paths{};

    OwnPtr<settings::Setting> _wireframe_setting;
    OwnPtr<settings::Setting> _theme_setting;
    OwnPtr<settings::Setting> _transparency_setting;
    OwnPtr<settings::Setting> _wallpaper_setting;

    void update_theme_buttons(const String &active_theme);
    void update_transparency_buttons(const String &active_mode);
    void update_wallpaper_buttons(const String &active_wallpaper);
};

} // namespace settings
