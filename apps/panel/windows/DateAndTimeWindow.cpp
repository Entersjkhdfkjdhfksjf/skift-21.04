#include <libwidget/Container.h>
#include <libwidget/Panel.h>
#include <libwidget/Screen.h>
#include <libwidget/Separator.h>

#include "panel/windows/DateAndTimeWindow.h"
#include "panel/windows/PanelWindow.h"

namespace panel
{

DateAndTimeWindow::DateAndTimeWindow()
    : Window(WINDOW_ALWAYS_FOCUSED | WINDOW_AUTO_CLOSE | WINDOW_ACRYLIC)
{
    title("Date And Time");
    type(WINDOW_TYPE_POPOVER);
    opacity(0.85);

    on(Event::DISPLAY_SIZE_CHANGED, [this](auto) {
        bound(Rect{320, root()->compute_size().y()}.centered_within(Screen::bound()).with_y(PanelWindow::HEIGHT));
    });

    root()->layout(VFLOW(8));
    root()->insets(Insetsi(12));

    /* --- Clock ------------------------------------------------------------- */

    _time_label = new Label(root(), "", Anchor::CENTER);
    _date_label = new Label(root(), "", Anchor::CENTER);

    _clock_timer = own<Timer>(1000, [this]() {
        TimeStamp timestamp = timestamp_now();
        DateTime datetime = timestamp_to_datetime(timestamp);

        char time_buffer[16];
        snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d:%02d", datetime.hour, datetime.minute, datetime.second);
        _time_label->text(time_buffer);

        char date_buffer[32];
        snprintf(date_buffer, sizeof(date_buffer), "%04d-%02d-%02d", datetime.year, datetime.month, datetime.day);
        _date_label->text(date_buffer);
    });

    _clock_timer->start();
    _clock_timer->trigger(); // show the correct time immediately, not one second late

    new Separator(root());

    /* --- Snapshots ("Time Travel") ------------------------------------------ */

    build_snapshot_section(root());

    bound(Rect{320, root()->compute_size().y()}.centered_within(Screen::bound()).with_y(PanelWindow::HEIGHT));
}

void DateAndTimeWindow::build_snapshot_section(Widget *parent)
{
    new Label(parent, "Time Travel");

    // Deliberately not wired to anything real yet -- there is no snapshot
    // primitive in the kernel to call into. Label says so outright rather
    // than pretending to work. See SnapshotInfo above for the shape a real
    // implementation is expected to produce.
    auto take_snapshot_button = new Button(parent, Button::OUTLINE, "Take Snapshot (coming soon)");
    take_snapshot_button->on(Event::ACTION, [](auto) {
        // Intentionally a no-op for now.
    });

    auto snapshot_list = new Panel(parent);
    snapshot_list->layout(VFLOW(4));
    snapshot_list->insets(Insetsi(8));

    if (_snapshots.count() == 0)
    {
        new Label(snapshot_list, "No snapshots yet.", Anchor::CENTER);
    }
    else
    {
        // Not reachable today -- _snapshots is never populated -- but this
        // is what listing real entries will look like once Take Snapshot
        // actually produces a SnapshotInfo: a timestamped row with its own
        // restore button, one per entry.
        for (size_t i = 0; i < _snapshots.count(); i++)
        {
            auto &snapshot = _snapshots[i];

            auto row = new Container(snapshot_list);
            row->layout(HFLOW(8));

            DateTime taken_at = timestamp_to_datetime(snapshot.taken_at);
            char label_buffer[32];
            snprintf(label_buffer, sizeof(label_buffer), "%02d:%02d:%02d", taken_at.hour, taken_at.minute, taken_at.second);

            new Label(row, label_buffer);
            row->flags(Widget::FILL);

            auto restore_button = new Button(row, Button::TEXT, "Restore");
            restore_button->on(Event::ACTION, [id = snapshot.id](auto) {
                __unused(id);
                // Intentionally a no-op for now.
            });
        }
    }
}

} // namespace panel
