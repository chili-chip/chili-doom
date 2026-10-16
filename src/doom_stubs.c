// chili-doom: symbols the engine expects from the pico SDK.
//
// Kept out of doom_blit.cpp because on the console the real pico SDK
// headers are in scope there, and they declare hard_assert() differently
// (as a static inline function the engine can't link against).

void hard_assert(int cond) { (void)cond; }
