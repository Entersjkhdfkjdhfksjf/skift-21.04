#pragma once

#include <libwidget/Container.h>

namespace settings
{

class MainWindow;

class HomePage : public Container
{
public:
    HomePage(Widget *parent, MainWindow *window);
};

} // namespace settings
