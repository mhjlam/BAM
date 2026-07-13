cmake_minimum_required(VERSION 3.16)
project(BloodAndMagic CXX)

find_package(SDL3 REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3_MIXER REQUIRED IMPORTED_TARGET sdl3-mixer)
pkg_check_modules(ENET REQUIRED IMPORTED_TARGET libenet)
find_library(SMACKER_LIB smacker REQUIRED)

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)   # enables gnu++ extensions

option(ASAN "Enable AddressSanitizer" OFF)

add_compile_options(
    -g
    -O2
    # Force-include into every TU: _os_wall_clock() and OS_Quit() declaration.
    -include ${CMAKE_SOURCE_DIR}/CODE/compat/portability.h

    # Targeted suppressions for architecturally intentional Watcom C++ patterns:
    # -Wint-to-pointer-cast / -Wpointer-arith: grip (uint32) ↔ void* in list
    #   storage is architecturally sound (grips are small integers, not real ptrs)
    -Wno-int-to-pointer-cast
    -Wno-pointer-arith
)

if(ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# ---------------------------------------------------------------------------
# Preprocessor definitions
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Include directories
# compat/ must come first so that portability.h (force-included above) is
# found for direct #include uses, and so TIGRE headers are visible to
# sdl_backend.cpp and smacker.cpp.
# ---------------------------------------------------------------------------
include_directories(
    ${CMAKE_SOURCE_DIR}/CODE/compat
    ${CMAKE_SOURCE_DIR}/CODE/TIGRE
    ${CMAKE_SOURCE_DIR}/CODE/SRC
)

# ---------------------------------------------------------------------------
# TIGRE engine sources  (from TIGRE.MAK, ASM files replaced by asm_stubs)
# ---------------------------------------------------------------------------
set(TIGRE_SOURCES
    CODE/TIGRE/api.cpp
    CODE/TIGRE/apidlg.cpp
    CODE/TIGRE/apievt.cpp
    CODE/TIGRE/apifont.cpp
    CODE/TIGRE/apigraph.cpp
    CODE/TIGRE/apimem.cpp
    CODE/TIGRE/apires.cpp
    CODE/TIGRE/cliprect.cpp
    CODE/TIGRE/clock.cpp
    CODE/TIGRE/comp.cpp
    CODE/TIGRE/comm.cpp
    CODE/TIGRE/commmgr.cpp
    CODE/TIGRE/tenetcomm.cpp
    CODE/TIGRE/config.cpp
    CODE/TIGRE/context.cpp
    CODE/TIGRE/debug.cpp
    CODE/TIGRE/dialog.cpp
    CODE/TIGRE/eventmgr.cpp
    CODE/TIGRE/fontmgr.cpp
    CODE/TIGRE/file.cpp
    CODE/TIGRE/graphmgr.cpp
    CODE/TIGRE/keylist.cpp
    CODE/TIGRE/list.cpp
    CODE/TIGRE/manager.cpp
    CODE/TIGRE/mem.cpp
    CODE/TIGRE/memmgr.cpp
    CODE/TIGRE/mono.cpp
    CODE/TIGRE/mouse.cpp
    CODE/TIGRE/mouseint.cpp
    CODE/TIGRE/mousescr.cpp
    CODE/TIGRE/object.cpp
    CODE/TIGRE/objmgr.cpp
    CODE/TIGRE/os.cpp
    # osgrph.obj in TIGRE.MAK comes from OSGRPH.ASM (not OSGRAPH.CPP)
    # — provided by asm_stubs below
    CODE/TIGRE/palette.cpp
    CODE/TIGRE/periodic.cpp
    CODE/TIGRE/rect.cpp
    CODE/TIGRE/resmgr.cpp
    CODE/TIGRE/resource.cpp
    CODE/TIGRE/savebase.cpp
    CODE/TIGRE/savemgr.cpp
    CODE/TIGRE/game_config.cpp
    CODE/TIGRE/sconfig.cpp
    CODE/TIGRE/scrimage.cpp
    CODE/TIGRE/soundmgr.cpp
    CODE/TIGRE/stream.cpp
    CODE/TIGRE/srle.cpp
    CODE/TIGRE/t12.cpp
    CODE/TIGRE/text.cpp
    CODE/TIGRE/trle.cpp
    # vesa.obj  → asm_stubs
    CODE/TIGRE/vgabuf.cpp
    # SDL3 graphics and input backend (replaces MODEX.ASM, OSGRPH.ASM, PENTIMER.ASM, VESA.ASM)
    CODE/compat/sdl_backend.cpp
    # libsmacker wrapper implementing the RAD Smacker API for Linux FMV playback
    CODE/compat/smacker.cpp

)

# ---------------------------------------------------------------------------
# BAM game sources  (from BAM.MAK)
# ---------------------------------------------------------------------------
set(BAM_SOURCES
    CODE/SRC/ai.cpp
    CODE/SRC/assess.cpp
    CODE/SRC/bam.cpp
    CODE/SRC/bam_dg.cpp
    CODE/SRC/bamfuncs.cpp
    CODE/SRC/bamfunc2.cpp
    CODE/SRC/bamguy.cpp
    CODE/SRC/bampopup.cpp
    CODE/SRC/bamroom.cpp
    CODE/SRC/cine.cpp
    CODE/SRC/choose.cpp
    CODE/SRC/credits.cpp
    CODE/SRC/death.cpp
    CODE/SRC/encyclo.cpp
    CODE/SRC/encymenu.cpp
    CODE/SRC/fade.cpp
    CODE/SRC/flicsmk.cpp
    CODE/SRC/fmt_lbm.cpp
    CODE/SRC/hall.cpp
    CODE/SRC/introhal.cpp
    CODE/SRC/items.cpp
    CODE/SRC/legendop.cpp
    CODE/SRC/mainmenu.cpp
    CODE/SRC/makechar.cpp
    CODE/SRC/maps.cpp
    CODE/SRC/mapsbase.cpp
    CODE/SRC/netchar.cpp
    CODE/SRC/nethall.cpp
    CODE/SRC/netconnect.cpp
    CODE/SRC/netstory.cpp
    CODE/SRC/option.cpp
    CODE/SRC/option2.cpp
    CODE/SRC/option3.cpp
    CODE/SRC/option4.cpp
    CODE/SRC/settingsmenu.cpp
    CODE/SRC/pather.cpp
    CODE/SRC/savemenu.cpp
    CODE/SRC/snap.cpp
    CODE/SRC/spendexp.cpp
    CODE/SRC/story.cpp
    CODE/SRC/tutorial.cpp
    CODE/SRC/units.cpp
    CODE/SRC/unitlib.cpp
    CODE/SRC/viewport.cpp
    CODE/SRC/winlose.cpp
    CODE/SRC/winlose2.cpp
    CODE/SRC/world.cpp
    CODE/SRC/worldmap.cpp
)

# ---------------------------------------------------------------------------
# GLSL → SPIR-V shader compilation (for SDL_GPU palette-lookup renderer)
# ---------------------------------------------------------------------------
find_program(GLSLC glslc PATHS /usr/bin /usr/local/bin /usr/local/shaderc/bin REQUIRED)

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/CODE/compat/shaders)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

add_custom_command(
    OUTPUT  ${SHADER_OUT_DIR}/blit.vert.spv
    COMMAND ${GLSLC} -fshader-stage=vert ${SHADER_SRC_DIR}/blit.vert.glsl
                     -o ${SHADER_OUT_DIR}/blit.vert.spv
    DEPENDS ${SHADER_SRC_DIR}/blit.vert.glsl
    COMMENT "Compiling blit.vert.glsl"
)
add_custom_command(
    OUTPUT  ${SHADER_OUT_DIR}/palette_blit.frag.spv
    COMMAND ${GLSLC} -fshader-stage=frag ${SHADER_SRC_DIR}/palette_blit.frag.glsl
                     -o ${SHADER_OUT_DIR}/palette_blit.frag.spv
    DEPENDS ${SHADER_SRC_DIR}/palette_blit.frag.glsl
    COMMENT "Compiling palette_blit.frag.glsl"
)
add_custom_target(shaders
    DEPENDS ${SHADER_OUT_DIR}/blit.vert.spv
            ${SHADER_OUT_DIR}/palette_blit.frag.spv
)

# ---------------------------------------------------------------------------
# Build target
# ---------------------------------------------------------------------------
add_executable(bam ${TIGRE_SOURCES} ${BAM_SOURCES})
add_dependencies(bam shaders)
target_link_libraries(bam SDL3::SDL3 PkgConfig::SDL3_MIXER ${SMACKER_LIB} PkgConfig::ENET)
target_link_options(bam PRIVATE -rdynamic)
