#pragma once

#include <libutils/String.h>
#include <libutils/Vector.h>
#include <libwidget/Button.h>
#include <libwidget/Container.h>
#include <libwidget/Window.h>

namespace settings
{

class MainWindow : public Window
{
private:
    Container *_content = nullptr;

    Button *_back_button = nullptr;
    Button *_forward_button = nullptr;

    Vector<String> _history{};
    int _history_index = -1;

public:
    MainWindow();

    void navigate(String page);

private:
    void show_page(String page);
};

} // namespace settings
