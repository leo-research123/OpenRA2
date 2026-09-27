extends Node2D
# Manual observation harness. Native classes generate genuine drawing requests;
# this RenderingDevice backend owns GPU buffers and executes palette/Z/light.
# Readback is deliberately synchronous and only for this inspection scene.
const SIZE := Vector2i(384, 224)
const SHADE_COUNT := 33
var rd: RenderingDevice
var shader := RID()
var pipeline := RID()
var colors := RID()
var depths := RID()
var lights := RID()
var palette := RID()
var native_check: Node
var image: ImageTexture
var label: Label
var frame := 0
var intensity := 1000
var depth_enabled := true
var special_flag := false
var shifted_clip := false
var palette_variant := false
var last_error := ""

func _ready() -> void:
    label = Label.new()
    label.position = Vector2(12, 12)
    add_child(label)
    if not ClassDB.class_exists("RA2TypeDrawingCheck"):
        label.text = "RA2TypeDrawingCheck is unavailable. Build/load the updated GDExtension."
        return
    rd = RenderingServer.create_local_rendering_device()
    if rd == null:
        label.text = "RenderingDevice unavailable. Select Forward+ or Mobile (not Compatibility)."
        return
    var source := RDShaderSource.new()
    var shader_path: String = "res://shaders/map_draw.glsl"
    var code := FileAccess.get_file_as_string(shader_path)
    if code.is_empty():
        label.text = "Cannot read shader: " + shader_path
        return
    source.source_compute = code.replace("#[compute]\n", "")
    var spirv := rd.shader_compile_spirv_from_source(source)
    if spirv == null:
        label.text = "SPIR-V compilation unavailable."
        return
    var error := spirv.get_stage_compile_error(RenderingDevice.SHADER_STAGE_COMPUTE)
    if not error.is_empty():
        label.text = "Shader compilation failed: " + error
        return
    shader = rd.shader_create_from_spirv(spirv)
    if shader.is_valid():
        pipeline = rd.compute_pipeline_create(shader)
    if not pipeline.is_valid():
        label.text = "GPU shader/pipeline creation failed."
        return
    native_check = ClassDB.instantiate("RA2TypeDrawingCheck")
    add_child(native_check)
    _render_check()

func _free_buffer(handle: RID) -> void:
    if rd != null and handle.is_valid():
        rd.free_rid(handle)

func _create_state() -> bool:
    _free_buffer(colors)
    _free_buffer(depths)
    _free_buffer(lights)
    _free_buffer(palette)
    var color_data := PackedByteArray()
    var z_data := PackedByteArray()
    var a_data := PackedByteArray()
    var pal_data := PackedByteArray()
    color_data.resize(SIZE.x * SIZE.y * 4)
    z_data.resize(color_data.size())
    a_data.resize(color_data.size())
    # The check's RGBA ramp and light field are explicit synthetic fixtures.
    # They are NOT claimed to be recovered original palette/LightConvert data.
    for y in range(SIZE.y):
        for x in range(SIZE.x):
            var i := (y * SIZE.x + x) * 4
            color_data.encode_u32(i, 0xff26221c)
            z_data.encode_u32(i, 65535)
            a_data.encode_u32(i, 64 + (x * 191 / (SIZE.x - 1)))
    pal_data.resize(256 * (SHADE_COUNT + 1) * 4)
    for row in range(SHADE_COUNT + 1):
        # Row zero is the separate unlit PaletteData; remaining rows are
        # FullColorData. A real host supplies its calibrated shade tables here.
        var scale_value: float = 1.0 if row == 0 else float(row - 1) / (SHADE_COUNT - 1)
        for index in range(256):
            var r := int((index * 37) % 256 * scale_value)
            var g := int((index * 17) % 256 * scale_value)
            var b := int((index * 53) % 256 * scale_value)
            if palette_variant:
                var old_r := r
                r = b
                b = old_r
            pal_data.encode_u32((row * 256 + index) * 4, r | (g << 8) | (b << 16) | (255 << 24))
    colors = rd.storage_buffer_create(color_data.size(), color_data)
    depths = rd.storage_buffer_create(z_data.size(), z_data)
    lights = rd.storage_buffer_create(a_data.size(), a_data)
    palette = rd.storage_buffer_create(pal_data.size(), pal_data)
    return colors.is_valid() and depths.is_valid() and lights.is_valid() and palette.is_valid()

# Numeric values exactly match game::DrawingStatus: drawn=0, skipped=1,
# unavailable=2, unsupported=3, invalid_argument=4, backend_failure=5.
func _dispatch(parameters: PackedByteArray, indices: PackedByteArray) -> int:
    if rd == null or not pipeline.is_valid():
        return 2
    if parameters.size() != 80 or indices.is_empty():
        return 4
    var w := parameters.decode_s32(8)
    var h := parameters.decode_s32(12)
    if w < 1 or h < 1 or w > 4096 or h > 4096 or indices.size() != w * h * 4:
        return 4
    if parameters.decode_s32(0) != SIZE.x or parameters.decode_s32(4) != SIZE.y:
        return 4
    var args := rd.storage_buffer_create(parameters.size(), parameters)
    var src := rd.storage_buffer_create(indices.size(), indices)
    if not args.is_valid() or not src.is_valid():
        _free_buffer(args)
        _free_buffer(src)
        return 5
    var buffers: Array[RID] = [args, src, colors, depths, lights, palette]
    var uniforms: Array[RDUniform] = []
    for binding in range(buffers.size()):
        var uniform := RDUniform.new()
        uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
        uniform.binding = binding
        uniform.add_id(buffers[binding])
        uniforms.append(uniform)
    var uniform_set := rd.uniform_set_create(uniforms, shader, 0)
    if not uniform_set.is_valid():
        _free_buffer(src)
        _free_buffer(args)
        return 5
    var commands := rd.compute_list_begin()
    if commands < 0:
        rd.free_rid(uniform_set)
        rd.free_rid(src)
        rd.free_rid(args)
        return 5
    rd.compute_list_bind_compute_pipeline(commands, pipeline)
    rd.compute_list_bind_uniform_set(commands, uniform_set, 0)
    var batch := PackedByteArray()
    batch.resize(16) # The inspection path contains one packet at index zero.
    rd.compute_list_set_push_constant(commands, batch, batch.size())
    rd.compute_list_dispatch(commands, ceili(w / 8.0), ceili(h / 8.0), 1)
    rd.compute_list_end()
    # Finish before releasing inputs or the caller's borrowed request/resources.
    # Separate packets (TMP base then extra, or two types) cannot race on Z.
    rd.submit()
    rd.sync()
    rd.free_rid(uniform_set)
    rd.free_rid(src)
    rd.free_rid(args)
    return 0

func _render_check() -> void:
    if native_check == null or not pipeline.is_valid():
        return
    if not _create_state():
        label.text = "GPU state allocation failed."
        return
    var clip := Rect2i(Vector2i.ZERO, SIZE)
    if shifted_clip:
        clip = Rect2i(24, 12, 256, 164)
    var status: String = native_check.call("render_packets", _dispatch, frame, intensity, depth_enabled, special_flag, clip)
    var output := rd.buffer_get_data(colors)
    if output.size() != SIZE.x * SIZE.y * 4:
        label.text = "GPU readback failed: " + status
        return
    var bitmap := Image.create_from_data(SIZE.x, SIZE.y, false, Image.FORMAT_RGBA8, output)
    image = ImageTexture.create_from_image(bitmap)
    label.text = "TypeClass GPU manual check — no visual result has been certified\n" + status + \
        "\nArrows: frame / light | D: depth | C: clip | P: palette | U: unsupported TMP flag" + \
        "\nframe=%d  intensity=%d  depth=%s  special=%s" % [frame, intensity, depth_enabled, special_flag]
    queue_redraw()

func _draw() -> void:
    if image != null:
        draw_texture_rect(image, Rect2(Vector2(12, 112), Vector2(SIZE) * 2.0), false)

func _unhandled_key_input(event: InputEvent) -> void:
    if not (event is InputEventKey) or not event.pressed or event.echo:
        return
    match event.keycode:
        KEY_RIGHT: frame = (frame + 1) % 3
        KEY_LEFT: frame = (frame + 2) % 3
        KEY_UP: intensity = mini(intensity + 100, 2000)
        KEY_DOWN: intensity = maxi(intensity - 100, 0)
        KEY_D: depth_enabled = not depth_enabled
        KEY_C: shifted_clip = not shifted_clip
        KEY_P: palette_variant = not palette_variant
        KEY_U: special_flag = not special_flag
        _: return
    _render_check()

func _exit_tree() -> void:
    if rd != null:
        _free_buffer(colors)
        _free_buffer(depths)
        _free_buffer(lights)
        _free_buffer(palette)
        _free_buffer(pipeline)
        _free_buffer(shader)
        rd.free()
        rd = null
