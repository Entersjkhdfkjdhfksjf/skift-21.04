#include <libgraphic/Painter.h>
#include <libwidget/Image.h>

Image::Image(Widget *parent, RefPtr<Bitmap> bitmap)
    : Widget(parent), _bitmap(bitmap)
{
}

void Image::change_bitmap(RefPtr<Bitmap> bitmap)
{
    if (_bitmap != bitmap)
    {
        _bitmap = bitmap;
        should_repaint();
    }
}

void Image::change_scaling(ImageScalling scaling)
{
    if (_scalling != scaling)
    {
        _scalling = scaling;
        should_repaint();
    }
}

void Image::paint(Painter &painter, const Recti &)
{
    if (!_bitmap)
    {
        return;
    }

    Recti destination = bound();
    bool needs_scaling = false;

    if (_scalling == ImageScalling::CENTER)
    {
        destination = _bitmap->bound().centered_within(bound());
    }
    else if (_scalling == ImageScalling::STRETCH)
    {
        destination = bound();
        needs_scaling = true;
    }
    else if (_scalling == ImageScalling::FIT)
    {
        // Scale-to-contain: fit the whole image inside our bound while
        // preserving aspect ratio, then center the result. This is what
        // CENTER should probably have been doing all along for anything
        // that isn't guaranteed to already be the right pixel size (like
        // the About window's logo, which broke the moment someone dropped
        // in a larger image — CENTER doesn't scale at all, it just clips).
        Vec2i bitmap_size = _bitmap->bound().size();
        Vec2i widget_size = bound().size();

        if (bitmap_size.x() > 0 && bitmap_size.y() > 0)
        {
            double scale_x = (double)widget_size.x() / (double)bitmap_size.x();
            double scale_y = (double)widget_size.y() / (double)bitmap_size.y();
            double scale = scale_x < scale_y ? scale_x : scale_y;

            Vec2i scaled_size = {
                (int)(bitmap_size.x() * scale),
                (int)(bitmap_size.y() * scale),
            };

            destination = Recti(scaled_size).centered_within(bound());
        }

        needs_scaling = true;
    }

    if (needs_scaling)
    {
        painter.blit_scaled(*_bitmap, _bitmap->bound(), destination);
    }
    else
    {
        painter.blit(*_bitmap, _bitmap->bound(), destination);
    }
}

Vec2i Image::size()
{
    if (_bitmap)
    {
        return _bitmap->bound().size();
    }
    else
    {
        return bound().size();
    }
}
