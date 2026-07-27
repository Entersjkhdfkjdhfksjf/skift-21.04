#include <libgraphic/Painter.h>

#include <libwidget/Button.h>
#include <libwidget/IconPanel.h>
#include <libwidget/Label.h>
#include <libwidget/Theme.h>

// Filled buttons draw the accent color as their background and used to
// always draw white text/icon on top of it. That's unreadable on themes
// with a light/bright accent (e.g. the Ayu-derived Dark theme's amber
// #E6B450 accent) since white-on-amber has very low contrast. Instead we
// pick whichever of black/white actually contrasts with the accent color,
// using perceived luminance (ITU-R BT.601) rather than a hardcoded color.
static Color accent_contrast_color(Color accent)
{
    double luminance = (0.299 * accent.red() + 0.587 * accent.green() + 0.114 * accent.blue()) / 255.0;
    return luminance > 0.6 ? Colors::BLACK : Colors::WHITE;
}

void Button::paint(Painter &painter, const Recti &rectangle)
{
    __unused(rectangle);

    if (enabled())
    {
        if (_style == OUTLINE)
        {
            painter.draw_rectangle_rounded(bound(), 4, 1, color(THEME_BORDER));
        }
        else if (_style == FILLED)
        {
            painter.fill_rectangle_rounded(bound(), 4, color(THEME_ACCENT));
        }

        if (_mouse_over)
        {
            painter.fill_rectangle_rounded(bound(), 4, color(THEME_FOREGROUND).with_alpha(0.1));
        }

        if (_mouse_press)
        {
            painter.fill_rectangle_rounded(bound(), 4, color(THEME_FOREGROUND).with_alpha(0.1));
        }
    }
}

void Button::event(Event *event)
{
    if (event->type == Event::MOUSE_ENTER)
    {
        _mouse_over = true;

        should_repaint();
        event->accepted = true;
    }
    else if (event->type == Event::MOUSE_LEAVE)
    {
        _mouse_over = false;

        should_repaint();
        event->accepted = true;
    }
    else if (event->type == Event::MOUSE_BUTTON_PRESS)
    {
        _mouse_press = true;

        should_repaint();
        event->accepted = true;
    }
    else if (event->type == Event::MOUSE_BUTTON_RELEASE)
    {
        _mouse_press = false;

        Event action_event = {};
        action_event.type = Event::ACTION;
        dispatch_event(&action_event);

        should_repaint();
        event->accepted = true;
    }
    else if (event->type == Event::WIDGET_DISABLE)
    {
        _mouse_over = false;
        _mouse_press = false;
    }
}

Button::Button(Widget *parent, Style style)
    : Widget(parent),
      _style(style)
{
    layout(HFLOW(0));
    insets(Insetsi(0, 16));
    min_height(32);
    flags(Widget::GREEDY);
}

Button::Button(Widget *parent, Style style, RefPtr<Icon> icon)
    : Button(parent, style)
{
    layout(STACK());
    insets(Insetsi(6));
    min_width(32);
    flags(Widget::GREEDY | Widget::SQUARE);

    auto icon_panel = new IconPanel(this, icon);

    if (style == FILLED)
    {
        icon_panel->color(THEME_FOREGROUND, accent_contrast_color(theme_get_color(THEME_ACCENT)));
    }
}

Button::Button(Widget *parent, Style style, String text)
    : Button(parent, style)
{
    layout(STACK());
    insets(Insetsi(0, 0, 6, 6));
    min_width(64);

    auto label = new Label(this, text, Anchor::CENTER);
    if (style == FILLED)
    {
        label->color(THEME_FOREGROUND, accent_contrast_color(theme_get_color(THEME_ACCENT)));
    }
}

Button::Button(Widget *parent, Style style, RefPtr<Icon> icon, String text)
    : Button(parent, style)
{
    insets(Insetsi(0, 0, 6, 10));
    min_width(64);

    auto icon_panel = new IconPanel(this, icon);
    icon_panel->insets(Insetsi(0, 0, 0, 4));

    auto label = new Label(this, text);

    if (style == FILLED)
    {
        label->color(THEME_FOREGROUND, accent_contrast_color(theme_get_color(THEME_ACCENT)));
        icon_panel->color(THEME_FOREGROUND, accent_contrast_color(theme_get_color(THEME_ACCENT)));
    }
}
