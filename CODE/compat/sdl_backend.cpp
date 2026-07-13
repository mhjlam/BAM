/*
 * sdl_backend.cpp
 *
 * SDL3 graphics and input backend for the Blood & Magic Linux port.
 *
 * Implements the engine's VGA/graphics/input API surface using SDL_GPU:
 *   - SetXMode()               : creates SDL3 window + GPU device + shader pipeline
 *   - XModeFlipPage()         : uploads indexed framebuffer, runs palette-lookup shader
 *   - XModeUploadUILayer()    : composites cursor/fence in indexed space, uploads
 *   - OS_SetPalette()         : stores RGBA palette; GPU shader reads it each frame
 *   - OS_WorldTexture*()      : manages indexed world tile texture (no RGB baking)
 *   - ASDLPumpEvents()        : SDL3 event pump for keyboard and mouse
 *   - OS_Quit()               : graceful shutdown
 *
 * The entire indexed→RGBA conversion is done by the GPU fragment shader
 * (palette_blit.frag.glsl).  Palette animation is a 1 KB palette texture
 * update — no world texture rebaking ever occurs on palette change.
 */

#include "types.hpp"
#include "modex.hpp"
#include "apigraph.hpp"
#include "palette.hpp"
#include "apievt.hpp"
#include "eventmgr.hpp"
#include "mouseint.hpp"
#include "keybrd.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdio.h>
#include <string.h>

/* ================================================================
 * GPU state
 * ================================================================ */

static SDL_Window*              s_window     = nullptr;
static SDL_GPUDevice*           s_gpu_device = nullptr;
static SDL_GPUGraphicsPipeline* s_pipeline   = nullptr;
static SDL_GPUSampler*          s_sampler    = nullptr;

/* GPU textures — all R8_UNORM (one palette index per texel) except palette */
static SDL_GPUTexture* s_battle_tex  = nullptr;  /* 320×400, battlefield layer */
static SDL_GPUTexture* s_ui_tex      = nullptr;  /* 320×400, UI + cursor layer */
static SDL_GPUTexture* s_palette_tex = nullptr;  /* 256×1 R8G8B8A8_UNORM, RGBA palette */
static SDL_GPUTexture* s_world_tex   = nullptr;  /* w×h R8_UNORM, indexed world tiles */

/* Persistent upload transfer buffers (cycle=true on Map so GPU pools them) */
static SDL_GPUTransferBuffer* s_battle_tbuf  = nullptr;  /* 320*400 bytes */
static SDL_GPUTransferBuffer* s_ui_tbuf      = nullptr;  /* 320*400 bytes */
static SDL_GPUTransferBuffer* s_palette_tbuf = nullptr;  /* 256*4 bytes */
static SDL_GPUTransferBuffer* s_tile_tbuf    = nullptr;  /* 20*38 bytes */

/* ================================================================
 * CPU-side state
 * ================================================================ */

/* RGBA palette (4th byte always 255).  Uploaded to s_palette_tex when dirty. */
static uint8_t s_palette[256][4];
static bool    s_palette_dirty = false;

/* Drag-select threshold: fence is not shown until mouse moves this many game
 * pixels from the button-down position, preventing accidental micro-drag selects. */
static const int DRAG_THRESHOLD_PX = 4;
static bool  s_fence_visible = false;
static coord s_drag_start_x  = 0;
static coord s_drag_start_y  = 0;

/* Two 320×400 indexed framebuffers replacing VGA pages 0xA0000 / 0xA8000 */
static uint8_t s_framebuf0[320 * 400];
static uint8_t s_framebuf1[320 * 400];

/* CPU-side UI indexed buffer.  XModeUploadUILayer writes here (including cursor
 * and fence compositing); XModeFlipPage uploads it to s_ui_tex. */
static uint8_t s_ui_indexed[320 * 400];

/* FMV deferred blit: AFBlit stores pDecBuf here; XModeFlipPage copies it to
 * pVGAMem after ChangeText has had a chance to write subtitle text. */
static unsigned char* s_fmv_frame = nullptr;

/* ================================================================
 * World tile texture cache
 * ================================================================ */

static int      s_world_tex_w = 0;
static int      s_world_tex_h = 0;
/* Indexed (8-bit) shadow of s_world_tex, kept in sync with the GPU texture so
 * ARBlit can substitute real terrain palette indices for 0xFE pixels. */
static uint8_t* s_world_indexed = nullptr;

/* World viewport parameters — set by OS_WorldTextureSetView, used by XModeFlipPage */
static float s_world_src_x = 0.f, s_world_src_y = 0.f;   /* source rect in world-tex pixels */
static float s_world_src_w = 0.f, s_world_src_h = 0.f;
static float s_world_game_x = 0.f, s_world_game_y = 0.f;  /* game-space destination position */

/* ================================================================
 * 4:3 output rect (in logical window pixels)
 * The original Mode X was 320×400 on a 4:3 CRT (pixels ~5/3 wider than tall).
 * We replicate that by always rendering into a 4:3 sub-rect with letterbox/pillarbox.
 * ================================================================ */
static SDL_FRect s_dst_rect = {0.f, 0.f, 640.f, 480.f};

static void update_dst_rect(void)
{
    int win_w = 640, win_h = 480;
    SDL_GetWindowSize(s_window, &win_w, &win_h);
    const float target_ar = 4.0f / 3.0f;
    float win_ar = (float)win_w / (float)win_h;
    if (win_ar > target_ar) {
        s_dst_rect.h = (float)win_h;
        s_dst_rect.w = s_dst_rect.h * target_ar;
        s_dst_rect.x = ((float)win_w - s_dst_rect.w) * 0.5f;
        s_dst_rect.y = 0.f;
    } else {
        s_dst_rect.w = (float)win_w;
        s_dst_rect.h = s_dst_rect.w / target_ar;
        s_dst_rect.x = 0.f;
        s_dst_rect.y = ((float)win_h - s_dst_rect.h) * 0.5f;
    }
}

/* Convert a window-space mouse position to game-space (320×400). */
static void window_to_game(float wx, float wy, float *gx, float *gy)
{
    *gx = (wx - s_dst_rect.x) * 320.f / s_dst_rect.w;
    *gy = (wy - s_dst_rect.y) * 400.f / s_dst_rect.h;
    if (*gx <   0.f) *gx =   0.f;
    if (*gy <   0.f) *gy =   0.f;
    if (*gx > 319.f) *gx = 319.f;
    if (*gy > 399.f) *gy = 399.f;
}

/* ================================================================
 * SDL_GPU helpers
 * ================================================================ */

/* Uniform buffer pushed to the vertex shader before each draw call.
 * Must match the GLSL BlitUniforms layout (std140: two vec4 = 32 bytes). */
struct BlitUniforms {
    float dst_x, dst_y, dst_w, dst_h;   /* clip-space position and size */
    float src_x0, src_y0, src_x1, src_y1; /* UV rectangle [0,1] */
};

/* Convert a game/window pixel rect and swapchain dimensions to BlitUniforms.
 * SDL_GPU clip space (as presented by the Vulkan backend on this platform):
 * x=−1 left, x=+1 right, y=+1 top, y=−1 bottom (OpenGL-style).
 * dst_h is negative so that increasing off.y moves downward in screen space. */
static BlitUniforms make_blit_uniforms(const SDL_FRect& dst_px, float sw, float sh,
                                        float src_x0, float src_y0,
                                        float src_x1, float src_y1)
{
    BlitUniforms u;
    u.dst_x  =  dst_px.x / sw * 2.0f - 1.0f;
    u.dst_y  =  1.0f - dst_px.y / sh * 2.0f;
    u.dst_w  =  dst_px.w / sw * 2.0f;
    u.dst_h  = -(dst_px.h / sh * 2.0f);
    u.src_x0 = src_x0;  u.src_y0 = src_y0;
    u.src_x1 = src_x1;  u.src_y1 = src_y1;
    return u;
}

static SDL_GPUTexture* create_gpu_texture(SDL_GPUDevice* dev, SDL_GPUTextureFormat fmt,
                                           uint32_t w, uint32_t h)
{
    SDL_GPUTextureCreateInfo ci = {};
    ci.type                  = SDL_GPU_TEXTURETYPE_2D;
    ci.format                = fmt;
    ci.usage                 = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    ci.width                 = w;
    ci.height                = h;
    ci.layer_count_or_depth  = 1;
    ci.num_levels            = 1;
    ci.sample_count          = SDL_GPU_SAMPLECOUNT_1;
    return SDL_CreateGPUTexture(dev, &ci);
}

static SDL_GPUTransferBuffer* create_tbuf(SDL_GPUDevice* dev, uint32_t size)
{
    SDL_GPUTransferBufferCreateInfo ci = {};
    ci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    ci.size  = size;
    return SDL_CreateGPUTransferBuffer(dev, &ci);
}

static SDL_GPUShader* load_shader(SDL_GPUDevice* dev, const char* filename,
                                   SDL_GPUShaderStage stage,
                                   uint32_t num_samplers,
                                   uint32_t num_uniform_buffers)
{
    char path[512];
    const char* base = SDL_GetBasePath();
    snprintf(path, sizeof(path), "%sshaders/%s", base ? base : "", filename);

    size_t code_size = 0;
    void* code = SDL_LoadFile(path, &code_size);
    if (!code) {
        fprintf(stderr, "Cannot load shader %s: %s\n", path, SDL_GetError());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo ci = {};
    ci.code                 = (const Uint8*)code;
    ci.code_size            = code_size;
    ci.entrypoint           = "main";
    ci.format               = SDL_GPU_SHADERFORMAT_SPIRV;
    ci.stage                = stage;
    ci.num_samplers         = num_samplers;
    ci.num_uniform_buffers  = num_uniform_buffers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(dev, &ci);
    SDL_free(code);
    if (!shader)
        fprintf(stderr, "SDL_CreateGPUShader(%s) failed: %s\n", filename, SDL_GetError());
    return shader;
}

/* Upload data to a sub-region of a GPU texture via the given transfer buffer.
 * The command buffer must already be acquired; a copy pass is begun and ended
 * internally so callers can chain multiple uploads in one command buffer. */
static void upload_texture(SDL_GPUCommandBuffer* cmd,
                            SDL_GPUTransferBuffer* tbuf, bool cycle,
                            const void* data, uint32_t data_size,
                            SDL_GPUTexture* tex,
                            uint32_t x, uint32_t y,
                            uint32_t w, uint32_t h)
{
    void* map = SDL_MapGPUTransferBuffer(s_gpu_device, tbuf, cycle);
    memcpy(map, data, data_size);
    SDL_UnmapGPUTransferBuffer(s_gpu_device, tbuf);

    SDL_GPUTextureTransferInfo src_info = {};
    src_info.transfer_buffer = tbuf;
    src_info.offset          = 0;
    src_info.pixels_per_row  = w;
    src_info.rows_per_layer  = h;

    SDL_GPUTextureRegion dst_region = {};
    dst_region.texture = tex;
    dst_region.x = x;  dst_region.y = y;  dst_region.z = 0;
    dst_region.w = w;  dst_region.h = h;  dst_region.d = 1;

    SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);
    SDL_UploadToGPUTexture(cp, &src_info, &dst_region, false);
    SDL_EndGPUCopyPass(cp);
}


/* ================================================================
 * From MODEX.ASM (XMODE.ASM)
 * Declared extern "C" in modex.hpp
 * ================================================================ */

extern "C" {

/* Global video buffer pointers declared extern in modex.hpp */
uchar  *pVGAMem      = s_framebuf0;
uchar  *pVGAMemPage0 = s_framebuf0;
uchar  *pVGAMemPage1 = s_framebuf1;

/* SetXMode — called by graphmgr.cpp:AInitVideo() to enter Mode X.
 * Creates SDL3 window, GPU device, shader pipeline, and all GPU resources. */
void SetXMode(void)
{
    if (s_window) return;   /* guard against double-init (flicsmk calls it too) */

    SDL_Init(SDL_INIT_VIDEO);
    SDL_HideCursor();

    s_window = SDL_CreateWindow("Blood & Magic", 640, 480, SDL_WINDOW_RESIZABLE);

    /* Create GPU device — prefer Vulkan/SPIRV; SDL_GPU falls back automatically */
    s_gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, nullptr);
    if (!s_gpu_device) {
        fprintf(stderr, "SDL_CreateGPUDevice failed: %s\n", SDL_GetError());
        _exit(1);
    }
    if (!SDL_ClaimWindowForGPUDevice(s_gpu_device, s_window)) {
        fprintf(stderr, "SDL_ClaimWindowForGPUDevice failed: %s\n", SDL_GetError());
        _exit(1);
    }

    /* Load compiled SPIR-V shaders from the shaders/ directory next to the binary.
     * Vertex: 1 uniform buffer (BlitUniforms), no samplers.
     * Fragment: 2 samplers (indexed + palette), no uniform buffers. */
    SDL_GPUShader* vert = load_shader(s_gpu_device, "blit.vert.spv",
                                       SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    SDL_GPUShader* frag = load_shader(s_gpu_device, "palette_blit.frag.spv",
                                       SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 0);
    if (!vert || !frag) { _exit(1); }

    /* Pipeline colour target must match the swapchain format. */
    SDL_GPUTextureFormat swapchain_fmt =
        SDL_GetGPUSwapchainTextureFormat(s_gpu_device, s_window);

    SDL_GPUColorTargetBlendState blend = {};
    blend.enable_blend            = true;
    blend.src_color_blendfactor   = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend.dst_color_blendfactor   = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.color_blend_op          = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor   = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_alpha_blendfactor   = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alpha_blend_op          = SDL_GPU_BLENDOP_ADD;
    /* enable_color_write_mask = false → write all channels */

    SDL_GPUColorTargetDescription color_tgt = {};
    color_tgt.format      = swapchain_fmt;
    color_tgt.blend_state = blend;

    SDL_GPUGraphicsPipelineTargetInfo target_info = {};
    target_info.color_target_descriptions = &color_tgt;
    target_info.num_color_targets         = 1;
    target_info.has_depth_stencil_target  = false;

    SDL_GPUGraphicsPipelineCreateInfo pci = {};
    pci.vertex_shader   = vert;
    pci.fragment_shader = frag;
    /* No vertex buffers — positions computed from gl_VertexIndex in the shader. */
    pci.vertex_input_state = {};
    pci.primitive_type     = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pci.rasterizer_state   = {};   /* defaults: FILL, no cull */
    pci.multisample_state  = {};
    pci.depth_stencil_state = {};
    pci.target_info        = target_info;

    s_pipeline = SDL_CreateGPUGraphicsPipeline(s_gpu_device, &pci);
    if (!s_pipeline) {
        fprintf(stderr, "SDL_CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
        _exit(1);
    }

    SDL_ReleaseGPUShader(s_gpu_device, vert);
    SDL_ReleaseGPUShader(s_gpu_device, frag);

    /* Nearest-neighbour sampler — game uses pixel art, never interpolate */
    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter     = SDL_GPU_FILTER_NEAREST;
    sci.mag_filter     = SDL_GPU_FILTER_NEAREST;
    sci.mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    s_sampler = SDL_CreateGPUSampler(s_gpu_device, &sci);

    /* GPU textures */
    s_battle_tex  = create_gpu_texture(s_gpu_device, SDL_GPU_TEXTUREFORMAT_R8_UNORM,      320, 400);
    s_ui_tex      = create_gpu_texture(s_gpu_device, SDL_GPU_TEXTUREFORMAT_R8_UNORM,      320, 400);
    s_palette_tex = create_gpu_texture(s_gpu_device, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, 256, 1);

    /* Persistent transfer buffers */
    s_battle_tbuf  = create_tbuf(s_gpu_device, 320 * 400);
    s_ui_tbuf      = create_tbuf(s_gpu_device, 320 * 400);
    s_palette_tbuf = create_tbuf(s_gpu_device, 256 * 4);
    s_tile_tbuf    = create_tbuf(s_gpu_device, 20 * 38);

    /* Initialise palette (opaque black) and force first-frame upload */
    for (int i = 0; i < 256; i++) {
        s_palette[i][0] = s_palette[i][1] = s_palette[i][2] = 0;
        s_palette[i][3] = 255;
    }
    s_palette_dirty = true;

    /* UI layer starts fully transparent */
    memset(s_ui_indexed, 0xFE, sizeof(s_ui_indexed));

    update_dst_rect();
}

/* XModeFlipPage — DOS: toggles CRTC start address between two VGA pages.
 * Linux: uploads the indexed framebuffer and UI layer to the GPU, then
 * runs a render pass that applies the palette-lookup shader for all three
 * layers (world, battlefield, UI) composited in order.
 * Frame rate is capped here at ~60 fps using wall time. */
void XModeFlipPage(void)
{
    if (!s_gpu_device) return;

    /* FMV deferred blit: AFBlit saved pDecBuf here so ChangeText could run
     * first and write subtitle text into the buffer before we composite it. */
    if (s_fmv_frame) {
        memcpy(pVGAMem, s_fmv_frame, 320 * 400);
        s_fmv_frame = nullptr;
    }

    /* Cap to ~60 fps using wall time. */
    static Uint64 s_lastFlip = 0;
    const Uint64 FRAME_NS = 1000000000ULL / 60;
    Uint64 now = SDL_GetTicksNS();
    if (s_lastFlip && now - s_lastFlip < FRAME_NS) {
        SDL_DelayNS(FRAME_NS - (now - s_lastFlip));
        now = SDL_GetTicksNS();
    }
    s_lastFlip = now;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(s_gpu_device);

    /* Upload indexed battlefield buffer (320×400 = 128 KB, one index per pixel) */
    upload_texture(cmd, s_battle_tbuf, true,
                   pVGAMem, 320 * 400,
                   s_battle_tex, 0, 0, 320, 400);

    /* Upload indexed UI buffer (cursor + fence already composited in indexed space) */
    upload_texture(cmd, s_ui_tbuf, true,
                   s_ui_indexed, 320 * 400,
                   s_ui_tex, 0, 0, 320, 400);

    /* Upload palette if dirty (256 RGBA entries = 1 KB) */
    if (s_palette_dirty) {
        upload_texture(cmd, s_palette_tbuf, true,
                       s_palette, 256 * 4,
                       s_palette_tex, 0, 0, 256, 1);
        s_palette_dirty = false;
    }

    /* Acquire swapchain texture (blocks until one is available at VSync) */
    SDL_GPUTexture* swapchain = nullptr;
    Uint32 sw = 640, sh = 480;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmd, s_window, &swapchain, &sw, &sh);
    if (!swapchain) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return;
    }

    /* Render pass: clear to black (letterbox), then composite layers */
    SDL_GPUColorTargetInfo ct = {};
    ct.texture     = swapchain;
    ct.load_op     = SDL_GPU_LOADOP_CLEAR;
    ct.clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
    ct.store_op    = SDL_GPU_STOREOP_STORE;
    ct.cycle       = false;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &ct, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(pass, s_pipeline);

    SDL_GPUViewport vp = {0.f, 0.f, (float)sw, (float)sh, 0.f, 1.f};
    SDL_SetGPUViewport(pass, &vp);

    /* Helper: bind textures, push uniforms, draw a 6-vertex quad. */
    auto draw_layer = [&](SDL_GPUTexture* indexed_tex,
                           float dst_x, float dst_y, float dst_w, float dst_h,
                           float src_x0, float src_y0, float src_x1, float src_y1) {
        SDL_GPUTextureSamplerBinding bindings[2] = {
            {indexed_tex, s_sampler},
            {s_palette_tex, s_sampler}
        };
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);

        SDL_FRect dst_px = {dst_x, dst_y, dst_w, dst_h};
        BlitUniforms u = make_blit_uniforms(dst_px, (float)sw, (float)sh,
                                             src_x0, src_y0, src_x1, src_y1);
        SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));
        SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    };

    /* Layer 1: world (terrain tiles), drawn first so sprites appear on top */
    if (s_world_tex && s_world_src_w > 0.f) {
        float sx = s_dst_rect.w / 320.f, sy = s_dst_rect.h / 400.f;
        float dst_x = s_dst_rect.x + s_world_game_x * sx;
        float dst_y = s_dst_rect.y + s_world_game_y * sy;
        float dst_w = s_world_src_w * sx;
        float dst_h = s_world_src_h * sy;
        float uv_x0 = s_world_src_x / (float)s_world_tex_w;
        float uv_y0 = s_world_src_y / (float)s_world_tex_h;
        float uv_x1 = (s_world_src_x + s_world_src_w) / (float)s_world_tex_w;
        float uv_y1 = (s_world_src_y + s_world_src_h) / (float)s_world_tex_h;
        draw_layer(s_world_tex, dst_x, dst_y, dst_w, dst_h,
                   uv_x0, uv_y0, uv_x1, uv_y1);
    }

    /* Layer 2: battlefield (sprites, fog) — 0xFE pixels are transparent */
    draw_layer(s_battle_tex,
               s_dst_rect.x, s_dst_rect.y, s_dst_rect.w, s_dst_rect.h,
               0.f, 0.f, 1.f, 1.f);

    /* Layer 3: UI + cursor + drag-select fence — 0xFE pixels are transparent */
    draw_layer(s_ui_tex,
               s_dst_rect.x, s_dst_rect.y, s_dst_rect.w, s_dst_rect.h,
               0.f, 0.f, 1.f, 1.f);

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
}

/* XModeUploadUILayer — composite cursor and drag-select fence into the indexed
 * UI buffer, then stage it for upload in the next XModeFlipPage call.
 * All compositing happens in indexed (palette-index) space: no RGBA conversion. */
void XModeUploadUILayer(const uint8_t* indexed)
{
    memcpy(s_ui_indexed, indexed, 320 * 400);

    /* Composite the game cursor above the UI layer. */
    if (pMouse && !pMouse->hideCount) {
        MouseInt* mi = static_cast<MouseInt*>(pMouse);
        const uchar* cursorData;
        int cw, ch, hotX, hotY;
        if (mi->GetCursorForComposite(&cursorData, &cw, &ch, &hotX, &hotY)) {
            int startX = mi->GetX() - hotX;
            int startY = mi->GetY() - hotY;
            for (int row = 0; row < ch; row++) {
                for (int col = 0; col < cw; col++) {
                    int fx = startX + col, fy = startY + row;
                    if (fx < 0 || fx >= 320 || fy < 0 || fy >= 400) continue;
                    uint8_t pix = cursorData[row * cw + col];
                    if (pix != 0xFE)
                        s_ui_indexed[fy * 320 + fx] = pix;
                }
            }
        }
    }

    /* Composite the drag-select fence if active and threshold reached. */
    if (pMouse && pMouse->fDragMode && s_fence_visible) {
        MouseInt* mi = static_cast<MouseInt*>(pMouse);
        const uchar* hFence;
        const uchar* vFence;
        if (mi->GetFenceForComposite(&hFence, &vFence)) {
            int x1 = pMouse->rCurrentFence.x1, x2 = pMouse->rCurrentFence.x2;
            int y1 = pMouse->rCurrentFence.y1, y2 = pMouse->rCurrentFence.y2;
            if (x2 < x1) { int t = x1; x1 = x2; x2 = t; }
            if (y2 < y1) { int t = y1; y1 = y2; y2 = t; }
            if (x2 > pMouse->rClickDrag.x2) x2 = pMouse->rClickDrag.x2;
            if (y2 > pMouse->rClickDrag.y2) y2 = pMouse->rClickDrag.y2;
            int w = x2 - x1 + 1;
            int h = y2 - y1 + 1;
            for (int col = 0; col < w; col++) {
                int fx = x1 + col;
                if (fx < 0 || fx >= 320) continue;
                uint8_t pix = hFence[col];
                if (pix == 0xFE) continue;
                if (y1 >= 0 && y1 < 400) s_ui_indexed[y1 * 320 + fx] = pix;
                if (y2 != y1 && y2 >= 0 && y2 < 400) s_ui_indexed[y2 * 320 + fx] = pix;
            }
            for (int row = 0; row < h; row++) {
                int fy = y1 + row;
                if (fy < 0 || fy >= 400) continue;
                uint8_t pix = vFence[row];
                if (pix == 0xFE) continue;
                if (x1 >= 0 && x1 < 320) s_ui_indexed[fy * 320 + x1] = pix;
                if (x2 != x1 && x2 >= 0 && x2 < 320) s_ui_indexed[fy * 320 + x2] = pix;
            }
        }
    }
}

void FillScreen(int colorIndex) {
    memset(pVGAMem, (uint8_t)colorIndex, 320 * 400);
}

/* ABlit — copies an offscreen VGABuffer to pVGAMem at (x,y). */
void ABlit(uint /*driver*/, uchar* pData,
           coord x, coord y,
           uint bufWidth, uint bufHeight,
           uint bufSpan, uint /*vSeg*/)
{
    for (uint row = 0; row < bufHeight; row++) {
        uchar* dst = pVGAMem + (y + row) * 320 + x;
        uchar* src = pData   + row * bufSpan;
        memcpy(dst, src, bufWidth);
    }
}

/* ARBlit — reverse blit: read pixels FROM the framebuffer INTO pData.
 * Substitutes 0xFE (transparent sentinel) with the real terrain palette index
 * from s_world_indexed so FadeTo sees the correct colour for terrain pixels. */
void ARBlit(uint /*driver*/, uchar* pData,
            coord x, coord y,
            uint bufWidth, uint bufHeight,
            uint /*vSeg*/)
{
    for (uint row = 0; row < bufHeight; row++) {
        uchar* src = pVGAMem + (y + row) * 320 + x;
        uchar* dst = pData   + row * bufWidth;
        if (!s_world_indexed) {
            memcpy(dst, src, bufWidth);
            continue;
        }
        for (uint col = 0; col < bufWidth; col++) {
            uint8_t px = src[col];
            if (px == 0xFE) {
                int wtx = (int)s_world_src_x + (int)(x + col) - (int)s_world_game_x;
                int wty = (int)s_world_src_y + (int)(y + row) - (int)s_world_game_y;
                if (wtx >= 0 && wtx < s_world_tex_w && wty >= 0 && wty < s_world_tex_h)
                    px = s_world_indexed[wty * s_world_tex_w + wtx];
                else
                    px = 0;
            }
            dst[col] = px;
        }
    }
}

/* ================================================================
 * From OSGRPH.ASM
 * ================================================================ */

void OS_ShutDownVideo(int /*origMode*/)
{
    if (s_world_tex)   { SDL_ReleaseGPUTexture(s_gpu_device, s_world_tex);   s_world_tex   = nullptr; }
    if (s_ui_tex)      { SDL_ReleaseGPUTexture(s_gpu_device, s_ui_tex);      s_ui_tex      = nullptr; }
    if (s_battle_tex)  { SDL_ReleaseGPUTexture(s_gpu_device, s_battle_tex);  s_battle_tex  = nullptr; }
    if (s_palette_tex) { SDL_ReleaseGPUTexture(s_gpu_device, s_palette_tex); s_palette_tex = nullptr; }
    if (s_battle_tbuf) { SDL_ReleaseGPUTransferBuffer(s_gpu_device, s_battle_tbuf);  s_battle_tbuf  = nullptr; }
    if (s_ui_tbuf)     { SDL_ReleaseGPUTransferBuffer(s_gpu_device, s_ui_tbuf);      s_ui_tbuf      = nullptr; }
    if (s_palette_tbuf){ SDL_ReleaseGPUTransferBuffer(s_gpu_device, s_palette_tbuf); s_palette_tbuf = nullptr; }
    if (s_tile_tbuf)   { SDL_ReleaseGPUTransferBuffer(s_gpu_device, s_tile_tbuf);    s_tile_tbuf    = nullptr; }
    if (s_sampler)     { SDL_ReleaseGPUSampler(s_gpu_device, s_sampler);     s_sampler     = nullptr; }
    if (s_pipeline)    { SDL_ReleaseGPUGraphicsPipeline(s_gpu_device, s_pipeline); s_pipeline  = nullptr; }
    if (s_gpu_device)  { SDL_ReleaseWindowFromGPUDevice(s_gpu_device, s_window);
                         SDL_DestroyGPUDevice(s_gpu_device); s_gpu_device = nullptr; }
    if (s_window)      { SDL_DestroyWindow(s_window); s_window = nullptr; }
    SDL_Quit();
}

int OS_GetScreenMode(void) { return 0; }

/* OS_SetPalette — stores 8-bit RGB palette entries and marks the palette
 * texture dirty.  The GPU shader will pick up the new palette on the next
 * XModeFlipPage call — no world texture rebaking is ever required. */
void OS_SetPalette(Gun* guns, uint startGun, uint endGun)
{
    uint n = endGun - startGun + 1;
    for (uint i = 0; i < n; i++) {
        s_palette[startGun + i][0] = guns[i].r;
        s_palette[startGun + i][1] = guns[i].g;
        s_palette[startGun + i][2] = guns[i].b;
        s_palette[startGun + i][3] = 255;
    }
    s_palette_dirty = true;
    /* No s_world_pal_dirty needed — palette texture update handles it. */
}

void OS_GetPalette(Gun* guns, uint startGun, uint endGun)
{
    uint n = endGun - startGun + 1;
    for (uint i = 0; i < n; i++) {
        guns[i].r = s_palette[startGun + i][0];
        guns[i].g = s_palette[startGun + i][1];
        guns[i].b = s_palette[startGun + i][2];
    }
}

int OS_FindPaletteIndex(int r, int g, int b)
{
    int best = 0, bestDist = 0x7fffffff;
    for (int i = 0; i < 256; i++) {
        if (i == 0xFE) continue; // transparent sentinel — never a valid draw colour
        int dr = r - (int)s_palette[i][0];
        int dg = g - (int)s_palette[i][1];
        int db = b - (int)s_palette[i][2];
        int dist = dr*dr + dg*dg + db*db;
        if (dist < bestDist) { bestDist = dist; best = i; }
    }
    return best;
}

} /* extern "C" */

/* ================================================================
 * SDL3 event pump — translates SDL input events into TIGRE events.
 * Called once per game cycle from MouseHandler().
 * ================================================================ */

static uchar sdl_to_tigre_key(SDL_Scancode sc)
{
	switch (sc)
	{
		case SDL_SCANCODE_ESCAPE:       return K_ESC;
		case SDL_SCANCODE_1:            return K_1;
		case SDL_SCANCODE_2:            return K_2;
		case SDL_SCANCODE_3:            return K_3;
		case SDL_SCANCODE_4:            return K_4;
		case SDL_SCANCODE_5:            return K_5;
		case SDL_SCANCODE_6:            return K_6;
		case SDL_SCANCODE_7:            return K_7;
		case SDL_SCANCODE_8:            return K_8;
		case SDL_SCANCODE_9:            return K_9;
		case SDL_SCANCODE_0:            return K_0;
		case SDL_SCANCODE_MINUS:        return 12;
		case SDL_SCANCODE_EQUALS:       return 13;
		case SDL_SCANCODE_BACKSPACE:    return K_BACK_SPACE;
		case SDL_SCANCODE_TAB:          return K_TAB;
		case SDL_SCANCODE_Q:            return K_Q;
		case SDL_SCANCODE_W:            return K_W;
		case SDL_SCANCODE_E:            return K_E;
		case SDL_SCANCODE_R:            return K_R;
		case SDL_SCANCODE_T:            return K_T;
		case SDL_SCANCODE_Y:            return K_Y;
		case SDL_SCANCODE_U:            return K_U;
		case SDL_SCANCODE_I:            return K_I;
		case SDL_SCANCODE_O:            return K_O;
		case SDL_SCANCODE_P:            return K_P;
		case SDL_SCANCODE_LEFTBRACKET:  return K_LEFT_BRACKET;
		case SDL_SCANCODE_RIGHTBRACKET: return K_RIGHT_BRACKET;
		case SDL_SCANCODE_RETURN:       return K_ENTER;
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:        return K_CTRL;
		case SDL_SCANCODE_A:            return K_A;
		case SDL_SCANCODE_S:            return K_S;
		case SDL_SCANCODE_D:            return K_D;
		case SDL_SCANCODE_F:            return K_F;
		case SDL_SCANCODE_G:            return K_G;
		case SDL_SCANCODE_H:            return K_H;
		case SDL_SCANCODE_J:            return K_J;
		case SDL_SCANCODE_K:            return K_K;
		case SDL_SCANCODE_L:            return K_L;
		case SDL_SCANCODE_SEMICOLON:    return K_SEMICOLON;
		case SDL_SCANCODE_APOSTROPHE:   return K_QUOTE;
		case SDL_SCANCODE_GRAVE:        return K_GRAVE;
		case SDL_SCANCODE_LSHIFT:       return K_LEFT_SHIFT;
		case SDL_SCANCODE_BACKSLASH:    return K_BACKSLASH;
		case SDL_SCANCODE_Z:            return K_Z;
		case SDL_SCANCODE_X:            return K_X;
		case SDL_SCANCODE_C:            return K_C;
		case SDL_SCANCODE_V:            return K_V;
		case SDL_SCANCODE_B:            return K_B;
		case SDL_SCANCODE_N:            return K_N;
		case SDL_SCANCODE_M:            return K_M;
		case SDL_SCANCODE_COMMA:        return K_COMMA;
		case SDL_SCANCODE_PERIOD:       return K_PERIOD;
		case SDL_SCANCODE_SLASH:        return K_SLASH;
		case SDL_SCANCODE_RSHIFT:       return K_RIGHT_SHIFT;
		case SDL_SCANCODE_PRINTSCREEN:  return K_PRINT_SCREEN;
		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:         return K_ALT;
		case SDL_SCANCODE_SPACE:        return K_SPACE;
		case SDL_SCANCODE_CAPSLOCK:     return K_CAPS;
		case SDL_SCANCODE_F1:           return K_F1;
		case SDL_SCANCODE_F2:           return K_F2;
		case SDL_SCANCODE_F3:           return K_F3;
		case SDL_SCANCODE_F4:           return K_F4;
		case SDL_SCANCODE_F5:           return K_F5;
		case SDL_SCANCODE_F6:           return K_F6;
		case SDL_SCANCODE_F7:           return K_F7;
		case SDL_SCANCODE_F8:           return K_F8;
		case SDL_SCANCODE_F9:           return K_F9;
		case SDL_SCANCODE_F10:          return K_F10;
		case SDL_SCANCODE_F11:          return K_F11;
		case SDL_SCANCODE_F12:          return K_F12;
		case SDL_SCANCODE_UP:           return K_UP;
		case SDL_SCANCODE_PAGEUP:       return K_PAGE_UP;
		case SDL_SCANCODE_LEFT:         return K_LEFT;
		case SDL_SCANCODE_RIGHT:        return K_RIGHT;
		case SDL_SCANCODE_END:          return K_END;
		case SDL_SCANCODE_DOWN:         return K_DOWN;
		case SDL_SCANCODE_PAGEDOWN:     return K_PAGE_DOWN;
		case SDL_SCANCODE_INSERT:       return K_INSERT;
		case SDL_SCANCODE_DELETE:       return K_DEL;
		case SDL_SCANCODE_HOME:         return K_HOME;
		case SDL_SCANCODE_PAUSE:        return K_BREAK;
		case SDL_SCANCODE_KP_PLUS:      return K_PLUS;
		case SDL_SCANCODE_KP_MINUS:     return K_MINUS;
		case SDL_SCANCODE_KP_1:         return K_PAD_1;
		case SDL_SCANCODE_KP_2:         return K_PAD_2;
		case SDL_SCANCODE_KP_3:         return K_PAD_3;
		case SDL_SCANCODE_KP_4:         return K_PAD_4;
		case SDL_SCANCODE_KP_5:         return K_PAD_5;
		case SDL_SCANCODE_KP_6:         return K_PAD_6;
		case SDL_SCANCODE_KP_7:         return K_PAD_7;
		case SDL_SCANCODE_KP_8:         return K_PAD_8;
		case SDL_SCANCODE_KP_9:         return K_PAD_9;
		case SDL_SCANCODE_KP_0:         return K_PAD_0;
		default:                        return 0;
	}
}

extern "C" void bam_audio_quit(void);

[[noreturn]] void OS_Quit(void)
{
	bam_audio_quit();
	OS_ShutDownVideo(0);
	_exit(0);
}

void ASDLPumpEvents(void)
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			case SDL_EVENT_QUIT:
				OS_Quit();
				break;

			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
			{
				if (ev.type == SDL_EVENT_KEY_DOWN &&
				    ev.key.scancode == SDL_SCANCODE_F4 &&
				    (ev.key.mod & SDL_KMOD_ALT))
				{
					OS_Quit();
				}
				uchar key = sdl_to_tigre_key(ev.key.scancode);
				if (key && pEventMgr)
				{
					pEventMgr->PostScanKey(key, ev.type == SDL_EVENT_KEY_DOWN);
				}
				break;
			}

			case SDL_EVENT_WINDOW_RESIZED:
				update_dst_rect();
				break;

			case SDL_EVENT_MOUSE_MOTION:
			{
				if (pMouse && s_gpu_device)
				{
					float lx, ly;
					window_to_game(ev.motion.x, ev.motion.y, &lx, &ly);
					MouseInt* mi = (MouseInt*)pMouse;
					mi->SetAbsolutePos((coord) lx, (coord) ly);

					if (mi->fDragMode)
					{
						coord cx = mi->GetX(), cy = mi->GetY();
						mi->rCurrentFence.x2 = cx < mi->rClickDrag.x2 ? cx : mi->rClickDrag.x2;
						mi->rCurrentFence.y2 = cy < mi->rClickDrag.y2 ? cy : mi->rClickDrag.y2;
						if (!s_fence_visible) {
							int dx = cx - s_drag_start_x;
							int dy = cy - s_drag_start_y;
							if (dx*dx + dy*dy >= DRAG_THRESHOLD_PX * DRAG_THRESHOLD_PX)
								s_fence_visible = true;
						}
					}
				}
				break;
			}

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
			{
				MouseInt* mi = pMouse ? (MouseInt*)pMouse : nullptr;
				if (mi)
				{
					float lx, ly;
					window_to_game(ev.button.x, ev.button.y, &lx, &ly);
					mi->SetAbsolutePos((coord) lx, (coord) ly);
				}
				if (pEventMgr)
				{
					evt_t  etype = (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
					               ? E_MOUSE_DOWN : E_MOUSE_UP;
					int32  btn   = (ev.button.button == SDL_BUTTON_LEFT)
					               ? LEFT_BTN : RIGHT_BTN;

					if (mi && btn == LEFT_BTN)
					{
						if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
						{
							if (mi->rClickDrag.Contains(mi->GetX(), mi->GetY()))
						{
							mi->EnableDragging(true);
							s_fence_visible = false;
							s_drag_start_x  = mi->GetX();
							s_drag_start_y  = mi->GetY();
						}
						}
					}

					APostEvent(etype, btn, false);

					if (mi && btn == LEFT_BTN && ev.type == SDL_EVENT_MOUSE_BUTTON_UP)
					{
						if (mi->fDragMode)
							mi->EnableDragging(false);
						s_fence_visible = false;
					}
				}
				break;
			}

			default:
				break;
		}
	}
}


/* ================================================================
 * AFBlit — Smacker Mode-X blit (from MODEX.ASM, called by flicsmk.cpp).
 * Defers the copy so ChangeText can write subtitle text first.
 * ================================================================ */
void AFBlit(unsigned char* pData, unsigned int /*vSeg*/)
{
    s_fmv_frame = pData;
}

extern "C" void SVGASetPalette(void* pal)
{
    OS_SetPalette((Gun*)pal, 0, 255);
}


/* ================================================================
 * World tile texture cache
 * ================================================================ */

void OS_WorldTextureCreate(int w_px, int h_px)
{
    if (s_world_tex) {
        SDL_ReleaseGPUTexture(s_gpu_device, s_world_tex);
        s_world_tex = nullptr;
    }
    s_world_tex   = create_gpu_texture(s_gpu_device, SDL_GPU_TEXTUREFORMAT_R8_UNORM,
                                        (uint32_t)w_px, (uint32_t)h_px);
    s_world_tex_w = w_px;
    s_world_tex_h = h_px;
    SDL_free(s_world_indexed);
    s_world_indexed = (uint8_t*)SDL_calloc((size_t)w_px * h_px, 1);

    /* Grow tile transfer buffer if the world texture is larger than one tile */
    if (s_tile_tbuf) {
        SDL_ReleaseGPUTransferBuffer(s_gpu_device, s_tile_tbuf);
        s_tile_tbuf = nullptr;
    }
    s_tile_tbuf = create_tbuf(s_gpu_device, 20 * 38);
}

void OS_WorldTextureDestroy(void)
{
    if (s_world_tex) {
        SDL_ReleaseGPUTexture(s_gpu_device, s_world_tex);
        s_world_tex = nullptr;
    }
    s_world_tex_w = 0;
    s_world_tex_h = 0;
    s_world_src_w = 0.f;   /* inhibit world layer draw in XModeFlipPage */
    SDL_free(s_world_indexed);
    s_world_indexed = nullptr;
}

/* OS_WorldTexturePaletteDirty — always returns false.
 * With the GPU shader approach, palette changes are handled by uploading the
 * palette texture; no world texture rebaking is ever needed.
 * ViewPort::_BakeVisibleTiles() calls this; the no-op return means only
 * explicit tile_dirty[] flags (building changes, SwapMapTile) trigger baking. */
bool OS_WorldTexturePaletteDirty(void)
{
    return false;
}

/* Expand one 20×38 tile from 8-bit indexed to indexed with CLUT pre-applied,
 * upload to the GPU world texture, and update the CPU-side shadow. */
void OS_WorldTextureUpdateTile(int wx, int wy, const uint8_t* indexed_20x38, const uint8_t* clut)
{
    if (!s_world_tex) return;

    /* Apply CLUT (team colour remap for indices 192–199) in index space only.
     * The GPU shader applies the full palette; we just pre-resolve team colours. */
    static uint8_t tile_buf[20 * 38];
    for (int row = 0; row < 38; row++) {
        for (int col = 0; col < 20; col++) {
            int i = row * 20 + col;
            uint8_t idx = indexed_20x38[i];
            if (clut && idx >= 192 && idx < 200)
                idx = clut[idx - 192];
            tile_buf[i] = idx;
            if (s_world_indexed)
                s_world_indexed[(wy + row) * s_world_tex_w + (wx + col)] = idx;
        }
    }

    /* Each tile update uses its own command buffer so uploads complete before
     * the render pass in XModeFlipPage reads the world texture. */
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(s_gpu_device);
    upload_texture(cmd, s_tile_tbuf, true,
                   tile_buf, 20 * 38,
                   s_world_tex, (uint32_t)wx, (uint32_t)wy, 20, 38);
    SDL_SubmitGPUCommandBuffer(cmd);
}

/* Store the world viewport parameters; XModeFlipPage converts them to clip
 * space each frame.  Clamping to texture bounds prevents out-of-range UVs. */
void OS_WorldTextureSetView(float world_x, float world_y,
                             float game_x,  float game_y,
                             float w,       float h)
{
    if (!s_world_tex) return;

    if (world_x < 0.f) { game_x -= world_x; w += world_x; world_x = 0.f; }
    if (world_y < 0.f) { game_y -= world_y; h += world_y; world_y = 0.f; }
    float max_w = (float)s_world_tex_w - world_x;
    float max_h = (float)s_world_tex_h - world_y;
    if (w > max_w) w = max_w;
    if (h > max_h) h = max_h;
    if (w <= 0.f || h <= 0.f) {
        s_world_src_w = 0.f;
        return;
    }

    s_world_src_x  = world_x;
    s_world_src_y  = world_y;
    s_world_src_w  = w;
    s_world_src_h  = h;
    s_world_game_x = game_x;
    s_world_game_y = game_y;
}
