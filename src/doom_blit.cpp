// chili-doom: glue between the Doom engine (rp2040-doom and ThumbyDOOM's
// port/ layer, from the vendor/ThumbyDOOM submodule) and the 32blit SDK.
//
// The engine was written for a bare-metal loop that never returns. Here the
// SDK owns the loop instead: init() runs D_DoomMain() up to the point where
// the game loop would start (see CHILI_BLIT in d_main.c), update() runs one
// Doom frame per 35 Hz tic, and render() copies the finished 128x128 RGB565
// frame onto the screen.
#include <algorithm>
#include <cstring>

#include "32blit.hpp"
#if PICO_ON_DEVICE
#include "engine/api_private.hpp" // blit::api.set_framebuffer
#endif

using namespace blit;

extern "C" {

// The engine draws every finished frame into this buffer (ThumbyDOOM port/i_video_thumby.c).
// On the console it is also the screen itself (see init()).
alignas(4) uint16_t g_fb[128 * 128];

int boot_active = 0;

void D_DoomMain(void);
void D_RunFrame(void);

extern const uint8_t doom1_whd_data[];
extern const uint32_t doom1_whd_data_size;

// --- time ----------------------------------------------------------------

// On the console the pico SDK already provides time_us_64(). Elsewhere,
// build it from blit::now_us(), which wraps every ~71 minutes.
uint64_t time_us_64(void);
#if !PICO_ON_DEVICE
uint64_t time_us_64(void) {
    static uint32_t last;
    static uint64_t high;
    uint32_t now = blit::now_us();
    if (now < last)
        high += 1ull << 32;
    last = now;
    return high | now;
}
#endif

void chili_sleep_us(uint64_t us) {
    uint64_t end = time_us_64() + us;
    while (time_us_64() < end) {
    }
}

// --- display -------------------------------------------------------------

static bool frame_ready = false;

void doom_lcd_present(const uint16_t *) { frame_ready = true; }
void doom_lcd_wait_idle(void) {}
void doom_lcd_backlight(int) {}

// --- buttons -------------------------------------------------------------

// Pin numbers the port's input code polls (ThumbyDOOM port/i_input_thumby.c), mapped to
// the VGC Zero buttons. Returns 0 when held (the port expects active-low pins).
int host_gpio_get(int pin) {
    uint32_t held = 0;
    switch (pin) {
    case 0:  held = buttons & Button::DPAD_LEFT;  break;
    case 1:  held = buttons & Button::DPAD_UP;    break;
    case 2:  held = buttons & Button::DPAD_RIGHT; break;
    case 3:  held = buttons & Button::DPAD_DOWN;  break;
    case 6:  held = buttons & Button::Y;          break; // strafe left
    case 22: held = buttons & Button::X;          break; // strafe right
    case 21: held = buttons & Button::A;          break; // fire / confirm
    case 25: held = buttons & Button::B;          break; // use / yes
    case 26: held = buttons & Button::MENU;       break; // Doom menu
    }
    return held ? 0 : 1;
}

void doom_buttons_init(void) {}
int doom_buttons_menu_pressed(void) { return host_gpio_get(26) == 0; }

// --- audio (not wired up yet) --------------------------------------------

void doom_audio_pwm_init(void) {}
void doom_audio_pwm_push(const int16_t *, int) {}
int doom_audio_pwm_room(void) { return 4096; }

} // extern "C"

// A build without game data embeds a placeholder instead of a WHD blob
// (see CMakeLists.txt), so it still compiles and starts.
static bool have_wad() {
    return doom1_whd_data_size >= 4 && doom1_whd_data[0] == 'I' && doom1_whd_data[1] == 'W' &&
           doom1_whd_data[2] == 'H' && doom1_whd_data[3] == 'D';
}

void init() {
#if PICO_ON_DEVICE
    // RAM is tight on the console, so instead of copying each frame into the
    // SDK's framebuffer, hand it Doom's: the screen shows g_fb directly
    // (single-buffered). CMakeLists.txt shrinks the SDK's own buffer to match.
    blit::api.set_framebuffer(reinterpret_cast<uint8_t *>(g_fb), sizeof(g_fb), Size(128, 128));
#endif
    set_screen_mode(ScreenMode::hires);

    if (have_wad())
        D_DoomMain();
}

void update(uint32_t) {
    if (!have_wad())
        return;

    // Doom runs at 35 tics a second. With CHILI_BLIT, D_RunFrame() returns
    // straight away when no tic is due instead of waiting for one, so the
    // SDK keeps reading input and drawing while Doom catches up.
    D_RunFrame();
}

void render(uint32_t) {
    if (!have_wad()) {
        screen.pen = Pen(0, 0, 0);
        screen.clear();
        screen.pen = Pen(255, 255, 255);
        screen.text("No WAD built in.\nSee README.", minimal_font, Point(4, 4));
        return;
    }

    // On the console the screen already is g_fb.
    if (!frame_ready || screen.data == reinterpret_cast<uint8_t *>(g_fb))
        return;

    // The console screen is exactly 128x128. Desktop and web screens are
    // larger, so scale the frame up to the screen height and centre it.
    int size = std::min(screen.bounds.w, screen.bounds.h);

    if (screen.format == PixelFormat::RGB565 && size == 128 && screen.bounds.w == 128) {
        memcpy(screen.data, g_fb, sizeof(g_fb));
        return;
    }

    int ox = (screen.bounds.w - size) / 2;
    int oy = (screen.bounds.h - size) / 2;

    for (int y = 0; y < size; y++) {
        const uint16_t *row = g_fb + (y * 128 / size) * 128;
        for (int x = 0; x < size; x++) {
            uint16_t c = row[x * 128 / size];
            screen.pen = Pen((c >> 11) << 3, ((c >> 5) & 0x3f) << 2, (c & 0x1f) << 3);
            screen.pixel(Point(ox + x, oy + y));
        }
    }
}
