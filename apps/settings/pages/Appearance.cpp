#include <libwidget/Label.h>
#include <libwidget/Separator.h>

#include "settings/pages/Appearance.h"
#include "settings/windows/MainWindow.h"

namespace settings
{

static constexpr const char *WIREFRAME_SETTING_PATH = "appearance:widgets.wireframe";
static constexpr const char *THEME_SETTING_PATH = "appearance:widgets.theme";
static constexpr const char *TRANSPARENCY_SETTING_PATH = "appearance:widgets.transparency";
static constexpr const char *WALLPAPER_SETTING_PATH = "appearance:wallpaper.image";

// {file path, display label} — labels are deliberately generic rather than
// showing raw filenames, since a couple of the shipped assets still carry
// the old "skift-*" naming.
static const struct
{
    const char *path;
    const char *label;
} WALLPAPERS[] = {
    {"/Files/Wallpapers/water.png", "Water"},
    {"/Files/Wallpapers/mountains.png", "Mountains"},
    {"/Files/Wallpapers/ripples.png", "Ripples"},
    {"/Files/Wallpapers/paint.png", "Paint"},
    {"/Files/Wallpapers/brand.png", "Brand"},
    {"/Files/Wallpapers/cube-dark.png", "Cube (Dark)"},
    {"/Files/Wallpapers/cube-light.png", "Cube (Light)"},
    {"/Files/Wallpapers/skift-dark.png", "Gradient (Dark)"},
    {"/Files/Wallpapers/skift-light.png", "Gradient (Light)"},
    {"/Files/Wallpapers/light.png", "Pale"},
    {"/Files/Wallpapers/devse-chan.png", "Devse-chan"},
    {"/Files/Wallpapers/amber.png", "Amber"},
};

AppearancePage::AppearancePage(Widget *parent, MainWindow *)
    : Container(parent)
{
    layout(VFLOW(12));
    flags(Widget::FILL);
    insets(Insetsi(16));

    /* --- Theme --------------------------------------------------------------- */

    new Label(this, "Theme");

    auto themes = new Container(this);
    themes->layout(HFLOW(8));

    _theme_dark = new Button(themes, Button::OUTLINE, "Dark");
    _theme_light = new Button(themes, Button::OUTLINE, "Light");

    auto bind_theme_button = [](Button *button, const char *theme_name) {
        button->on(Event::ACTION, [theme_name](auto) {
            settings::write(settings::Path::parse(THEME_SETTING_PATH), theme_name);
        });
    };

    bind_theme_button(_theme_dark, "dark");
    bind_theme_button(_theme_light, "light");

    _theme_setting = own<settings::Setting>(THEME_SETTING_PATH, [this](const json::Value &value) {
        update_theme_buttons(value.as_string());
    });

    /* --- Wireframe ------------------------------------------------------------ */

    _wireframe_button = new Button(this, Button::OUTLINE, "Toggle Wireframe");

    _wireframe_button->on(Event::ACTION, [](auto) {
        auto current = settings::read(settings::Path::parse(WIREFRAME_SETTING_PATH));
        bool enabled = current.present() && current.value().as_bool();

        settings::write(settings::Path::parse(WIREFRAME_SETTING_PATH), !enabled);
    });

    _wireframe_setting = own<settings::Setting>(WIREFRAME_SETTING_PATH, [this](const json::Value &value) {
        _wireframe_button->style(value.as_bool() ? Button::FILLED : Button::OUTLINE);
    });

    new Separator(this);

    /* --- Transparency ----------------------------------------------------------- */

    new Label(this, "Transparency");

    auto transparency = new Container(this);
    transparency->layout(HFLOW(8));

    _transparency_none = new Button(transparency, Button::OUTLINE, "None");
    _transparency_slight = new Button(transparency, Button::OUTLINE, "Slight");
    _transparency_full = new Button(transparency, Button::OUTLINE, "Full");

    auto bind_transparency_button = [](Button *button, const char *mode_name) {
        button->on(Event::ACTION, [mode_name](auto) {
            settings::write(settings::Path::parse(TRANSPARENCY_SETTING_PATH), mode_name);
        });
    };

    bind_transparency_button(_transparency_none, "none");
    bind_transparency_button(_transparency_slight, "slight");
    bind_transparency_button(_transparency_full, "full");

    _transparency_setting = own<settings::Setting>(TRANSPARENCY_SETTING_PATH, [this](const json::Value &value) {
        update_transparency_buttons(value.as_string());
    });

    new Separator(this);

    /* --- Wallpaper ------------------------------------------------------------- */

    new Label(this, "Wallpaper");

    auto wallpapers = new Container(this);
    wallpapers->flags(Widget::FILL);
    wallpapers->layout(GRID(3, 4, 8, 8));

    for (auto &entry : WALLPAPERS)
    {
        auto button = new Button(wallpapers, Button::OUTLINE, entry.label);
        _wallpaper_buttons.push_back(button);
        _wallpaper_paths.push_back(entry.path);

        String path = entry.path;
        button->on(Event::ACTION, [path](auto) {
            settings::write(settings::Path::parse(WALLPAPER_SETTING_PATH), path.cstring());
        });
    }

    _wallpaper_setting = own<settings::Setting>(WALLPAPER_SETTING_PATH, [this](const json::Value &value) {
        update_wallpaper_buttons(value.is(json::STRING) ? value.as_string() : String(""));
    });
}

void AppearancePage::update_theme_buttons(const String &active_theme)
{
    _theme_dark->style(active_theme == "dark" ? Button::FILLED : Button::OUTLINE);
    _theme_light->style(active_theme == "light" ? Button::FILLED : Button::OUTLINE);
}

void AppearancePage::update_transparency_buttons(const String &active_mode)
{
    _transparency_none->style(active_mode == "none" ? Button::FILLED : Button::OUTLINE);
    _transparency_slight->style(active_mode == "slight" ? Button::FILLED : Button::OUTLINE);
    _transparency_full->style(active_mode == "full" ? Button::FILLED : Button::OUTLINE);
}

void AppearancePage::update_wallpaper_buttons(const String &active_wallpaper)
{
    for (size_t i = 0; i < _wallpaper_buttons.count(); i++)
    {
        _wallpaper_buttons[i]->style(_wallpaper_paths[i] == active_wallpaper ? Button::FILLED : Button::OUTLINE);
    }
}

} // namespace settings
