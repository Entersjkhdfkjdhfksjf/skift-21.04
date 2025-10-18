#include <libgraphic/Painter.h>

#include <libwidget/Button.h>
#include <libwidget/Spacer.h>
#include <libwidget/TitleBar.h>
#include <libwidget/Window.h>

TitleBar::TitleBar(Widget *parent) : Widget(parent)
{
    max_height(36);

    layout(HFLOW(4));
    insets({6, 6});

    new Button(
        this,
        Button::TEXT,
        window()->icon(),
        window()->title());

    new Spacer(this);

    if (window()->flags() & WINDOW_RESIZABLE)
    {
        Widget *button_minimize = new Button(this, Button::TEXT, Icon::get("window-minimize"));
        button_minimize->insets(3);

        Widget *button_maximize = new Button(this, Button::TEXT, Icon::get("window-maximize"));
        button_maximize->insets(3);
        button_maximize->on(Event::ACTION, [this](auto) {
            window()->toggle_maximise();
        });
    }

    Widget *close_button = new Button(this, Button::TEXT, Icon::get("window-close"));
    close_button->insets(3);

    close_button->on(Event::ACTION, [this](auto) {
        window()->hide();
    });
}

TitleBar::~TitleBar()
{
}

void TitleBar::event(Event *event)
{
    __unused(event);

    if (is_mouse_event(event))
    {
        if (window()->maximised())
        {
            return;
        }

        if (!_is_dragging &&
            event->type == Event::MOUSE_BUTTON_PRESS &&
            event->mouse.button == MOUSE_BUTTON_LEFT)
        {
            _is_dragging = true;
            cursor(CURSOR_MOVE);
            event->accepted = true;
        }
        else if (
            _is_dragging &&
            event->type == Event::MOUSE_BUTTON_RELEASE &&
            event->mouse.button == MOUSE_BUTTON_LEFT)
        {
            _is_dragging = false;
            cursor(CURSOR_DEFAULT);
            event->accepted = true;
        }
        else if (
            _is_dragging &&
            event->type == Event::MOUSE_MOVE)
        {
            Vec2i offset = event->mouse.position_on_screen - event->mouse.old_position_on_screen;
            window()->bound(window()->bound_on_screen().offset(offset));
            event->accepted = true;
        }
    }
}