## Rendering Architecture

The renderer converts the original DOS Mode X 320×400 8-bit indexed framebuffer into an SDL3 window with correct aspect ratio, world texture background, and an independently-composited UI layer.

---

### Overview: SDL_GPU pipeline with three indexed layers

Each frame, three indexed textures are composited in order by a single GPU render pass. All three use `SDL_GPU_TEXTUREFORMAT_R8_UNORM` — one palette index per texel. A shared 256×1 RGBA palette texture is bound alongside each indexed texture; the fragment shader (`palette_blit.frag.glsl`) performs the indexed→RGBA lookup entirely on the GPU.

```
┌─────────────────────────────────────────────────────────────┐
│  3. s_ui_tex    (320×400 R8_UNORM, 4:3 letterboxed)         │  UI panels, buttons, cursor, fence
├─────────────────────────────────────────────────────────────┤
│  2. s_battle_tex (320×400 R8_UNORM, 4:3 letterboxed)        │  sprites, fog overlay
├─────────────────────────────────────────────────────────────┤
│  1. s_world_tex (w×h R8_UNORM, viewport sub-rect)           │  terrain tiles (GPU-resident)
└─────────────────────────────────────────────────────────────┘
         shared: s_palette_tex (256×1 R8G8B8A8_UNORM)
```

All four textures are managed by `XModeFlipPage()` in `CODE/compat/sdl_backend.cpp`. There is no `SDL_Renderer` — the backend uses `SDL_GPUDevice` directly.

**Transparency**: index `0xFE` (254) is the engine sentinel for "transparent pixel". The fragment shader emits `vec4(0.0)` (alpha = 0) for index 254, letting lower layers show through.

---

### Key source files

| File | Role |
|------|------|
| `CODE/compat/sdl_backend.cpp` | SDL3 window/GPU setup, all texture uploads, `XModeFlipPage`, `XModeUploadUILayer`, event pump |
| `CODE/compat/shaders/blit.vert.glsl` | Vertex shader: computes clip-space quad from `BlitUniforms` (no vertex buffer) |
| `CODE/compat/shaders/palette_blit.frag.glsl` | Fragment shader: samples R8_UNORM index, looks up RGBA in palette, `0xFE` → alpha 0 |
| `CODE/TIGRE/graphmgr.cpp` | `GraphicsMgr::Animate()` — two-pass scrimage compositor |
| `CODE/TIGRE/graphmgr.hpp` | `VGABuffer vbuf` (battlefield), `VGABuffer vbuf_ui` (UI) |
| `CODE/SRC/viewport.cpp` | `ViewPort::Draw()` — fills `gViewPortCel`; `_BakeWorldTile()` — uploads terrain tiles |
| `CODE/TIGRE/modex.hpp` | Public API: `XModeFlipPage`, `XModeUploadUILayer`, `OS_WorldTexture*` |

---

### GPU pipeline setup

`SetXMode()` in `sdl_backend.cpp`:

1. Creates a resizable `SDL_Window` (default 640×480).
2. Creates `SDL_GPUDevice` with `SDL_GPU_SHADERFORMAT_SPIRV`.
3. Loads compiled SPIR-V shaders from `shaders/` next to the binary (`SDL_GetBasePath()`).
4. Creates a single `SDL_GPUGraphicsPipeline` with alpha blending enabled. The vertex shader takes a `BlitUniforms` uniform buffer (clip-space dst rect + UV rect); the fragment shader takes two samplers (indexed + palette), both nearest-neighbour.
5. Creates four GPU textures and three persistent upload transfer buffers (`s_battle_tbuf`, `s_ui_tbuf`, `s_palette_tbuf`; plus `s_tile_tbuf` for per-tile world updates).

---

### Layer 1: world texture (`s_world_tex`)

- **Format**: `SDL_GPU_TEXTUREFORMAT_R8_UNORM`, dimensions `(WORLD_WIDTH×TILE_WIDTH)` × `(WORLD_HEIGHT×TILE_HEIGHT)` in pixels (e.g. 640×1520 for the largest maps).
- **Content**: raw palette indices for all terrain tiles, with team-colour CLUT remapping (palette indices 192–199) pre-applied for building tiles. No RGB data — the GPU shader resolves colours.
- **CPU shadow**: `s_world_indexed[]` in `sdl_backend.cpp` mirrors the GPU texture in CPU memory. `ARBlit` uses it to substitute real terrain colours for `0xFE` sentinel pixels when capturing the framebuffer for `FadeTo`.
- **Updated lazily**: only tiles marked `tile_dirty[ty][tx]` are re-baked each frame via `ViewPort::_BakeWorldTile()`. Dirty flags are set by: map load (all tiles), `SwapMapTile` (one tile), building construction/capture (footprint tiles). **Palette changes never mark tiles dirty** — the palette texture update handles them on the GPU.
- **Scrolling**: free. `OS_WorldTextureSetView()` stores the viewport source rect and game-space destination position; `XModeFlipPage` converts these to UV coordinates and clip-space each frame with no rebake.
- **Destination**: the letterboxed `s_dst_rect`, scaled to match the visible viewport region (not necessarily the full 320×400 game area).
- **Fog**: never baked here. Fog lives entirely in `gViewPortCel` (layer 2).

Tile upload: `OS_WorldTextureUpdateTile(wx, wy, indexed_pixels, clut_or_null)`.

**TIGRE compaction hazard**: always `memcpy` tile pixel data and CLUT to local stack buffers before calling `OS_WorldTextureUpdateTile`, since any internal `SDL_malloc` can trigger TIGRE compaction and invalidate grip-derived raw pointers.

---

### Layer 2: battlefield (`s_battle_tex`)

Built by **pass 1** of `GraphicsMgr::Animate()` in `graphmgr.cpp`.

1. Pre-fill `vbuf` (320×400) with `0xFE` (transparent sentinel).
2. Iterate `scrimList` ascending by priority; skip any scrimage with `priority >= 10000`.
3. Composite each scrimage into `vbuf` via `vbuf.Load()`.
4. If `g_health_bars_enabled`, call `g_health_bar_hook(vbuf.GetBuffer(), 320, 400)` to draw health bars directly into the indexed buffer.
5. Call `vbuf.Blit()` → writes to `pVGAMem` (the 320×400 indexed framebuffer).

`XModeFlipPage()` then uploads `pVGAMem` to `s_battle_tex` via transfer buffer. The fragment shader converts index → RGBA; `0xFE` → alpha 0 (world texture shows through).

**`gViewPortCel`** (the viewport scrimage, priority 1000) is the most important battlefield scrimage:
- Fogged tiles (`FOG_CENTER`): filled with palette index `0x00` (opaque black).
- Clear tiles: filled with actual indexed terrain pixels. These are opaque and visually match the world texture, but they must be present in `pVGAMem` so `FadeTo::ARBlit` captures real terrain colours (see ARBlit section).
- Fog edge animations (`FOG_ALL`): drawn on top via `CopyCel`.

---

### Layer 3: UI (`s_ui_tex`)

Built by **pass 2** of `GraphicsMgr::Animate()`.

1. Pre-fill `vbuf_ui` (320×400) with `0xFE`.
2. Iterate `scrimList`; skip any scrimage with `priority < 10000`.
3. Composite into `vbuf_ui` via `vbuf_ui.Load()`.
4. Merge non-`0xFE` pixels from `vbuf_ui` into `pVGAMem` (so `FadeTo::ARBlit` sees the full composite — see below).
5. Call `XModeUploadUILayer(vbuf_ui.GetBuffer())`.

`XModeUploadUILayer()` composites the game cursor (and drag-select fence, if active and past the DRAG_THRESHOLD_PX) into `s_ui_indexed` in palette-index space — no RGBA conversion. The result is uploaded to `s_ui_tex` by `XModeFlipPage`.

`s_ui_tex` is rendered to `s_dst_rect` — the same 4:3 letterboxed rectangle as the battle and world layers.

**Priority split point**: `PRI_INTERFACE = 10000` (defined in `CODE/SRC/viewport.hpp`). Scrimages at or above this priority are UI; below are battlefield.

---

### ARBlit and FadeTo compatibility

`ARBlit()` reads pixels FROM `pVGAMem` into a destination buffer. This is used by `FadeTo::Setup()` to capture the current frame before displaying a menu.

Two mechanisms ensure `pVGAMem` contains a complete visual at capture time:

1. **UI pixels**: merged from `vbuf_ui` into `pVGAMem` at the end of pass 2 (step 4 above).
2. **Terrain pixels**: `gViewPortCel` writes actual tile palette indices for clear (non-fogged) tiles. Without this, those areas would contain `0xFE`, which `FadeTo::MapPalette` would remap to a solid grey-pink colour.

`ARBlit` also substitutes `0xFE` pixels using `s_world_indexed`: for any pixel where `pVGAMem` still contains `0xFE` (e.g. tile areas that the viewport cel didn't cover), it reads the terrain index directly from the CPU shadow of the world texture. This handles edge cases such as partial viewport coverage.

---

### Aspect ratio and pixel scaling

The original game ran at 320×400 Mode X on a 4:3 CRT where pixels were ~5:3 taller than wide. `update_dst_rect()` corrects this: it computes the largest 4:3 rectangle that fits within the current window, centering it with pillarbox/letterbox bars, and stores it in `s_dst_rect`. This function is called on `SDL_EVENT_WINDOW_RESIZED`.

Mouse coordinates are mapped through `window_to_game()` which accounts for `s_dst_rect` offset and scale.

---

### Frame sequence

```
ViewPort::Draw()           — rebuild gViewPortCel (terrain + fog), mark tile_dirty[]
ViewPort::_BakeWorldTile() — upload dirty terrain tiles to s_world_tex
GraphicsMgr::Animate()
  SortScrimsByPri()
  MouseHandler(false)
  pass 1: battlefield scrimages → vbuf → [health bar hook] → vbuf.Blit() → pVGAMem
  pass 2: UI scrimages → vbuf_ui → merge into pVGAMem
  XModeUploadUILayer()     — cursor + fence composite in indexed space → s_ui_indexed
  XModeFlipPage()
    [FMV deferred blit if s_fmv_frame set]
    SDL_AcquireGPUCommandBuffer
    upload_texture(pVGAMem  → s_battle_tex via s_battle_tbuf)
    upload_texture(s_ui_indexed → s_ui_tex via s_ui_tbuf)
    upload_texture(s_palette → s_palette_tex, if s_palette_dirty)
    SDL_WaitAndAcquireGPUSwapchainTexture
    SDL_BeginGPURenderPass (clear to black)
      draw_layer(s_world_tex, viewport sub-rect within s_dst_rect)
      draw_layer(s_battle_tex, s_dst_rect)
      draw_layer(s_ui_tex, s_dst_rect)
    SDL_EndGPURenderPass
    SDL_SubmitGPUCommandBuffer
```

The frame rate cap (~60 fps) is enforced in `XModeFlipPage()` using `SDL_GetTicksNS` / `SDL_DelayNS` before acquiring the command buffer.
