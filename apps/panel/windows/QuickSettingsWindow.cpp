#include <libsystem/process/Environment.h>
#include <libsystem/process/Process.h>

#include <libwidget/Container.h>
#include <libwidget/Label.h>
#include <libwidget/Panel.h>
#include <libwidget/Screen.h>
#include <libwidget/Separator.h>
#include <libwidget/Spacer.h>

#include "panel/windows/PanelWindow.h"
#include "panel/windows/QuickSettingsWindow.h"

namespace panel
{

static constexpr const char *WIREFRAME_SETTING_PATH = "appearance:widgets.wireframe";
static constexpr const char *THEME_SETTING_PATH = "appearance:widgets.theme";

QuickSettingsWindow::QuickSettingsWindow()
    : Window(WINDOW_AUTO_CLOSE | WINDOW_ALWAYS_FOCUSED | WINDOW_ACRYLIC)
{
    title("Panel");
    type(WINDOW_TYPE_POPOVER);
    opacity(0.85);

    on(Event::DISPLAY_SIZE_CHANGED, [this](auto) {
        layout_window();
    });

    root()->layout(VFLOW(8));
    root()->insets(Insetsi(8));

    new Label(root(), "Quick Settings");

    new Separator(root());

    /* --- Wireframe toggle -------------------------------------------------- */

    _wireframe_button = new Button(root(), Button::OUTLINE, "Toggle Wireframe");

    _wireframe_button->on(Event::ACTION, [](auto) {
        auto current = settings::read(settings::Path::parse(WIREFRAME_SETTING_PATH));
        bool enabled = current.present() && current.value().as_bool();

        settings::write(settings::Path::parse(WIREFRAME_SETTING_PATH), !enabled);
    });

    _wireframe_setting = own<settings::Setting>(WIREFRAME_SETTING_PATH, [this](const json::Value &value) {
        _wireframe_button->style(value.as_bool() ? Button::FILLED : Button::OUTLINE);
    });

    new Separator(root());

    /* --- Theme picker -------------------------------------------------------- */

    new Label(root(), "Theme");

    auto themes = new Container(root());
    themes->flags(Widget::FILL);
    themes->layout(HFLOW(6));

    _theme_dark = new Button(themes, Button::OUTLINE, "Dark");
    _theme_dark->flags(Widget::FILL);

    _theme_light = new Button(themes, Button::OUTLINE, "Light");
    _theme_light->flags(Widget::FILL);

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

    new Separator(root());

    /* --- Account row --------------------------------------------------------- */

    auto account_container = new Panel(root());
    account_container->layout(HFLOW(4));
    account_container->insets(Insetsi(6));

    new Button(account_container, Button::TEXT, Icon::get("account"), environment().get("POSIX").get("LOGNAME").as_string());

    new Spacer(account_container);

    auto folder_button = new Button(account_container, Button::TEXT, Icon::get("folder"));
    folder_button->on(Event::ACTION, [](auto) {
        process_run("file-manager", nullptr);
    });

    auto setting_button = new Button(account_container, Button::TEXT, Icon::get("cog"));
    setting_button->on(Event::ACTION, [](auto) {
        process_run("settings", nullptr);
    });

    auto logout_button = new Button(account_container, Button::TEXT, Icon::get("power-standby"));
    logout_button->on(Event::ACTION, [](auto) {
        process_run("logout", nullptr);
    });

    layout_window();
}

void QuickSettingsWindow::layout_window()
{
    bound(Screen::bound()
              .take_right(WIDTH)
              .shrinked({PanelWindow::HEIGHT, 0, 0, 0})
              .with_height(root()->compute_size().y()));
}

void QuickSettingsWindow::update_theme_buttons(const String &active_theme)
{
    _theme_dark->style(active_theme == "dark" ? Button::FILLED : Button::OUTLINE);
    _theme_light->style(active_theme == "light" ? Button::FILLED : Button::OUTLINE);
}

} // namespace panel
