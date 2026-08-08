#include <libgraphic/Framebuffer.h>
#include <libsystem/process/Process.h>

static const auto BACKGROUND = Colors::BLACK;
static const auto GLOW_COLOR = Color::from_hex(0xffb454);

static constexpr int BADGE_SIZE = 200;
static constexpr int HALO_SIZE = 280; // must comfortably fit the widest ring below

// The glow is drawn procedurally every frame as a stack of concentric
// filled circles (outer = large + near-transparent, inner = small +
// opaque), approximating a soft radial gradient without needing a
// gaussian blur or a pre-baked sprite sheet. No image asset for the halo
// at all -- only logo.png (the static badge) is loaded from disk.
static constexpr int RING_COUNT = 12;
static constexpr int RING_RADIUS[RING_COUNT] = {130, 120, 110, 99, 89, 79, 69, 59, 49, 38, 28, 18};
static constexpr int RING_ALPHA[RING_COUNT] = {0, 4, 12, 24, 38, 54, 72, 92, 114, 138, 163, 190};

// Precomputed breathing curve (smooth ease in/out, 0.45..1.0..0.45), so the
// glow's intensity animates smoothly without calling sin()/cos() at
// runtime -- no other userspace app in this codebase links libm, so this
// avoids relying on that being available at all.
static constexpr int BREATHE_FRAME_COUNT = 48;
static constexpr float BREATHE_CURVE[BREATHE_FRAME_COUNT] = {
    0.7250f, 0.7609f, 0.7962f, 0.8302f, 0.8625f, 0.8924f, 0.9195f, 0.9432f,
    0.9632f, 0.9791f, 0.9906f, 0.9976f, 1.0000f, 0.9976f, 0.9906f, 0.9791f,
    0.9632f, 0.9432f, 0.9195f, 0.8924f, 0.8625f, 0.8302f, 0.7962f, 0.7609f,
    0.7250f, 0.6891f, 0.6538f, 0.6198f, 0.5875f, 0.5576f, 0.5305f, 0.5068f,
    0.4868f, 0.4709f, 0.4594f, 0.4524f, 0.4500f, 0.4524f, 0.4594f, 0.4709f,
    0.4868f, 0.5068f, 0.5305f, 0.5576f, 0.5875f, 0.6198f, 0.6538f, 0.6891f};

// Only these two need tuning to change total splash duration and pacing --
// process_sleep() takes raw kernel ticks (~1ms nominally at 1000Hz), but
// actual wall-clock time depends on the host (QEMU timer emulation
// overhead, etc), so this is meant to be tuned by feel, not calculated.
static constexpr int TICKS_PER_FRAME = 1;
static constexpr int TOTAL_FRAMES = 96;

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

    auto halo_container = Recti(0, 0, HALO_SIZE, HALO_SIZE).centered_within(framebuffer->resolution());
    auto halo_center = halo_container.position() + Vec2i(HALO_SIZE / 2, HALO_SIZE / 2);

    auto badge_destination = Recti(
        halo_container.position().x() + (HALO_SIZE - BADGE_SIZE) / 2,
        halo_container.position().y() + (HALO_SIZE - BADGE_SIZE) / 2,
        BADGE_SIZE,
        BADGE_SIZE);

    auto &painter = framebuffer->painter();

    painter.clear(BACKGROUND);
    framebuffer->mark_dirty_all();
    framebuffer->blit();

    for (int i = 0; i < TOTAL_FRAMES; i++)
    {
        float intensity = BREATHE_CURVE[i % BREATHE_FRAME_COUNT];

        painter.clear(halo_container, BACKGROUND);

        for (int r = 0; r < RING_COUNT; r++)
        {
            int radius = RING_RADIUS[r];
            float alpha = (RING_ALPHA[r] * intensity) / 255.0f;

            Recti ring_bound(
                halo_center.x() - radius,
                halo_center.y() - radius,
                radius * 2,
                radius * 2);

            painter.fill_rectangle_rounded(ring_bound, radius, GLOW_COLOR.with_alpha(alpha));
        }

        painter.blit(*logo, logo->bound(), badge_destination);

        framebuffer->mark_dirty(halo_container);
        framebuffer->blit();

        process_sleep(TICKS_PER_FRAME);
    }

    return 0;
}
