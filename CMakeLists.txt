cmake_minimum_required(VERSION 3.15...3.31)

project(chili-doom LANGUAGES C CXX ASM)

set(PROJECT_DISTRIBS LICENSE README.md)

# --- game data ---------------------------------------------------------------
#
# The WHD blob is made from your own copy of DOOM1.WAD with tools/make_whd.sh
# and is never committed. Without it the game still builds (CI does this) and
# shows "No WAD built in" when started.
set(DOOM_WHD "${CMAKE_CURRENT_SOURCE_DIR}/data/doom1.whd" CACHE FILEPATH "WHD blob made by tools/make_whd.sh")

set(WHD_DIR "${CMAKE_CURRENT_BINARY_DIR}/whd")
file(MAKE_DIRECTORY "${WHD_DIR}")
if(EXISTS "${DOOM_WHD}")
  message(STATUS "chili-doom: embedding ${DOOM_WHD}")
  configure_file("${DOOM_WHD}" "${WHD_DIR}/doom1.whd" COPYONLY)
else()
  message(WARNING "chili-doom: no ${DOOM_WHD}, building without game data. Run tools/make_whd.sh /path/to/doom1.wad")
  file(WRITE "${WHD_DIR}/doom1.whd" "NONE")
endif()

# --- Doom engine -------------------------------------------------------------

# Everything below comes from the ThumbyDOOM submodule (chili-chip fork):
# rp2040-doom, ThumbyDOOM's port/ layer and its host/ pico SDK stand-ins.
set(THUMBY ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ThumbyDOOM)
if(NOT EXISTS ${THUMBY}/vendor/rp2040-doom/src/doom/d_main.c)
  message(FATAL_ERROR "chili-doom: vendor/ThumbyDOOM is empty. Run: git submodule update --init")
endif()
set(VENDOR ${THUMBY}/vendor/rp2040-doom)
set(VSRC   ${VENDOR}/src)
set(VDOOM  ${VSRC}/doom)
set(VOPL   ${VENDOR}/opl)
set(PORT   ${THUMBY}/port)
set(SHIM   ${THUMBY}/host)

file(GLOB DOOM_GAME_SRC CONFIGURE_DEPENDS ${VDOOM}/*.c)

add_library(doom_engine STATIC
  ${VSRC}/aes_prng.c
  ${VSRC}/d_event.c
  ${VSRC}/d_iwad.c
  ${VSRC}/d_loop.c
  ${VSRC}/d_mode.c
  ${VSRC}/deh_str.c
  ${VSRC}/gusconf.c
  ${VSRC}/i_oplmusic.c
  ${VSRC}/i_sound.c
  ${VSRC}/m_argv.c
  ${VSRC}/m_bbox.c
  ${VSRC}/m_cheat.c
  ${VSRC}/m_config.c
  ${VSRC}/m_controls.c
  ${VSRC}/m_fixed.c
  ${VSRC}/m_misc.c
  ${VSRC}/memio.c
  ${VSRC}/midifile.c
  ${VSRC}/mus2mid.c
  ${VSRC}/sha1.c
  ${VSRC}/tables.c
  ${VSRC}/v_video.c
  ${VSRC}/w_checksum.c
  ${VSRC}/w_main.c
  ${VSRC}/w_wad.c
  ${VSRC}/w_file.c
  ${VSRC}/w_merge.c
  ${VSRC}/z_zone.c
  ${VSRC}/tiny_huff.c
  ${VSRC}/musx_decoder.c
  ${VSRC}/image_decoder.c
  ${VSRC}/pd_render.cpp
  ${VSRC}/adpcm-xq/adpcm-lib.c
  ${DOOM_GAME_SRC}
  ${VOPL}/opl_api.c
  ${VOPL}/emu8950.c
  ${VOPL}/slot_render.cpp
  ${PORT}/i_video_thumby.c
  ${PORT}/i_sound_thumby.c
  ${PORT}/i_picosound_thumby.c
  ${PORT}/opl_thumby.c
  ${PORT}/i_system_thumby.c
  ${PORT}/i_timer_thumby.c
  ${PORT}/i_input_thumby.c
  ${PORT}/doom_overlay_menu.c
  ${THUMBY}/device/doom_font.c
  ${PORT}/stubs_thumby.c
  ${PORT}/w_file_thumby.c
  ${PORT}/thumby_hud.c
  ${PORT}/doom1_whd.S
  src/doom_stubs.c
)

# The pico SDK stand-ins must win over any real SDK headers.
target_include_directories(doom_engine BEFORE PRIVATE ${SHIM})
target_include_directories(doom_engine PRIVATE
  ${PORT} ${THUMBY}/device ${VSRC} ${VDOOM} ${VENDOR}/textscreen ${VOPL} ${VSRC}/adpcm-xq ${VSRC}/whd_gen)

set_source_files_properties(${PORT}/doom1_whd.S PROPERTIES COMPILE_FLAGS "-I${WHD_DIR}" OBJECT_DEPENDS "${WHD_DIR}/doom1.whd")

# Compile-time configuration from rp2040-doom's small_doom_common and
# doom_tiny targets, plus ThumbyDOOM's native 128x128 renderer.
target_compile_definitions(doom_engine PRIVATE
  CHILI_BLIT=1
  PICO_ON_DEVICE=0 PICO_BUILD=1

  SHRINK_MOBJ=1 DOOM_ONLY=1 DOOM_SMALL=1 DOOM_CONST=1
  SOUND_LOW_PASS=1 NUM_SOUND_CHANNELS=8
  NO_USE_CHECKSUM=1 NO_USE_RELOAD=1 USE_SINGLE_IWAD=1 NO_USE_WIPE=1 NO_USE_JOYSTICK=1
  NO_USE_DEH=1 NO_USE_MUSIC_PACKS=1 USE_FLAT_MAX_256=1 USE_MEMMAP_ONLY=1 USE_LIGHTMAP_INDEXES=1
  USE_ERASE_FRAME=1 NO_DRAW_MID=1 NO_DRAW_TOP=1 NO_DRAW_BOTTOM=1 NO_DRAW_MASKED=1 NO_DRAW_SKY=1
  NO_VISPLANE_GUTS=1 NO_VISPLANE_CACHES=1 NO_DRAWSEGS=1 NO_VISSPRITES=1 NO_MASKED_FLOOR_CLIP=1
  PD_DRAW_COLUMNS=1 PD_DRAW_MARKERS=1 PD_DRAW_PLANES=1 PD_SCALE_SORT=1 PD_CLIP_WALLS=1
  PD_QUANTIZE=1 PD_SANITY=1 PD_COLUMNS=1 PICO_DOOM=1
  NO_USE_DS_COLORMAP=1 NO_USE_DC_COLORMAP=1 USE_READONLY_MMAP=1
  NO_USE_TIMIDITY=1 NO_USE_GUS=1 NO_USE_LIBSAMPLERATE=1
  EMU8950_NO_WAVE_TABLE_MAP=1 EMU8950_NO_TLL=1 EMU8950_NO_FLOAT=1 EMU8950_NO_TIMER=1
  EMU8950_NO_TEST_FLAG=1 EMU8950_SIMPLER_NOISE=1 EMU8950_SHORT_NOISE_UPDATE_CHECK=1
  EMU8950_LINEAR_SKIP=1 EMU8950_LINEAR_END_OF_NOTE_OPTIMIZATION=1 EMU8950_NO_PERCUSSION_MODE=1
  EMU8950_LINEAR=1 EMU8950_NO_RATECONV=1
  USE_RAW_MAPNODE=1 USE_RAW_MAPVERTEX=1 USE_RAW_MAPSEG=1 USE_RAW_MAPLINEDEF=1 USE_RAW_MAPTHING=1
  USE_INDEX_LINEBUFFER=1 NO_USE_ZLIGHT=1 NO_Z_ZONE_ID=1 Z_MALOOC_EXTRA_DATA=1 USE_THINKER_POOL=1
  NO_INTERCEPTS_OVERRUN=1 TEMP_IMMUTABLE_DISABLED=1 USE_CONST_SFX=1 USE_CONST_MUSIC=1
  NO_DEMO_RECORDING=1 PICO_NO_TIMING_DEMO=1 NO_USE_EXIT=1 NO_USE_STATE_MISC=1

  DOOM_TINY=1 EMU8950_SLOT_RENDER=1 NO_RDRAW=1 USE_EMU8950_OPL=1 USE_DIRECT_MIDI_LUMP=1
  NO_USE_NET=1 USE_PICO_NET=1 NO_FILE_ACCESS=1 SAVE_COMPRESSED=1 LOAD_COMPRESSED=1
  NO_USE_ARGS=1 NO_USE_SAVE_CONFIG=1 NO_USE_FLOAT=1 USE_VANILLA_KEYBOARD_MAPPING_ONLY=1
  NO_USE_LOADING_DISK=1 USE_WHD=1 NO_Z_MALLOC_USER_PTR=1 FLOOR_CEILING_CLIP_8BIT=1
  USE_MUSX=1 MUSX_COMPRESSED=1 NO_SCREENSHOT=1 NO_USE_BOUND_CONFIG=1 USE_FPS=1
  USE_MEMORY_WAD=1 NO_IERROR=1 NO_USE_ENDDOOM=1

  SCREENWIDTH=128 SCREENHEIGHT=128 MAIN_VIEWHEIGHT=112 THUMBY_NATIVE=1

  # Doom's zone heap. ThumbyDOOM uses 160 KB; the demo loop peaks at about
  # 48 KB of non-purgeable blocks, and the console needs the 16 KB this
  # frees for malloc (OPL music, MIDI tracks, the SDK).
  THUMBY_ZONE_KB=144
)

target_compile_options(doom_engine PRIVATE
  -w -fno-strict-aliasing
  # Every vendor file needs the pico.h stand-in (__packed, panic, ...).
  "$<$<COMPILE_LANGUAGE:C,CXX>:SHELL:-include ${SHIM}/pico.h>"
)
set_source_files_properties(${VOPL}/slot_render.cpp ${VOPL}/emu8950.c ${VOPL}/opl_api.c
  PROPERTIES COMPILE_FLAGS "-fms-extensions")

# --- 32blit game -------------------------------------------------------------

find_package (32BLIT CONFIG REQUIRED PATHS ../32blit-sdk $ENV{PATH_32BLIT_SDK})

blit_executable (${PROJECT_NAME} src/doom_blit.cpp)
target_link_libraries(${PROJECT_NAME} doom_engine)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)

if(32BLIT_PICO)
  # The game hands the SDK its own 128x128 frame as the screen (see init() in
  # src/doom_blit.cpp), so the SDK's startup framebuffer only has to hold the
  # small lores screen shown before init(): 16 KB instead of 64 KB.
  target_compile_definitions(${PROJECT_NAME} PRIVATE ALLOW_HIRES=0)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
else()
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
endif()

blit_metadata (${PROJECT_NAME} metadata.yml)
add_custom_target (flash DEPENDS ${PROJECT_NAME}.flash)

install (FILES ${PROJECT_DISTRIBS} DESTINATION .)
set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set (CPACK_GENERATOR "ZIP" "TGZ")
include (CPack)
