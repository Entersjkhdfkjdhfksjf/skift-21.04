#pragma once

#include <libsystem/eventloop/Timer.h>
#include <libutils/Vector.h>
#include <libwidget/Button.h>
#include <libwidget/Label.h>
#include <libwidget/Window.h>

namespace panel
{

// Mirrors the shape a real snapshot primitive (kernel/system/Snapshot.h,
// not yet written) is expected to expose: an id to restore by, when it was
// taken, and a rough size so a retention policy can be memory-aware (see
// the note on Stage 5 in the snapshot roadmap -- each retained snapshot
// pins whatever physical pages diverged from it). No producer of this
// struct exists yet; this is here so the UI's shape and the primitive's
// eventual return shape were designed together, not the UI guessing.
struct SnapshotInfo
{
    int id;
    TimeStamp taken_at;
    size_t pages_captured;
};

class DateAndTimeWindow : public Window
{
private:
    OwnPtr<Timer> _clock_timer;

    Label *_time_label = nullptr;
    Label *_date_label = nullptr;

    Vector<SnapshotInfo> _snapshots{}; // always empty for now -- see .cpp

public:
    DateAndTimeWindow();

private:
    void build_snapshot_section(Widget *parent);
};

} // namespace panel
