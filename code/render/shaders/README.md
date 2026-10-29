# Render library shaders

This directory contains pure GLSL 450 stages (`.comp`, `.vert`, `.frag`).
The arithmetic preserves the calibrated RGB565, shape/particle depth and
ABuffer behavior. ABuffer clears to 127; shroud/fog and AlphaShape writes
precede terrain/object consumers in the Tactical background stage.

## Code style

Formatting is mechanical: the stages use the repository C++ style
(`code/.clang-format`: 4-space indent, 120 columns, attached braces, spaced
operators). clang-format has no GLSL mode, so `render/shaders/.clang-format`
inherits that style and only declares `uniform` and `buffer` as block keywords,
which keeps an instance name on the closing line (`} run;`). The
`render_shader_format` test runs `clang-format --dry-run -Werror` on every stage;
format with `clang-format -i code/render/shaders/*.{comp,vert,frag}`. Formatting
must not change the generated SPIR-V (glslang emits no line information without
`-g`), so a format-only change is checked by comparing `render-shaders/*.spv`.

Conventions the formatter does not enforce:

- **Layout of a file.** One stage per file. Line 1 is `#version 450`, followed by
  an English comment stating what the stage computes and which stage or host
  path it pairs with. Then bindings, constants, functions, `main`.
- **Bindings.** One binding per declaration with explicit `set` and `binding`.
  Block names are PascalCase, one member per line, and each binding carries a
  comment describing the layout of its words.
- **Names.** Constants `UPPER_SNAKE`; functions and variables `snake_case`;
  inter-stage varyings use the `v_` prefix; the push-constant block instance is
  `run`.
- **Macros.** Use the preprocessor only for compile-time variants selected by
  the build (`RESIDENT_RESOURCES`, `RING_TILE`) and for named word offsets.
  Runtime conditions are functions or `const bool` values, not macros.
- **Integer arithmetic.** Calibrated RGB565, depth and ABuffer arithmetic uses
  explicit `int`/`uint` conversions; hexadecimal constants use `0x`. Where a
  computation reproduces an original routine, cite its address in the comment.
- **Shared values.** A constant mirrored by C++ (packet flags, word offsets,
  binding indices) names its C++ counterpart in the comment until both sides
  include one shared definition.
- **Comments.** Describe the current contract, not its history.

## Build

With `RA2_BUILD_RENDER_DEVICE=ON`, CMake's `ra2_render_shaders` target compiles
all stages and resident/ring variants with the pinned glslang 15.0.0 host tool.
The `third_party/glslang` adapter downloads verified sources and builds it
automatically; no system installation is required. Cross builds use a separate
native host build. Set `RA2_GLSLANG_EXECUTABLE` only to explicitly override the
bundled tool; clear old paths with `-U RA2_GLSLANG_EXECUTABLE`.
SPIR-V files live in the build's `render-shaders/` directory; generated word
arrays are embedded in `ra2_render`. The Godot Device uses
`shader_create_from_spirv` with these arrays. There are no runtime GLSL imports
or shader resource paths. A normal core-only build needs no shader compiler.

Rebuild the native target after editing shaders:

```sh
cmake --build out/godot-native --target ra2_godot --parallel
```

The independent renderer consumes Frame instance and compute streams and
records ordered Device commands. Godot owns render-thread submission and texture
publication. The frame color is itself the published texture; the map view's
canvas material truncates it to RGB565 and expands it again. Frame clears wait
for the first writer (see Frame structure).

Frame color is an image shared with the raster path (see Raster path): RGBA8
holding expanded RGB565. Scene Z is the raster path's D32F attachment, holding
`Z / 65536`; compute paths only read it, through a nearest sampler.
Shaders read color by truncating each channel to RGB565; writes store the
expansion of an RGB565 word. Compute paths keep the RGB565 word between the read
and the write (only arbitrary RGBA8 source words, such as copies, fills and
palette entries, are truncated first) and expand it once in the store, so they
keep exact RGB565 arithmetic; raster blends produce arbitrary 8-bit values,
which the next read truncates (within one RGB565 step of the original). ABuffer
stays a storage buffer.

## Reading the drawing shader

`map_draw.comp` is ordered as buffer declarations, packet field aliases, mode
constants, RGB565 helpers, effect/depth/palette calculations, and finally `main()`. Start at
`main()` for dispatch order and `draw_pixel()` for source coverage, clipping,
direct copies, circular state addressing and color calculation. Ordinary packets
run one pixel per invocation. Plain warp packets run one invocation in forward
row/pixel order, matching the original offset destination reads. Pixel batch
headers run `draw_pixel()` for each member covering the invocation's pixel, in
packet order. Source reads and early exits retain their existing order.

The field aliases expand to individual reads from the existing integer buffer;
they do not construct or upload a new packet structure. Reused words have
separate names for each meaning, such as `SHADE_COUNT`, `EFFECT_OPACITY`, and
`LIGHT_STRENGTH`. All drawing paths use the same seven bindings and 8 by 8 workgroup:
**0 = parameters, 1 = source, 2 = color image, 3 = Z (sampled D32F), 4 = ABuffer, 5 = palette,
6 = schedule**. Binding 1 (source) and binding 5 (palette) may name one shared buffer: binding 6
holds each packet's source and palette word offsets at words `2 * packet` and
`2 * packet + 1`. A standalone packet with private buffers uses offsets zero.

Isolated packets have one dispatch followed by a barrier. Consecutive laser
packets with disjoint color read/write footprints share a dispatch; an overlap,
another draw kind, or the batch size limit ends that batch. Consecutive
pixel-local packets share a pixel batch (below). Barriers between batches retain
the original order at every affected pixel.

### Packet layout

Each packet is 20 signed 32-bit words (80 bytes). The push constant is one signed
32-bit packet index. The producers are `../src/type_drawing_packets.cpp` and
the core presentation frame compiler; `../src/renderer.cpp` uploads and dispatches them.
This is a private render protocol, not an original-game ABI.

| Word | Meaning |
| --- | --- |
| 0, 1 | Target width and height; also the circular state buffer dimensions |
| 2, 3 | Source width and height; source row stride and dispatch bounds |
| 4, 5 | Destination origin in target pixels |
| 6, 7, 8, 9 | Clip rectangle: x, y, width, height; right/bottom are exclusive |
| 10 | Palette paths: bit 0 enables lighting, bits 16..31 hold the selected SHP blitter's RGB565 tint; spotlight/glow: take strength from the source low byte |
| 11 | Palette intensity; spotlight/glow channel flags; depth-alpha/particle RGB with red in the low byte |
| 12 | Palette shade count; spotlight/glow constant strength; depth-alpha opacity |
| 13 | Depth mode in bits 0..7, blend mode in bits 8..15, original SHP depth flag `0x10000`, auxiliary depth flag `0x20000`, serial plain warp flag `0x40000` |
| 14 | Base depth |
| 15 | Original SHP depth: packed phase, step, limit, signed delta (low to high bytes); other resource depth: enable wrapping to 16 bits |
| 16 | Circular Z/ABuffer row origin |
| 17 | Palette paths: reuse the first visible row's lighting |
| 18 | First visible destination row for SHP depth and repeated lighting |
| 19 | Draw kind, listed below |

Packet depth modes are **0 = none, 1 = read/write, 2 = read**. These are not the
numeric values of the public C++ `ShapeDepthMode` enum. Blend modes are
**0 = palette, 1 = shadow, 2/3/4 = 25/50/75 percent transparency**.

### Drawing paths

All paths clip to the target and clip rectangle. Color indices use screen rows;
Z and ABuffer indices wrap around the circular row origin. ABuffer values are
lighting/visibility intensities, not alpha/opacity. This drawing shader never
changes ABuffer; that is not a rule for the original game's entire frame.

| Kind | Function/path | Scene Z | ABuffer |
| --- | --- | --- | --- |
| 0 | SHP/TMP depth and shaded palette | Packet depth mode | Optional palette shading |
| 1 | Direct RGBA8 copy | Unused | Unused |
| 2 | Fill with source texel 0 | Unused | Unused |
| 3 | Indexed image depth and shaded palette | Packet depth mode | Optional palette shading |
| 4 | `selection_line_color` | Strict test, no write | Zero hides; 127 bypasses scaling |
| 5 | `light_effect_color` (spotlight) | Unused | Unused |
| 6 | `light_effect_color` (depth glow) | Strict test, no write | Unused |
| 7 | `depth_alpha_color` | Strict test, no write | Zero hides; scaling also applies at 127 |
| 8 | `particle_color` | Strict test, no write | Zero hides; scaling applies only below 127 |
| 9 | Additive laser sample | Strict test, no write | Scales RGB at the packet's light-buffer X |
| 10 | Antialiased laser pair | One strict test gates both pixels, no write | One light sample scales both pixels |
| 11 | Batch of kind 9/10 samples | Delegates to each sample | Delegates to each sample |
| 13 | Pixel batch of kinds 0..9 and 14 | Delegates to each member | Delegates to each member |
| 14 | Rally/waypoint dashed pixel (`0x4C0750`) | Unused | Copy only for the requested zero/nonzero light word |

Kind 14 copies one RGBA8 source word without color modulation. Word 10 selects
the ABuffer gate: 0 requires a nonzero word, 1 requires zero. Word 16 supplies
the circular row origin. It does not read or write Z, and never writes ABuffer.
Core drawing ABI version 17 introduces the corresponding `light_nonzero` and
`light_zero` raster modes; software and GPU hosts implement the same test.

Kind 11 is a host scheduling header appended after the original packets. Word 2
holds the sample count and word 4 the first packet index. Each 8 by 8 workgroup
processes 64 samples, with one invocation per packet. Batches contain at most
65,536 samples. The host tracks both pixels of an antialiased pair, including
their reads for the horizontal sample swap; samples sharing either pixel cannot
run in parallel. Packet data, per-pixel arithmetic, and the four-byte push
constant remain unchanged. No original game or core ABI uses these headers.

Kind 13 is also a host scheduling header appended after the original packets.
Word 2/3 hold the batch's bin columns/rows, word 4 its bin table offset in binding
6 and word 5/6 the first bin's column/row. The 32 by 32 bins match
`render::PixelBinSize`; the dispatch covers exactly the bin rectangle. Each bin
entry is an absolute offset/count pair locating ascending member packet indices.
Each invocation owns one target pixel and calls `draw_pixel()` for its bin's
members in that order, so overlapping members keep the original read/modify/write
sequence without a barrier between them. The invocation keeps its color pixel and Z
entry in registers and writes them back after the last member: Metal does not order
an image read after an image write in the same thread without a texture fence.
Other paths read a pixel before writing it, or read pixels not yet written (plain
warp); wave samples read earlier writes and are checked on Metal and Vulkan.
Members must touch only their own pixel
and circular Z/ABuffer entry and share target size and row origin (word 16):
kinds 0..9 and 14 without the serial plain warp flag. Additive lasers already in a
laser batch stay there. Laser pairs, other headers, kind 12, every ABuffer
producer and the background boundary end a batch. Members left outside every bin
have no visible pixel. The host batches runs of at least two members.

Strict depth tests hide equal depth. Palette paths select strict comparison for
original SHP depth and accept equal depth otherwise. Shadows can still write Z
when requested; they return before palette lookup and lighting.

Source texels have different encodings:

- Kind 0: palette index in bits 0..7, resource depth in bits 8..15, occupied bit
  `0x10000`. Auxiliary RLE data also stores prefix overhang in bits 17..24;
  the auxiliary depth byte is interpreted as signed.
- Kind 3: palette index in bits 0..7, signed depth in bits 8..23, occupied bit
  `0x1000000`. A zero palette index is transparent even when occupied.
- Kinds 1/2/4/14: RGBA8 words with red in the low byte.
- Kinds 5/6: source low byte supplies strength when word 10 is nonzero.
- Kinds 7/8: effect RGB comes from the packet. The common source read remains
  in place; these drawing functions do not use its value.

### Integer arithmetic and presentation

`rgba8_to_rgb565` truncates channels. `rgb565_to_rgba8` expands with integer
nearest rounding, matching the CPU palette and reference images. Effect paths
expand RGB565 channels by multiplication by 8/4/8 before their arithmetic;
substituting the normalized output expansion changes results.

`palette_pixel_depth` decodes SHP/TMP/indexed depth without testing or writing Z.
`shaded_palette_index` selects the unlit or prepared shade table; `main()` performs
the palette read and any transparency blend through `draw_pixel()`.

Keep signed casts, channel masks, overflow behavior, individual truncations and
depth comparisons in their documented order. In particular, depth-alpha floors
the two weighted color terms separately before applying light. The RGB565
half/quarter masks prevent shifted channel bits from leaking into a neighbor.
Light handling intentionally differs between palette, selection, alpha and
particle paths; a single generic lighting formula would change their behavior.

`map_present.comp` binds **0 = color image, 1 = ABuffer, 2 = effect snapshot,
3 = kept Composite**; the snapshot and kept Composite are buffers of expanded
RGBA8 words. It uses twelve 32-bit push constants: width, height,
operation, effect stage, warp step, initial duration, brightness, padding,
effect width, effect height and two padding words. Operations:

| Op | Pass |
| ---: | --- |
| 1 | Clear to opaque black, depth `0xFFFF` and light 127. |
| 2 | Copy the frame to the effect snapshot (binding 4). |
| 3 | Apply the Chrono stage inside the effect area. |
| 4 | Reset depth and light, keeping color. |
| 5 | Keep the frame in binding 5 before the new-world capture. |
| 6 | Restore the kept frame in columns below the effect width. |

The Chrono transition follows `ChronoScreenEffect` (0x53B560 / 0x53BBA0 /
0x53C0A0). The effect area is the Composite surface (Tactical view width) minus
its 32-row command bar; the sidebar column and command bar are never written
by op 3. The white stage fills the area with RGB565 `0xFFFF`, the restore stage
copies the snapshot, and the swirl stages whiten the warped snapshot in RGB565:
each channel moves `(brightness * (max - c)) >> 8` toward its 31/63/31
maximum. Swirl sampling approximates the original x87 path; area, clamping
and parameters match.

At the end of the white stage the original renders the new world with
Composite redirected to the snapshot, then restores Composite; the sidebar
surface keeps that redraw. The renderer reproduces this with ops 5, 1, the
capture packets, then 2 and 6. Op 3 never writes the output texture; only op
0 does, after messages drawn over the effect.

## ABuffer producer pass

`map_abuffer.comp` is a separate compute pass with 8 by 8 workgroups. Keeping it
separate leaves the existing drawing shader's execution path unchanged.
Its bindings are **0 = parameters, 1 = decoded source, 2 = writable ABuffer**;
it has no color, Z or palette binding. The push constant is one signed 32-bit
packet index. Packets have the same 20-word stride and geometry slots 0..9 and
16 as the drawing shader, but word 19 selects the following operations. These
packets must be dispatched with the ABuffer pipeline, not the drawing pipeline.
Word 17 is the source word offset into binding 1, zero for a private source
buffer. Words 10..15 and 18 are unused and should be zero.

| Operation (word 19) | Behavior | Original VA |
| --- | --- | --- |
| 0: reset | Write 127 in the clipped destination rectangle; no source read | `0x00411330` |
| 1: shroud | Write source byte except 254, which preserves the old value | `0x0047EFE0` |
| 2: fog | Preserve old value for source > 127; otherwise `max(0, old + source - 127)` | `0x0047F250` |
| 3: AlphaShape | `min(255, old * source / 127)`, truncating integer division | `0x00420F40`, `0x00421350`; table built at `0x00420960` |

ABuffer occupies one 32-bit GPU word per pixel; original WORD intensities are
in 0..255. Source words use bits 0..7 for the intensity and bit `0x10000` for
coverage; all other bits are ignored. **Covered zero is a valid intensity**,
including fully dark AlphaShape pixels. Source 254 skips only in the shroud
operation; it remains a valid AlphaShape multiplier. Fog has its own >127 skip.

The host must decode the selected SHP frame, supply coverage for the original
cell diamond/rectangular path, and keep pixels outside that coverage unmarked.
Resource decoding, frame choice and object lifetime are not shader operations.
Each non-reset source contains `SOURCE_WIDTH * SOURCE_HEIGHT` words from word 17.
Reset uses these dimensions as its rectangle extent and may bind one dummy
source word. Target dimensions must be positive, packet/buffer indices must be
valid, and geometry arithmetic must fit signed 32-bit values.

Screen clipping precedes circular ABuffer addressing, using the same row origin
as `map_draw.comp`. Each invocation owns one destination pixel. Submit overlapping
packets in original order with a barrier after each write dispatch and before
the drawing consumer. Initialization alone is insufficient: the intended order
is frame/region reset, shroud/fog, AlphaShape, then terrain/object reads. Dirty
regions repeat this preparation; partial reset must preserve color and Z.

## Native frame integration and ordered batches

The native renderer resets lighting to 127, then submits Tactical's shroud and
AlphaShape producers before terrain/object drawing. Core methods still execute
on each render. Immutable SHP frames are cached by resource identity and frame;
camera position and clip are per-request metadata.

Drawing and sequential ABuffer sources and palettes share one resource arena, so
one uniform set serves every drawing dispatch. Identities used by consecutive
frames stay resident in its stable region and upload once; identities first used
in a frame occupy a frame region rewritten every frame, so per-frame rasters do
not fill the stable region. A compaction keeps only identities the current frame
references. `RA2_MAP_PROFILE` reports packets, batches, dispatches and arena
uploads. TypeDrawingBackend v12 adds
an optional lighting_shape callback; hosts without it retain the raster fallback.

Inspection of the pinned `gamemd.exe` confirms that the original prepares
ABuffer before drawing terrain: shroud/fog masks write per-pixel values and
AlphaShape images multiply existing values through a lookup table. The neutral
reset value 127 is correct; it does not replace the write phase. Preserve
the drawing shader's non-neutral lighting behavior when changing it.

`map_abuffer_batch.comp` batches each contiguous run of ABuffer producers, cached
SHP frames and transient rasters alike. Bindings are **0 = parameters, 1 = resource
arena, 2 = ABuffer, 3 = spatial bins**. Push constants are width, height, bin
columns, bin-header word offset and four reserved words. Packet word 17 is the source offset in arena words; word 16 must be
zero for the native nonrotating target. Operations 1..3 retain the table above. The
native renderer no longer uses the sequential `map_abuffer.comp`; it remains the
one-request reference for the same operations and their GPU tests.

Each 32×32 destination bin stores an absolute offset/count pair followed by
packet indices in original submission order. Each GPU invocation owns one pixel,
checks clip/coverage and applies each contributing request in that order, including
each AlphaShape division and clamp. This avoids pixel write races. A barrier
follows the batch before any subsequent batch or drawing consumer. Sources live
in the resource arena, so camera movement uploads parameters and bins, not the same
SHP pixels again.

`map_light_field.comp` resolves one lighting batch per pixel
(`LightingShapeBatchRequest`, planned by `lighting_batch.cpp::plan_light_field`).
`TacticalClass::DrawShroud` submits its per-cell shroud requests and the
following AlphaShape requests as one batch. When every cell request draws one
SHP inside its own 60x30 footprint, at a common footprint offset, a pixel lies
in at most two footprints (one per iso row); the shader reads those cells from
a grid indexed by a = x - y and s = x + y, upper row first, then applies the
remaining producers of its 32-pixel bin as `map_abuffer_batch.comp` would.
Pixels no request covers are not written. Bindings are **0 = field tables,
1 = resource arena, 2 = ABuffer**; push constants are width, height, header
offset and one reserved word. Iso-row division must be exact
integer floor division. Any other batch is submitted request by request. The field runs at its position in
packet order (after the screen-effect marks submitted before it).

With a resident background scene, `TacticalClass::Render` replaces
`DrawShroud`'s per-cell pass with `draw_world_shroud`: the core keeps every
cell's occlusion, refreshes it only for `BackgroundShroud` invalidations, sends
changed SHROUD.SHP frames through the scene's `shroud` operation (drawing ABI
24) and writes `Visibility`/`Foggedness` from the cache for cells in view when
the camera moves. The scene's `alpha` operation (drawing ABI 25) delivers the
AlphaShapeClass set whenever the Array changes (`alpha_shapes_changed()` in its
constructor, destructor and Load). The batch then carries only the SHROUD.SHP
image, the footprint origin and its camera (`LightingShapeBatchRequest::shroud`,
`shroud_origin`, `shroud_camera`). `RA2_MAP_WORLD_SHROUD=0` restores the
per-cell pass.

Such a world field is kept in the **light ring** (`map_light_page.comp`): a
world-anchored (toroidal) R8 texture holding, at (world & mask), the value the
field gives a pixel after the frame clear (shroud cells upper row first, then
the AlphaShapes covering it in Array order). The ring is made of 256x256 world
tiles, a power of two of them in each direction (the view plus a tile of margin
on each side: 4096x2048 texels, 8 MiB, for a 1600x900 target), so a world pixel
lives at the same ring texel for as long as its tile is resident and scrolling
only makes the newly exposed tiles. The presentation compiler keeps one SHROUD.SHP frame per
cell index (512 x 512, -1 none) in a storage buffer, uploading changed
1,024-cell chunks; a cell whose frame changed invalidates the tiles under its
60x30 footprint, an added, removed or moved AlphaShape the tiles under its
image, and a new footprint origin or scene every tile. A frame generates only
the tiles under the view that are new or invalid (operation 0, one work group
layer per tile, one invocation per texel) and, when the ABuffer is needed,
expands the ring into it inside the view (operation 1). Bindings are **0 = field
tables (SHROUD.SHP frame table), 1 = resource arena, 2 = world shroud, 3 =
host tables (tile entries, AlphaShapes), 4 = light ring (storage image), 5 =
ABuffer**. `RA2_MAP_PROFILE` reports the tiles generated per frame on the
`LIGHT_RING` line.

## Raster path

`TacticalClass::Render` reports the point where the original publishes the
completed background (`0x6D4539`) through the optional `background_complete`
backend callback (drawing ABI 18). Packets submitted before it form the
background; everything later is foreground. The original kept its background
between frames by blitting a DDraw surface and redrawing exposed strips and dirty
rectangles. The renderer instead draws the whole background every frame from the
resident scene (`BackgroundSceneRequest`, see Resident background below).

Every packet of a frame, background or foreground, that fixed-function state
reproduces is rasterized as a hardware quad. `raster_batch.cpp::raster_packet`
selects them: kind 0 (SHP/TMP) and kind 3 (indexed VXL bitmaps) with any blend,
kind 1 copies and kind 2 fills, with row origin 0 and no plain warp. Every
packet that writes Z is among them; the compute paths that remain (plain warp,
lines, particles, lasers, spotlights, waves, light tests) only read Z. Each
consecutive run of rasterized packets, uninterrupted by a light field or screen
effect mark, is one draw list on the color image and the D32F depth attachment.
`map_raster.vert/.frag` draws
one instance per packet (the quad of its destination rectangle clipped to its clip
and the target) and its fragment stage ports `draw_pixel()` for these packets.
Instances of one draw are consecutive packets with one of 25 pipeline states,
`depth (none, LESS with/without write, LEQUAL with/without write) x blend (palette,
shadow, 25/50/75% transparent)`; draws are in packet order, so blending and depth
testing see every earlier write as the ordered compute dispatches do.

- Depth: D32F holds `Z / 65536`, exact for 16-bit Z, and is the only scene Z.
  Original SHP depth hides equal depth (LESS); other resource depth accepts it
  (LEQUAL). Z beyond 16 bits is clamped where the original wrapped it. The frame
  clear of Z (0xFFFF) is the load action of the next draw list, or an empty draw
  list when a compute path reads Z first.
- Color: palette writes are expanded RGB565, exact. A shadow outputs zero with
  destination factor 0.5 (blend constant), a plain multiply in 8-bit precision;
  the next read truncates the result to RGB565, within one step of the original
  `(d >> 1) & 0x7BEF` per shadow. Translucency outputs the original masked source
  term (`3 * (s >> 2)`, `s >> 1`, `s >> 2` per channel for 25/50/75%
  transparency) and adds the destination times the blend constant (0.25, 0.5,
  and for 75% the per-channel weights 0.6797/0.7148/0.6797 found by exhaustive
  search). Per blend layer the result is within one RGB565 step of the original
  for 25% and 50%, and within two steps for 75%, whose `3 * (d >> 2)` depends on
  the destination's low bits (over all source/destination pairs: 5-bit channels
  12% off by two and 50% by one, 6-bit 14% by two and 49% by one; mean error
  ~0). Stacked blends compound.

`map_raster.vert/.frag` bindings are **0 = parameters, 1 = resource arena, 2 = ABuffer,
3 = schedule** (source/palette word offsets; the draw order of resident draws),
**4 = frame, 5 = cell lights, 6 = converter slots, 7 = light ring** (and, for
resident resources, 9 = resource pages and 10 = resource metadata); the push constant is the run's first packet, camera, mode
(0 immediate, 1 resident, 2/3 dynamic instances) and clip. The vertex stage
resolves everything constant over an instance (packet fields, resident light
sources and converter slot, depth base and gradient, light row) into flat
varyings; fragments read their source sample, the palette and the light. The
renderer requires RGBA8 storage/color/sampled and sampled D32F depth
attachments.

### Frame structure

A frame without compute paths or screen effects is one draw list: the
background (material page resolve or resident rows) and every raster run share
it, and it clears color to opaque black and Z on load. Light comes from the
light ring: when the frame's only ABuffer writer is one world field and no lit
packet precedes it ("page light", frame words 4..12: mode, camera, view and the
ring's address mask), `map_raster.vert/.frag` and `map_resolve.vert/.frag` read the ring
texel at a pixel's world position (127 outside the view) and no ABuffer is
written. Otherwise the field expands the ring into the ABuffer, and being the
first writer it also fills the pixels outside the view with 127.
A compute path closes the draw list; the first compute writer clears color and
ABuffer with `map_present.comp` op 1 when no draw list or field did. Screen
effect operations flush pending clears first. `RA2_MAP_PROFILE` reports rasterized packets,
runs, draws and depth clears, and ends the draw list after the resident background so
`MAP_GPU` times the background (`background_us`: resolve plus its resident rows) and the
foreground runs (`raster_us`) separately; the foreground then loads the background's
color and depth, which costs a little extra only while profiling.

## Resident background: material pages

The resident background scene (drawing ABI 30) keeps its records on the GPU
as 48-byte records in the slots of the core's `TerrainSpriteLayer` instances
(one layer, drawing group and pipeline state per instance, a fixed number of
slots per map cell in original row order). One record buffer holds every
instance from the base slot given by its id; a global slot number is the record
id the pages store (slot 0 is reserved). Each instance also has a draw order:
its occupied slots in ascending order, which the core keeps and sends as it
changes, stored as record ids in an order buffer at the same base. Draws index
the order, so empty slots are never submitted: overlays, overlay shadows and
terrain objects occupy a few percent of the cells, and a visible row range
spans the whole map width. Records name arena offsets, light sources and
converter slots, not light values.

`map_page.vert/.frag` draws the order ranges the core computed for a window around the
viewport (instance i of a draw is the record id at order entry `first + i`) into one 256x256 world-space
page, clipped to it, into one layer of three texture arrays: R32UI material (`record << 8 |
palette index`, 0 = empty), R8 shadow count and D32F world Z. Opaque writes
replace the material and clear the count; shadows keep the material and add
one. Page Z is world Z: gradient and auxiliary depth start at the image's own
first row and column and gradients are quantized on world rows, so an image has
one depth in every page and while the camera moves (the original's rounding
followed the screen rows; equal-depth ties may resolve differently). Resource
Z is not wrapped to 16 bits.

`map_resolve.vert/.frag` draws one quad over the view: each pixel finds its page
through a small page table, evaluates the record's lighting exactly as the
resident raster path did (cell intensity and tint, converter slot, light field),
halves the color once per shadow and writes the world Z read from the page.
Bindings 9 and 10 are the light ring and the frame words (page light, see Frame
structure). Lighting, tint, converter and shroud changes therefore never redraw
a page.
A page is drawn when it enters the view without a valid copy, or when a
changed row gained or lost a record over it (the multiset difference of the
row's record ids gives the rectangles); the least recently used slot is
reused. The pool holds every page a view of the current size can touch plus a
row and column of slack.

### Lit background ring

With page light the lit background does not change while the camera scrolls,
so it is cached too: the **lit background ring** is a world-anchored (toroidal)
RG32UI texture of 256x256 world tiles, one per material page, addressed by
(world & mask) like the light ring (a power of two of tiles covering the view,
no margin: 2048x2048 texels, 32 MiB, for a 1600x900 target). A texel is x =
world Z * 2^24 (0..2^24; bit 31 set: no background pixel) and y = the lit color
as RGB8 (red in the low byte), the exact value the frame holds after the
background, including the per-shadow halving.

`map_resolve.vert/.frag` compiled with `RING_TILE` (the `map_ring_tile` resource, the
same lighting code) resolves one tile of its material page into the ring: one
quad per invalid tile in one draw list on the ring's framebuffer, push
constants = world origin, ring origin, page layer, light ring mask and ring
size, bindings 0, 1, 4..9 of the resolve shader (no page table, ABuffer or frame
words; the light is the world light ring). `map_ring.vert/.frag` draws the per-frame
background: one quad over the view clip fetches the texel at (world & mask) and
writes the frame color and depth (depth test always, written; no-pixel texels
are discarded, so the frame clear shows). Binding 0 = ring; push constants:
camera, clip, ring mask, target size.

A tile is made when it enters the view without a valid copy and is invalidated
by push notification, as a world rectangle, from the presentation change streams:
records changed under it (the same rectangles that invalidate material pages),
a changed record set of the paged prefix, a cell light chunk (compared per cell
against the published chunk before it, mapped to the bounds of the records that
read the cell), a replaced converter palette (bounds of the records using the
slot), and every light ring invalidation. Only the pages of the tiles being made
are needed. The ring is used when the frame is in page light, the light camera
equals the scene origin, the clip lies inside the light view and no record of
the prefix that is lit takes its light from the first visible row (metadata
0x200: its lit color follows the camera; none on ALL01UMD). Otherwise, and
when most of the view's tiles are stale while changes still arrive (global
lighting), the pages are resolved directly with the table-driven pass above,
which stays the exact fallback. `RA2_MAP_LIT_RING=0` always resolves directly.
`RA2_MAP_PROFILE` reports the path, its reason, tiles made and tiles visible
on the `LIT_RING` line, and the ring pass as the `ring` segment of `MAP_GPU`.

A scene with translucent records draws the rows directly
with `map_raster.vert/.frag` (resident mode: binding 0 is the record table and binding 3 the
order buffer; instance i is the record id at order entry `first + i`); the rows the
viewport can touch are one contiguous order range per instance, so a layer costs one draw. `RA2_MAP_PROFILE`
reports pages generated per frame and the record count on `BACKGROUND_GPU`
lines.

## Validation and performance

External GPU validation requires a real RenderingDevice and the same
build's `.spv` files. It can consume the build directory's SPIR-V directly,
without importing shaders.

Compare RGB565 words using the existing accepted blend steps: palette 0,
shadow and 25%/50% transparency 1, 75% transparency 2. Cases that do not write Z
also use the compute path and must match exactly against sampled D32F Z.
Public CPU engineering contracts are documented in `../../tests/README.md`.
External pixel comparisons must declare their own inputs and oracle; CPU
contract results alone do not establish original-game pixel equivalence.

Measure the same workload after warming both implementations. Include GPU
timestamps: `RA2_MAP_PROFILE` preparation and command-recording times measure CPU
work. Capture readback must be disabled for performance measurements.

### Plain voxel ripple

`ConvertClass::SelectPlainBlitter` selects warp translucency for flags containing
`0x8`, a translucency mode and Z-read. PlainBlit seeds the offset at the clipped
first pixel with `(2 * ((top - state_origin_y) & 1)) | ((left ^ (top - state_origin_y)) & 1)`
and XORs it with `3` after every row. Each pixel blends against the destination
at that positive offset, including transparent-source and occluded gaps. The
shader executes these packets serially so reads see the same earlier writes as
the original; ordinary packets retain parallel dispatch. Tinted wrappers force
the offset to zero and ignore tint. RLE warp remains a separate unsupported path.
A read beyond the allocated final surface pixel is rejected instead of copying
the original raw-pointer overread; valid in-surface reads preserve the original
linear address calculation, including crossing a row boundary.
