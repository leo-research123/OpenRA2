# TypeClass 图形人工检查

本场景消费真实 `OverlayTypeClass::Draw`、`SmudgeTypeClass::DrawIt`、
`IsometricTileTypeClass::DrawTMP` 的公共请求，不另写类型逻辑。正常构造原类，
C++ 仅解码索引、TMP 菱形和高度元数据，GDScript 的 RenderingDevice 后端执行
GLSL 调色板查询、整数光照、裁剪及深度读写。没有链接 `ra2_software_render`。

## 运行

从仓库根目录使用 `cmake -S code -B out/godot-native -DRA2_BUILD_GODOT=ON` 构建 Godot 扩展，更新 GDExtension 库；
使用 Forward+ 或 Mobile 渲染器，打开本目录 `typeclass_graphics_check.tscn`。
如缺少类，先确认更新的库已加载。Godot 工程位于 `code/godot/`；
不要凭空创建第二份核心或混用旧库。场景和脚本保留在 `res://tests/type_drawing/`；共用 GLSL 在 `res://shaders/map_draw.glsl`，检查路径为单 packet，push constant 索引为 0。正式地图使用导入的 `RDShaderFile`，无需导出原始源码文本。

本场景用于合成类型请求检查，正式游戏测试使用 C# 自动化脚本。

左右切帧，上下改亮度；D 切深度，C 切裁剪，P 切调色板，U 请求不支持的 TMP flag17。
状态应明确返回 `unsupported`，不能画近似结果冒充成功。画面由使用者观察。

## 范围、契约与限制

- 资源和颜色是代码内**显式合成检查样本**，不包含游戏资源，不声称等同原版画面。
  `TypeGpuPacket` 的准备函数可用于已验证并保持存活的真实 SHP/TMP 资源。
- 调色板第一个 256 色表是普通 PaletteData，后续是 shade_count 个 256 色表，
  由宿主准备。示例斜坡不是猜测原版的 LightConvert；真实宿主须提供正确色表。
- SHP 支持本批原版入口实际使用的 0x600/0xE00，按标准 ZFlags=0x3000 选择。
  这两种 blitter 不读取/写入 Z，不会人为增加遮挡测试。Alpha 表示光照，不是透明度。
- SHP 位置按现有原版等尺寸 Blit 的裁剪原点相对规则；TMP 位置绝对。
- TMP 支持主体/附加图像、零索引差异、光照、整数深度、环形状态和层高；
  flat、flag16、flag17 明确 unsupported，尚未实现这些特殊软件效果的 GPU 对等路径。
- 每个包提交后同步，随后才释放输入，再提交下一包。故没有借用地址排队、跨包 Z 竞争。
  GPU 输出回读仅用于这张人工检查场景，不是整局渲染的性能设计，也不是 CPU 软件光栅。
- 输出 `drawn` 表示提交路径完成；图形驱动、实际着色器执行和画面正确性仍需本机检查。
- 不实现 DX；不声称原版 EXE、Windows ABI 或 Godot 运行已在生成环境验收。

API 依据：Godot 4.4 官方 compute shader 教程（RenderingDevice、storage buffers、
submit/sync、buffer_get_data）；godot-cpp 仍使用工程固定提交
`714c9e2c165db2dcb7e6ea57e62a04204d3cfbfa`。
https://docs.godotengine.org/en/4.4/tutorials/shaders/compute_shaders.html
