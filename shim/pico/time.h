/* host pico/time.h shim */
#ifndef _PICO_TIME_H
#define _PICO_TIME_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stdbool.h>
/* chili-doom: the clock comes from the 32blit glue (src/doom_blit.cpp). */
uint64_t time_us_64(void);
typedef uint64_t absolute_time_t;
static inline absolute_time_t make_timeout_time_ms(uint32_t ms) { return time_us_64() + ms * 1000ull; }
static inline bool time_reached(absolute_time_t t) { return time_us_64() >= t; }

#endif
