# UI drawing resources

The version 26 drawing backend adds two optional POD requests:

- `mask`: MSB-first font coverage with RGB565 ink. `game::submit_bitfont_glyph` retains the original character lookup, missing-character color, tab advance and clipping semantics. A missing/unsupported callback expands to the original horizontal raster runs.
- `shape_strip`: consecutive opaque, unlit copies of a UI SHP frame. A missing/unsupported callback expands to the original ordered shape requests. Transparent overlap preserves the previous covered pixel.

The Godot backend caches canonical glyph bits and decoded strips. Glyph keys own the complete coverage bytes (excluding padding), width and height; a font pointer is never a cache identity. Color, position and clip remain per-draw values. Strip keys include the UI resource owner's reload revision, image, frame, count and spacing. `UiResources::clear` issues a new revision before releasing art. Map resource teardown clears both caches; bounded caches can also evict entries while submitted frames keep their resource references alive.

Power bands, sidebar repetitions and command-bar backgrounds submit strips. Buttons, production progress, power updates, text layout and UI side effects continue to execute normally. There is no whole-layer invalidation heuristic and no dependency on `NeedsRedraw` to skip original logic. Radar images and video are outside this change.

Both resource paths are always used when the backend provides the callback; a backend without `mask` or `shape_strip` receives the original expanded request paths. `RA2_MAP_PROFILE=1` reports packet counts, parameter bytes, total buffer updates and CPU preparation timings. Packet hashes differ by design; compare pixels and draw order when validating output.

Validation targets: `ra2_bitfont_tests`, `ra2_type_drawing_tests`, `ra2_type_drawing_packet_tests`, `ra2_game_ui_layout_tests`, `ra2_game_ui_input_tests`, `ra2_power_tests`, `ra2_messages_tests`, `ra2_resource_shutdown_tests`. Font tests compare both paths against the original-machine-code corpus. Strip tests compare full canvases with transparent overlap, offsets and clipping. Real-host checks are `godot/tests/run_power_tests.py` and the `TestGameLayout` scene.
