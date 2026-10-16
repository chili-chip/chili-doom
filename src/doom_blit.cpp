// chili-doom: glue between the Doom engine (vendor/rp2040-doom + port/) and
// the 32blit SDK.
//
// The engine was written for a bare-metal loop that never returns. Here the
// SDK owns the loop instead: init() runs D_DoomMain() up to the point where
// the game loop would start (see CHILI_BLIT in d_main.c), update() runs one
// Doom frame per 35 Hz tic, and render() copies the finished 128x128 RGB565
// frame onto the screen.
#include <cstring>

#include "32blit.hpp"

using namespace blit;

extern "C" {

// The engine draws every finished frame into this buffer (port/i_video_thumby.c).
uint16_t g_fb[128 * 128];

int boot_active = 0;
void hard_assert(int) {}

void D_DoomMain(void);
void D_RunFrame(void);
int I_GetTime(void);

extern const uint8_t doom1_whd_data[];
extern const uint32_t doom1_whd_data_size;

// --- time ----------------------------------------------------------------

// blit::now_us() wraps every ~71 minutes; Doom wants a 64-bit clock.
uint64_t time_us_64(void) {
    static uint32_t last;
    static uint64_t high;
    uint32_t now = blit::now_us();
    if (now < last)
        high += 1ull << 32;
    last = now;
    return high | now;
}

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

// Pin numbers the port's input code polls (port/i_input_thumby.c), mapped to
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
    set_screen_mode(ScreenMode::hires);

    if (have_wad())
        D_DoomMain();
}

void update(uint32_t) {
    if (!have_wad())
        return;

    // Run Doom once per 35 Hz tic, so TryRunTics() never has to wait for one.
    static int last_tic = -1;
    int tic = I_GetTime();
    if (tic != last_tic) {
        last_tic = tic;
        D_RunFrame();
    }
}

void render(uint32_t) {
    if (!have_wad()) {
        screen.pen = Pen(0, 0, 0);
        screen.clear();
        screen.pen = Pen(255, 255, 255);
        screen.text("No WAD built in.\nSee README.", minimal_font, Point(4, 4));
        return;
    }

    if (!frame_ready)
        return;

    // Centre the 128x128 frame: the console screen is exactly that size,
    // desktop and web screens are larger.
    int ox = (screen.bounds.w - 128) / 2;
    int oy = (screen.bounds.h - 128) / 2;

    if (screen.format == PixelFormat::RGB565 && ox == 0 && oy == 0) {
        memcpy(screen.data, g_fb, sizeof(g_fb));
        return;
    }

    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            uint16_t c = g_fb[y * 128 + x];
            screen.pen = Pen((c >> 11) << 3, ((c >> 5) & 0x3f) << 2, (c & 0x1f) << 3);
            screen.pixel(Point(ox + x, oy + y));
        }
    }
}
