#include <libwidget/Button.h>
#include <libwidget/Panel.h>
#include <libwidget/TitleBar.h>

#include "settings/pages/Appearance.h"
#include "settings/pages/Home.h"
#include "settings/windows/MainWindow.h"

namespace settings
{

MainWindow::MainWindow() : Window(WINDOW_RESIZABLE)
{
    title("Settings");
    icon(Icon::get("cog"));
    size({700, 500});

    root()->layout(VFLOW(0));

    new TitleBar(root());

    auto navigation_bar = new Panel(root());
    navigation_bar->layout(HFLOW(4));
    navigation_bar->insets(4);
    navigation_bar->max_height(38);
    navigation_bar->min_height(38);

    _back_button = new Button(navigation_bar, Button::TEXT, Icon::get("arrow-left"));
    _forward_button = new Button(navigation_bar, Button::TEXT, Icon::get("arrow-right"));
    auto home_button = new Button(navigation_bar, Button::TEXT, Icon::get("home"));

    _back_button->on(Event::ACTION, [this](auto) {
        if (_history_index > 0)
        {
            _history_index--;
            show_page(_history[_history_index]);
        }
    });

    _forward_button->on(Event::ACTION, [this](auto) {
        if (_history_index < (int)_history.count() - 1)
        {
            _history_index++;
            show_page(_history[_history_index]);
        }
    });

    home_button->on(Event::ACTION, [this](auto) {
        navigate("home");
    });

    _content = new Container(root());
    _content->layout(STACK());
    _content->flags(Widget::FILL);

    navigate("home");
}

void MainWindow::navigate(String page)
{
    // Discard any forward history once the user branches off in a new
    // direction, same as a normal browser back/forward stack.
    while ((int)_history.count() > _history_index + 1)
    {
        _history.pop_back();
    }

    _history.push_back(page);
    _history_index = _history.count() - 1;

    show_page(page);
}

void MainWindow::show_page(String page)
{
    _content->clear_children();

    if (page == "appearance")
    {
        new AppearancePage(_content, this);
    }
    else
    {
        new HomePage(_content, this);
    }
}

} // namespace settings
