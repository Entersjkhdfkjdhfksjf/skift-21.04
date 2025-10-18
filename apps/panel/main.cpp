#include <libwidget/Application.h>

#include "panel/windows/PanelWindow.h"

static constexpr int PANEL_HEIGHT = 38;

int main(int argc, char **argv)
{
    Application::initialize(argc, argv);

    auto window = own<panel::PanelWindow>();
    window->show();

    return Application::run();
}
