#include <karm/entry>

#include "doom.h"

import Karm.Core;
import Karm.App;
import Karm.Gfx;
import Karm.Sys;
import Karm.Math;
import Karm.Logger;

using namespace Karm;
using namespace Karm::Literals;

static constexpr Math::Vec2i SIZE = {320, 200};
static constexpr bool DEBUG_DOOM = false;

struct Handler : App::Handler {
    Rc<App::Window> win;

    Handler(Rc<App::Window> win) : win(win) {}

    void update() override {
        doom_update();

        auto* fb = doom_get_framebuffer(4);
        auto surface = Gfx::Surface::wrap({
            const_cast<u8*>(fb), SIZE.x * SIZE.y * 4uz}, SIZE, SIZE.x * 4, Gfx::RGBA8888);
        auto s = win->acquireSurface();
        s.clear(Gfx::BLACK);
        Gfx::CpuCanvas g;
        g.begin(s);
        g.blit(surface->bound(), surface->bound().fit(s.bound()), surface);
        win->releaseSurface(s.bound());
    }

    static doom_key_t toDoom(App::Key l) {
        switch (l.code()) {
        case App::Key::TAB:
            return DOOM_KEY_TAB;
        case App::Key::ENTER:
            return DOOM_KEY_ENTER;
        case App::Key::ESC:
            return DOOM_KEY_ESCAPE;
        case App::Key::SPACE:
            return DOOM_KEY_SPACE;
        case App::Key::NUM0:
            return DOOM_KEY_0;
        case App::Key::NUM1:
            return DOOM_KEY_1;
        case App::Key::NUM2:
            return DOOM_KEY_2;
        case App::Key::NUM3:
            return DOOM_KEY_3;
        case App::Key::NUM4:
            return DOOM_KEY_4;
        case App::Key::NUM5:
            return DOOM_KEY_5;
        case App::Key::NUM6:
            return DOOM_KEY_6;
        case App::Key::NUM7:
            return DOOM_KEY_7;
        case App::Key::NUM8:
            return DOOM_KEY_8;
        case App::Key::NUM9:
            return DOOM_KEY_9;
        case App::Key::A:
            return DOOM_KEY_A;
        case App::Key::B:
            return DOOM_KEY_B;
        case App::Key::C:
            return DOOM_KEY_C;
        case App::Key::D:
            return DOOM_KEY_D;
        case App::Key::E:
            return DOOM_KEY_E;
        case App::Key::F:
            return DOOM_KEY_F;
        case App::Key::G:
            return DOOM_KEY_G;
        case App::Key::H:
            return DOOM_KEY_H;
        case App::Key::I:
            return DOOM_KEY_I;
        case App::Key::J:
            return DOOM_KEY_J;
        case App::Key::K:
            return DOOM_KEY_K;
        case App::Key::L:
            return DOOM_KEY_L;
        case App::Key::M:
            return DOOM_KEY_M;
        case App::Key::N:
            return DOOM_KEY_N;
        case App::Key::O:
            return DOOM_KEY_O;
        case App::Key::P:
            return DOOM_KEY_P;
        case App::Key::Q:
            return DOOM_KEY_Q;
        case App::Key::R:
            return DOOM_KEY_R;
        case App::Key::S:
            return DOOM_KEY_S;
        case App::Key::T:
            return DOOM_KEY_T;
        case App::Key::U:
            return DOOM_KEY_U;
        case App::Key::V:
            return DOOM_KEY_V;
        case App::Key::W:
            return DOOM_KEY_W;
        case App::Key::X:
            return DOOM_KEY_X;
        case App::Key::Y:
            return DOOM_KEY_Y;
        case App::Key::Z:
            return DOOM_KEY_Z;
        case App::Key::BKSPC:
            return DOOM_KEY_BACKSPACE;
        case App::Key::LCTRL:
            return DOOM_KEY_CTRL;
        case App::Key::RCTRL:
            return DOOM_KEY_CTRL;
        case App::Key::LEFT:
            return DOOM_KEY_LEFT_ARROW;
        case App::Key::UP:
            return DOOM_KEY_UP_ARROW;
        case App::Key::RIGHT:
            return DOOM_KEY_RIGHT_ARROW;
        case App::Key::DOWN:
            return DOOM_KEY_DOWN_ARROW;
        case App::Key::LSHIFT:
            return DOOM_KEY_SHIFT;
        case App::Key::RSHIFT:
            return DOOM_KEY_SHIFT;
        case App::Key::LALT:
            return DOOM_KEY_ALT;
        case App::Key::RALT:
            return DOOM_KEY_ALT;
        case App::Key::F1:
            return DOOM_KEY_F1;
        case App::Key::F2:
            return DOOM_KEY_F2;
        case App::Key::F3:
            return DOOM_KEY_F3;
        case App::Key::F4:
            return DOOM_KEY_F4;
        case App::Key::F5:
            return DOOM_KEY_F5;
        case App::Key::F6:
            return DOOM_KEY_F6;
        case App::Key::F7:
            return DOOM_KEY_F7;
        case App::Key::F8:
            return DOOM_KEY_F8;
        case App::Key::F9:
            return DOOM_KEY_F9;
        case App::Key::F10:
            return DOOM_KEY_F10;
        case App::Key::F11:
            return DOOM_KEY_F11;
        case App::Key::F12:
            return DOOM_KEY_F12;
        default:
            break;
        }
        return DOOM_KEY_UNKNOWN;
    }

    static doom_button_t toDoom(App::MouseButton btn) {
        switch (btn) {
        case App::MouseButton::LEFT:
            return DOOM_LEFT_BUTTON;
        case App::MouseButton::MIDDLE:
            return DOOM_MIDDLE_BUTTON;
        case App::MouseButton::RIGHT:
            return DOOM_RIGHT_BUTTON;
        default:
            return (doom_button_t)-1;
        }
    }

    void handle(App::WindowId, App::Event& e) override {
        if (auto ke = e.is<App::KeyboardEvent>()) {
            if (ke->type == App::KeyboardEvent::PRESS)
                doom_key_down(toDoom(ke->key));

            if (ke->type == App::KeyboardEvent::RELEASE)
                doom_key_up(toDoom(ke->key));
        } else if (auto me = e.is<App::MouseEvent>()) {
            if (me->type == App::MouseEvent::MOVE) {
                doom_mouse_move(me->delta.x * 10, me->delta.y * 10);
            } else if (me->type == App::MouseEvent::PRESS) {
                doom_button_down(toDoom(me->button));
            } else if (me->type == App::MouseEvent::RELEASE) {
                doom_button_up(toDoom(me->button));
            }
        }
        e.accept();
    }
};

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto app = co_trya$(App::Application::createAsync(env, App::ApplicationProps::simple(), ct));
    auto win = co_trya$(
        app->createWindowAsync(
            {
                .title = "DOOM"s,
                .size = SIZE * 3,
            },
            ct
        )
    );

    doom_set_print([](char const* cstr) {
        Str msg = Str::fromNullterminated(cstr);
        logDebugIf(DEBUG_DOOM, "doom: {}", msg);
    });

    doom_set_malloc(
        [](int size) -> void* {
            return new u8[size];
        },
        [](void* ptr) {
            delete[] (u8*)ptr;
        }
    );

    doom_set_file_io(
        // open
        [](char const* filename, char const*) -> void* {
            Str path = Str::fromNullterminated(filename);
            auto maybeFile = Sys::File::open(Ref::parseUrlOrPath(path, Sys::globalEnv().cwd()));
            if (not maybeFile)
                return nullptr;
            return new Sys::File(maybeFile.take());
        },
        // close
        [](void* handle) {
            if (not handle)
                return;
            delete static_cast<Sys::File*>(handle);
        },
        // read
        [](void* handle, void* buf, int count) -> int {
            if (not handle)
                return -1;

            return static_cast<Sys::File*>(handle)
                ->read({(u8*)buf, (usize)count})
                .unwrapOr(-1);
        },
        // write
        [](void* handle, void const* buf, int count) -> int {
            return static_cast<Sys::File*>(handle)
                ->write({(u8 const*)buf, (usize)count})
                .unwrapOr(-1);
        },
        // seek
        [](void* handle, int offset, doom_seek_t origin) -> int {
            if (not handle)
                return -1;

            Io::Whence whence;
            switch (origin) {
            case DOOM_SEEK_CUR:
                whence = Io::Whence::CURRENT;
                break;
            case DOOM_SEEK_END:
                whence = Io::Whence::END;
                break;
            case DOOM_SEEK_SET:
                whence = Io::Whence::BEGIN;
                break;
            }

            auto result = static_cast<Sys::File*>(handle)
                ->seek({whence, offset});

            if (not result)
                return -1;

            return 0;
        },
        // tell
        [](void* handle) -> int {
            return static_cast<Sys::File*>(handle)
                ->seek(Io::Seek::fromCurrent(0))
                .unwrapOr(-1);
        },
        // eof,
        [](void* handle) -> int {
            auto* file = static_cast<Sys::File*>(handle);

            auto curr = file->seek(Io::Seek::fromCurrent(0)).unwrapOr(0);
            auto end = file->seek(Io::Seek::fromEnd(0)).unwrapOr(0);
            (void)file->seek(Io::Seek::fromBegin(curr));

            return curr == end;
        }
    );

    doom_set_gettime([](int* sec, int* usec) {
        auto time = Sys::instant() - Instant::epoch();
        if (sec)
            *sec = time.toSecs();
        if (usec)
            *usec = (time - Duration::fromSecs(time.toSecs())).toUSecs();
    });

    doom_set_exit([](int code) {
        logDebugIf(DEBUG_DOOM, "exiting {}", code);
        Sys::exit(Ok());
    });

    doom_set_getenv([](char const* cstr) -> char* {
        Str key = Str::fromNullterminated(cstr);
        if (key == "HOME")
            return (char*)"bundle://hideo-doom";
        if (key == "DOOMWADDIR")
            return (char*)"bundle://hideo-doom";

        return (char*)"";
    });

    char* argv[] = {(char*)"doom", nullptr};
    doom_init(1, argv, 0);
    auto handler = makeRc<Handler>(win);
    co_return co_await app->runAsync(handler, ct);
}
