#include <libsystem/process/Process.h>

#include "settings/pages/Home.h"
#include "settings/widgets/Link.h"
#include "settings/windows/MainWindow.h"

namespace settings
{

HomePage::HomePage(Widget *parent, MainWindow *window)
    : Container(parent)
{
    layout(STACK());
    flags(Widget::FILL);

    auto links = new Container(this);
    links->layout(GRID(4, 4, 8, 8));
    links->insets(16);

    auto appearance_link = new Link(links, Icon::get("palette"), "Appearance");
    appearance_link->on(Event::ACTION, [window](auto) {
        window->navigate("appearance");
    });

    auto about_link = new Link(links, Icon::get("information"), "About");
    about_link->on(Event::ACTION, [](auto) {
        process_run("about", nullptr);
    });
}

} // namespace settings
