#include <libgraphic/Framebuffer.h>
#include <libsystem/process/Process.h>

static const auto BACKGROUND = Colors::BLACK;

// The halo is a soft radial glow, bigger than the badge itself, so it can
// bloom out around it. Both sizes come from how halo-strip.png/logo.png
// were generated -- see toolchain notes if regenerating either.
static constexpr int HALO_SIZE = 400;
static constexpr int BADGE_SIZE = 340;
static constexpr int BADGE_OFFSET = (HALO_SIZE - BADGE_SIZE) / 2;
static constexpr int HALO_FRAME_COUNT = 24;

// process_sleep() takes raw kernel ticks (PIT is initialized at 1000Hz, so
// nominally ~1 tick/ms), not a fixed wall-clock unit -- and how long that
// actually takes in practice depends on the host (real hardware vs QEMU
// timer emulation overhead, etc). These two constants are the only things
// that need tuning to change total splash duration and perceived
// smoothness -- lower TICKS_PER_FRAME for smoother/faster, raise
// TOTAL_FRAMES for a longer wait without changing the pacing.
static constexpr int TICKS_PER_FRAME = 2;
static constexpr int TOTAL_FRAMES = 40;

int main(int argc, char **argv)
{
    __unused(argc);
    __unused(argv);

    auto framebuffer_or_result = Framebuffer::open();

    if (!framebuffer_or_result.success())
    {
        return -1;
    }

    auto framebuffer = framebuffer_or_result.take_value();

    auto logo = Bitmap::load_from_or_placeholder("/Applications/splash-screen/logo.png");
    auto halo_strip = Bitmap::load_from_or_placeholder("/Applications/splash-screen/halo-strip.png");

    auto halo_container = Recti(0, 0, HALO_SIZE, HALO_SIZE).centered_within(framebuffer->resolution());
    auto badge_destination = Recti(
        halo_container.position() + Vec2i(BADGE_OFFSET, BADGE_OFFSET),
        Vec2i(BADGE_SIZE, BADGE_SIZE));

    auto &painter = framebuffer->painter();

    painter.clear(BACKGROUND);
    framebuffer->mark_dirty_all();
    framebuffer->blit();

    for (int i = 0; i < TOTAL_FRAMES; i++)
    {
        int halo_frame = i % HALO_FRAME_COUNT;
        Recti halo_source(halo_frame * HALO_SIZE, 0, HALO_SIZE, HALO_SIZE);

        painter.clear(halo_container, BACKGROUND);
        painter.blit(*halo_strip, halo_source, halo_container);
        painter.blit(*logo, logo->bound(), badge_destination);

        framebuffer->mark_dirty(halo_container);
        framebuffer->blit();

        process_sleep(TICKS_PER_FRAME);
    }

    return 0;
}
