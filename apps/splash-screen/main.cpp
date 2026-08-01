#include <libgraphic/Framebuffer.h>
#include <libsystem/process/Process.h>

static const auto BACKGROUND = Color::from_hex(0x18181B);
static const auto PROGRESS = Color::from_hex(0x0066ff);
static const auto REMAINING = Color::from_hex(0x444444);

// Position (within logo.png) and framing of the arrow glyph's animated
// glow overlay. These are derived from how glow-strip.png was generated
// from the source artwork -- if the artwork or its scale ever changes,
// these need regenerating alongside it, they aren't computed at runtime.
static constexpr int GLOW_OFFSET_X = 51;
static constexpr int GLOW_OFFSET_Y = 297;
static constexpr int GLOW_FRAME_WIDTH = 48;
static constexpr int GLOW_FRAME_HEIGHT = 34;
static constexpr int GLOW_FRAME_COUNT = 28;

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
    auto cat = Bitmap::load_from_or_placeholder("/Applications/splash-screen/cat.png");
    auto glow_strip = Bitmap::load_from_or_placeholder("/Applications/splash-screen/glow-strip.png");

    auto logo_container = logo->bound().centered_within(framebuffer->resolution());

    auto glow_position = logo_container.position() + Vec2i(GLOW_OFFSET_X, GLOW_OFFSET_Y);
    auto glow_destination = Recti(glow_position, Vec2i(GLOW_FRAME_WIDTH, GLOW_FRAME_HEIGHT));

    auto loading_container = Recti(0, 0, logo_container.width() * 1.4, 4)
                                 .centered_within(framebuffer->resolution())
                                 .offset(Vec2i(0, logo_container.height() + 26));

    auto &painter = framebuffer->painter();

    painter.clear(BACKGROUND);

    painter.blit(*logo, logo->bound(), logo_container);

    framebuffer->mark_dirty_all();
    framebuffer->blit();

    for (size_t i = 0; i <= 100; i++)
    {
        // Sweep the arrow's glow from tip to tail on a repeating cycle,
        // riding along with the existing progress loop rather than
        // needing a separate timer/animation loop of its own.
        int glow_frame = i % GLOW_FRAME_COUNT;
        Recti glow_source(glow_frame * GLOW_FRAME_WIDTH, 0, GLOW_FRAME_WIDTH, GLOW_FRAME_HEIGHT);

        painter.clear(glow_destination, BACKGROUND); // erase the previous frame's glow first
        painter.blit(*glow_strip, glow_source, glow_destination);

        painter.clear(loading_container, REMAINING);

        Recti progress = loading_container.take_left(loading_container.width() * (i / 100.0));

        if (argc == 2 && strcmp(argv[1], "--nyan") == 0)
        {
            auto color = Color::from_hsv((int)(360 * (i / 100.0) * 2) % 360, 0.5, 1);

            painter.clear(progress, color);
            painter.blit(*cat, cat->bound(), cat->bound().moved(progress.top_right() + Vec2i(-4, -2 - 8)));
        }
        else
        {
            painter.clear(progress, PROGRESS);
            painter.fill_rectangle(progress.take_right(1), REMAINING);
        }

        framebuffer->mark_dirty(glow_destination);
        framebuffer->mark_dirty(loading_container.expended(Insetsi(16)));
        framebuffer->blit();

        process_sleep(5);
    }

    return 0;
}
