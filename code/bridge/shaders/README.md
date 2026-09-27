# Bridge compute shaders

This directory is the only maintained source for `map_draw.glsl`,
`map_present.glsl`, `map_abuffer.glsl` and `map_abuffer_batch.glsl`. It contains drawing arithmetic calibrated
against reference cases, including RGB565 arithmetic, strict shape/particle
depth tests and ABuffer reads. ABuffer clears to 127; the original game's
subsequent shroud/fog and AlphaShape writes run before terrain/object consumers
in the native Tactical background stage (see below).

The CMake `ra2_godot` target (through `ra2_godot_shaders`) and the Godot C# project
build stage these files into `code/godot/shaders/`. That entire resource directory,
including generated `.import` metadata, is ignored by Git. Edit shaders here;
changes to the staged copies will be overwritten on the next build.

From the repository root, refresh resources and their compiled Godot imports:

```sh
cmake --build out/godot-native --target ra2_godot_shaders
godot --editor --import --quit --path code/godot --rendering-method mobile
```

Use an existing build configured with `RA2_BUILD_GODOT=ON`; building
`code/godot/ra2opengodot.csproj` with `dotnet build` also prepares the resources.
Import requires a real rendering device. Staging alone does not rebuild SPIR-V.

The bridge renderer and GPU reference tests both load the imported
`res://shaders/map_draw.glsl`. The renderer also loads
`res://shaders/map_present.glsl`. Development and exported builds use these same
resource paths. Core-only and original-game builds do not stage these resources
or depend on Godot. `map_abuffer.glsl` is staged and imported through the same
build flow, together with the batched variant. Both are loaded by the renderer
and exercised by `TestABufferGpuReference`.

The drawing shader consumes the bridge's 20-word packets and six storage-buffer
bindings; dispatches are ordered with barriers. It is a project rendering backend,
not a standalone renderer. The present shader clears color/Z/light or writes the
result to the output texture.

## Reading the drawing shader

`map_draw.glsl` is ordered as buffer declarations, packet field aliases, mode
constants, RGB565 helpers, effect/depth/palette calculations, and finally `main()`. Start at
`main()` to follow source coverage, clipping, direct copies, circular state
addressing, and the appropriate color calculation. Rejection and buffer writes
stay in `main()`; calculation helpers have a single return so inlining does not
introduce extra early-return control flow. Source reads and early exits retain
their existing order.

The field aliases expand to individual reads from the existing integer buffer;
they do not construct or upload a new packet structure. Reused words have
separate names for each meaning, such as `SHADE_COUNT`, `EFFECT_OPACITY`, and
`LIGHT_STRENGTH`. All drawing paths use the same six bindings and 8 by 8 workgroup.
Each packet has one dispatch followed by a barrier, preserving color/Z ordering.

### Packet layout

Each packet is 20 signed 32-bit words (80 bytes). The push constant is one signed
32-bit packet index. The producers are `../src/type_drawing_packets.cpp` and
`../src/map_resources.cpp`; `../src/map_renderer.cpp` uploads and dispatches them.
This is a private bridge protocol, not an original-game ABI.

| Word | Meaning |
| --- | --- |
| 0, 1 | Target width and height; also the circular state buffer dimensions |
| 2, 3 | Source width and height; source row stride and dispatch bounds |
| 4, 5 | Destination origin in target pixels |
| 6, 7, 8, 9 | Clip rectangle: x, y, width, height; right/bottom are exclusive |
| 10 | Palette paths: bit 0 enables lighting, bits 16..31 hold the selected SHP blitter's RGB565 tint; spotlight/glow: take strength from the source low byte |
| 11 | Palette intensity; spotlight/glow channel flags; depth-alpha/particle RGB with red in the low byte |
| 12 | Palette shade count; spotlight/glow constant strength; depth-alpha opacity |
| 13 | Depth mode in bits 0..7, blend mode in bits 8..15, original SHP depth flag `0x10000`, auxiliary depth flag `0x20000` |
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

Strict depth tests hide equal depth. Palette paths select strict comparison for
original SHP depth and accept equal depth otherwise. Shadows can still write Z
when requested; they return before palette lookup and lighting.

Source texels have different encodings:

- Kind 0: palette index in bits 0..7, resource depth in bits 8..15, occupied bit
  `0x10000`. Auxiliary RLE data also stores prefix overhang in bits 17..24;
  the auxiliary depth byte is interpreted as signed.
- Kind 3: palette index in bits 0..7, signed depth in bits 8..23, occupied bit
  `0x1000000`. A zero palette index is transparent even when occupied.
- Kinds 1/2/4: RGBA8 words with red in the low byte.
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
the palette read and any transparency blend.

Keep signed casts, channel masks, overflow behavior, individual truncations and
depth comparisons in their documented order. In particular, depth-alpha floors
the two weighted color terms separately before applying light. The RGB565
half/quarter masks prevent shifted channel bits from leaking into a neighbor.
Light handling intentionally differs between palette, selection, alpha and
particle paths; a single generic lighting formula would change their behavior.

`map_present.glsl` uses four 32-bit push constants: width, height, clear, padding.
Its clear path writes opaque black, depth `0xFFFF` and light 127. Its present
path writes RGB to the output texture with opaque alpha. Both paths keep the
existing bindings and 8 by 8 workgroup.

## ABuffer producer pass

`map_abuffer.glsl` is a separate compute pass with 8 by 8 workgroups. Keeping it
separate leaves the existing drawing shader's execution path unchanged.
Its bindings are **0 = parameters, 1 = decoded source, 2 = writable ABuffer**;
it has no color, Z or palette binding. The push constant is one signed 32-bit
packet index. Packets have the same 20-word stride and geometry slots 0..9 and
16 as the drawing shader, but word 19 selects the following operations. These
packets must be dispatched with the ABuffer pipeline, not the drawing pipeline.
Words 10..15 and 17..18 are unused and should be zero.

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
Each non-reset source buffer contains `SOURCE_WIDTH * SOURCE_HEIGHT` words.
Reset uses these dimensions as its rectangle extent and may bind one dummy
source word. Target dimensions must be positive, packet/buffer indices must be
valid, and geometry arithmetic must fit signed 32-bit values.

Screen clipping precedes circular ABuffer addressing, using the same row origin
as `map_draw.glsl`. Each invocation owns one destination pixel. Submit overlapping
packets in original order with a barrier after each write dispatch and before
the drawing consumer. Initialization alone is insufficient: the intended order
is frame/region reset, shroud/fog, AlphaShape, then terrain/object reads. Dirty
regions repeat this preparation; partial reset must preserve color and Z.

## Native frame integration and ordered batches

The native renderer resets lighting to 127, then submits Tactical's shroud and
AlphaShape producers before terrain/object drawing. Core methods still execute
on each render. Immutable SHP frames are cached by resource identity and frame;
camera position and clip are per-request metadata. TypeDrawingBackend v12 adds
an optional lighting_shape callback; hosts without it retain the raster fallback.

Inspection of the pinned `gamemd.exe` confirms that the original prepares
ABuffer before drawing terrain: shroud/fog masks write per-pixel values and
AlphaShape images multiply existing values through a lookup table. The neutral
reset value 127 is correct; it does not replace the write phase. Preserve
the drawing shader's non-neutral lighting behavior when changing it.

`map_abuffer_batch.glsl` batches each contiguous run of cached lighting requests.
Bindings are **0 = parameters, 1 = persistent source atlas, 2 = ABuffer,
3 = spatial bins**. Push constants are width, height, bin columns and bin-header
word offset. Packet word 17 is the source atlas offset in uint words; word 16
must be zero for the native nonrotating target. Operations 1..3 retain the table
above. Transient raster producers use the sequential shader and break a batch.

Each 32×32 destination bin stores an absolute offset/count pair followed by
packet indices in original submission order. Each GPU invocation owns one pixel,
checks clip/coverage and applies each contributing request in that order, including
each AlphaShape division and clamp. This avoids pixel write races. A barrier
follows the batch before any subsequent batch or drawing consumer. Source bytes
are uploaded only when the atlas gains a frame or the target/generation changes;
camera movement uploads parameters and bins, not the same SHP pixels again.

Run the producer checks on a real RenderingDevice after building the C# tests
and importing shaders (append `--rendering-driver vulkan` to use Vulkan):

```sh
dotnet build code/godot/ra2opengodot.csproj --nologo
godot --editor --import --quit --path code/godot --rendering-method mobile
godot --path code/godot --rendering-method mobile --audio-driver Dummy \
  res://tests/TestABufferGpuReference.tscn
```

The committed fixture contains original x86 outputs for all 65,536 old/source
pairs in each of the three writers. Additional checks cover explicit coverage,
zero pixels, sentinel values, clipping, ring origins, ordered overlap/region
reset and the existing drawing shader reading updated lighting. See the fixture
README for regeneration; these tests do not require original assets at runtime.

## Checking a readability change

Stage and reimport both shaders before running GPU checks. The existing
`res://tests/TestShapeGpuReference.tscn` accepts one or more
`--packets=<absolute fixture path>` arguments and compares color and Z buffers
exactly. It loads the same imported drawing shader as the renderer; compiling
only loose GLSL does not validate the production import.

Use the SHP, indexed raster, light, particle and all-RGB565 fixtures appropriate
to the changed paths. The fixture README in `../../tests/fixtures/README.md`
documents their provenance: some expectations come from original instructions,
others from recovered arithmetic. A before/after comparison proves preservation
of the previous shader only, not additional original-game equivalence.

For performance, compare the same packets on the same device after warming both
pipelines. Include actual GPU timestamps; the renderer's `MAP_PROFILE` measures
CPU preparation and command recording. Function extraction alone is not proof
of identical GPU execution or timing.
