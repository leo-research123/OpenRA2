# 独立 C++ 核心与适配目标

`ra2_core` 是不依赖 Godot 的 C++20 静态库。包含 INI、MIX、文件服务、SHP/TMP/VXL/HVA 资源和第一阶段挂包流程、Scenario/Rules、地图与对象绘制、移动/寻路/战斗、生产/电力、雷达、原版 UI 与主循环的已移植部分，以及音视频后端接口。源码存在不代表完整游戏行为已复现。Godot 桥接单独构建，内存方法统一由 core 提供。YRpp 原有条件编译、跳转写法和调用约定保持不变。

地图会话公开合同见 [api/map_view.hpp](core/include/api/map_view.hpp) 与 [api/map_objects.hpp](core/include/api/map_objects.hpp)：会话独占借用资源环境，拥有正常 Scenario/Tactical，实际 Cells 保存在稳定原 Map 根对象中。`load_map_view` 已串联类型、地形、对象与场景字段加载，`CellClass::SetupLAT`/`RecalcAttributes` 已有实现并接入对象加载；`draw_map_view` 绘制地图，`draw_game_view` 绘制完整逻辑画布，`submit_game_input` 与 `advance_game_view` 接入原类输入和主循环。原类组织游戏状态与绘制，Godot bridge 管理 GPU 缓存、提交与纹理。当前启动仍包含浏览模式的全图揭示等行为，不能视为完整原版开局实现。

基础核心已包含最小 Surface、色表生成、FileSystem 名称缓存及绘制请求分发，不要求链接可选 Convert/Blitter 软件光栅组件。


## 目录与接口

| 目录 | 职责 |
|---|---|
| `core/include/yrpp` | YRpp 原类公开声明，保留原有作用域 |
| `core/include/api` | 宿主接口：版本、资源生命周期/解码、地图/对象/UI、绘制、计时、类型资源/流、INI/Rules/Scenario 服务、音视频后端与诊断 |
| `core/src` | 核心实现与内部头，项目服务使用顶层 `game` 命名空间 |
| `core/src/filesystem` | 物理文件、名称、搜索路径、资源上下文、挂包所有权与文件启动适配 |
| `core/src/yrpp` | 对应已有原类或模块的实现与配套算法 |
| `bridge/include/bridge`、`bridge/src` | Godot 适配、工作线程、进度快照与图集 |
| `render` | 独立帧编译、GPU 调度和 Device 接口，不依赖 core、YRpp 或 Godot |
| `render/shaders` | 纯 GLSL 阶段源码；构建时生成并嵌入 SPIR-V |
| `experiments`、`third_party` | 实验代码与固定版本的外部依赖 |

核心只公开 `core/include`，调用方使用 `api/...`、`yrpp/...`。公开头与 Godot 桥接不能依赖核心内部头；测试访问内部头时按目标显式配置。目录依赖由 [CheckYrppLayout.cmake](cmake/CheckYrppLayout.cmake) 检查；[CheckCoreOrganization.cmake](cmake/CheckCoreOrganization.cmake) 补充原模块归属、公开头清单、文件关停与链接依赖检查。

YRpp 头须独立于 `game::`；新增/扩展的公开 ABI 使用基础类型、原版已有类型、指针与长度或有类型的不透明句柄，不能暴露 STL 对象，项目自有 C++ 代码不能使用 `std::optional`。

项目版本由核心持有：`RA2_PROJECT_VERSION` 仅作为 `ra2_core` 的私有编译定义，调用方通过 `api/version.hpp` 的 `game::project_version` 读取。Godot 的 `RA2Core.get_version()` 转发该核心版本，C# 宿主继续通过桥接读取。

## 独立构建

```sh
cmake -S code -B out/resource-core -DRA2_BUILD_GODOT=OFF -DRA2_BUILD_MOVIES=OFF -DBUILD_TESTING=ON
cmake --build out/resource-core --parallel
ctest --test-dir out/resource-core --output-on-failure --timeout 60
```

以上命令从仓库根目录执行。`RA2_BUILD_GODOT` 和 `RA2_BUILD_MOVIES` 默认开启；以上命令显式关闭两者，此路径不下载 godot-cpp，也不需要 Godot、.NET、FFmpeg 或原版素材。真实素材用例另需 `RA2_GAME_DATA`，缺省时相关子用例跳过。

只构建静态库时显式关闭 `RA2_BUILD_GODOT` 和 `RA2_BUILD_MOVIES`，设置 `BUILD_TESTING=OFF`，并构建 `--target ra2_core`。多配置生成器需为构建指定 `--config Debug`，为测试指定 `-C Debug`。

其他 CMake 工程可通过 `add_subdirectory` 引入本目录，关闭 `RA2_BUILD_GODOT`、`RA2_BUILD_MOVIES` 和不需要的测试，再链接 `ra2::core`。

`RA2_BUILD_MOVIES=ON` 将 BIK/VQA 解码实现编入 `ra2_core`。此选项默认开启，可在仅构建核心时显式关闭；需要 FFmpeg 7 的 libavformat、libavcodec、libavutil、libswscale、libswresample 开发库。FFmpeg 类型不进入公开接口。`ra2::movies` 保留为链接核心的兼容目标。

核心可以使用 Emscripten 构建，bridge 与 Godot 不参与此目标：

```sh
PKG_CONFIG_PATH=/path/to/wasm-ffmpeg/lib/pkgconfig emcmake cmake \
  -S code -B out/wasm-core -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DCMAKE_FIND_ROOT_PATH=/path/to/wasm-ffmpeg \
  -DBUILD_TESTING=OFF -DRA2_BUILD_GODOT=OFF -DRA2_BUILD_MOVIES=ON
cmake --build out/wasm-core --target ra2_core --parallel
```

此处 FFmpeg 必须由同一 WASM 工具链交叉编译，不能链接本机库；关闭解码选项则无需 FFmpeg。核心目标传播 `-fwasm-exceptions`，宿主运行环境需支持 WebAssembly 异常，以保留内部失败处理。静态库还需由新宿主链接成 `.wasm`，宿主负责文件系统、时钟、RGB565 绘制与 PCM 输出。浏览器端接入不包含在这条静态库构建命令中。


## 单机交互容量

`RA2_CONTROL_CAPACITY` 是独立核心的交互容量设置，默认 `16384`，同时决定屏幕点击／框选候选表和玩家命令队列容量，可框选并一次接收测试地图的 10000 条单位命令，必须是 `128` 到 `1048576` 之间的 2 的幂。已有构建目录会保留 CMake 缓存值，可显式更新：

```sh
cmake -S code -B out/resource-core -DRA2_CONTROL_CAPACITY=16384
cmake --build out/resource-core --parallel
```

同一个数字决定 `TacticalClass::SelectableObjects` 和 `OutList` 容量；`MegaMissionList` 为它的 2 倍，`DoList` 为 `max(16384, 8 × OutList)`。默认选择候选表容量为 `16384`，命令队列容量对应 `16384 / 131072 / 32768`（顺序为 `OutList / DoList / MegaMissionList`）。`RA2_YRPP_GAME` 分支始终保留原版 500 项候选表及 `128 / 16384 / 256` 命令队列的固定布局与地址；该设置只用于独立核心，不能改变原版可执行文件内的内存大小。容量变化后须重编译核心与宿主。原 `RA2_COMMAND_CAPACITY` 已更名，旧参数须改为 `RA2_CONTROL_CAPACITY`。

## 主循环与宿主调度

主循环入口按职责区分，不能把相似名称全部合并。行为核对使用的 `gamemd.exe` SHA-256 为 `7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6`。

| 原版职责 | 当前实现 | 边界 |
|---|---|---|
| `0x0055D360`：执行一次游戏迭代 | `core/src/yrpp/GameMainLoop.cpp` 的 `Game::MainLoop` | 唯一的生产游戏阶段编排 |
| `0x0055E160`：Sync_Delay | `core/src/game_loop.cpp` 的帧计时器 | 单机等待让出给宿主；网络等待中的输入、渲染未接入 |
| `0x0048CCC0`：外层游戏／菜单循环 | 宿主调度器只承接迭代驱动与停止 | 原版在 `0x0048CE8A` 调用 MainLoop，并通过结束处理函数决定退出；当前宿主并未复现完整菜单／结束流程 |
| 宿主单步／批量推进 | `advance_game_loop`／`advance_game_loop_batch` 共用一个内部驱动 | 单步至多一帧；批量仅在扩展速度下多帧；时间推进、暂停、异常处理共用 |
| 有绘制／无绘制入口 | `advance_game_view*`／`update_game_view*` | 共用视图绑定与调度；有绘制入口还共用后端校验、准备与结果处理 |

`Unsorted.h` 中两处 `MainLoop` 声明是互斥的 `RA2_YRPP_GAME` 条件分支：原版跳转与独立核心声明，须同时保留。宿主每次迭代经过 `run_original_iteration → Game::MainLoop`；输入、绘制、帧末钩子只提供宿主服务，不再另串联游戏逻辑。

`Game::MainLoop` 按原版 `0x0055D360` 战役／遭遇战分支的顺序自行执行各阶段：暂停计时；暂停时依次为 `Game::CallBack`、屏幕特效、Tactical 更新与渲染；否则为 `GScreenClass::GetInputAndUpdate` 与 `Game::KeyboardProcess`、`FoggedObjectClass::UpdateAll`、渲染、Ground 层增量排序、`LogicClass::Update`（原版在其中调用 Tactical 更新）、方向键滚屏、Planning 事件与消息列表、本地事件队列、`Game::CallBack`（音频 AI）、帧号递增与输入解锁计时，最后在帧末执行 `AbstractClass::RemoveAllInactive` 与 `PlanningManager_Update`。与原版一样，`Game::MainLoop` 返回 `bool`：游戏不再活动（`!Game::IsActive`），或本次迭代以胜负结束时返回 true，独立宿主调度器随即停止推进。宿主只通过 `GameLoopContext` 提供输入阶段的设备绑定（`input`）、绘制目标（`render`）、帧末记账（`frame_end`）和每次迭代一次的资源绑定（`bind`）；原版 `Sync_Delay` 的等待由宿主调度器（`game_loop.cpp`）承担。`LineTrail::UpdateAll` 原在 `TacticalClass::Render` 内，因绘制与模拟分离而在 `LogicClass::Update` 末尾执行，这是唯一调用点。

单机调度在 `Game::MainLoop` 的逻辑、消息阶段之后、帧号递增之前执行。核心 `EventClass` 负责 `OutList → DoList → MegaMissionList → Execute`、按所属玩家和帧号筛选、群体落点分配以及队列清理。地图包装层仅绑定各世界的原类队列并转发核心阶段，不判断事件是否可以执行、不替单位补入 Logic 列表。核心已移除仅测试使用的 `update_map_world`。完整循环通过宿主调度器和 `Game::MainLoop` 执行，以保持暂停、消息、排序、胜负与帧号顺序。

此处补齐的是战役／遭遇战普通命令路径；网络收发、同步校验、录像与退出控制分支仍未接入。满队列的失败语义沿用原版，不承诺无限缓冲。公共工程测试及其独立预期依据见 [测试说明](tests/README.md)。

## Godot 与 YRpp 声明检查

默认构建 Godot 扩展，并配置固定提交的 godot-cpp：

```sh
cmake -S code -B out/godot-native -DCMAKE_BUILD_TYPE=Debug
cmake --build out/godot-native --target ra2_godot --parallel
```

Godot/.NET 要求、运行和导出命令见 [宿主 README](godot/README.md)。

保留的 [clang-cl 工具链](cmake/clang-msvc-windows-x86.cmake) 与 [环境准备脚本](../scripts/setup-msvc-x86.sh) 可用于 Windows x86 的 YRpp 声明和调用约定检查。`BUILD_TESTING=ON` 且目标为 Windows x86 Microsoft ABI 时，`MsvcAbiTests.cmake` 提供布局、虚表和代码生成检查；它们不启动原版游戏。

## 构建范围与约束

源码由以下清单显式选择，不递归收集目录中的所有实现：

| 目标 | 源码清单 | 范围 |
|---|---|---|
| `ra2_core` / `ra2::core` | [Core.cmake](cmake/Core.cmake) | 独立核心、已移植原类算法及宿主服务；包含最小 Surface、ColorScheme/Convert/Blitter 对象及色表、Drawing 颜色操作、名称缓存和绘制请求，不链接 Godot |
| `ra2_render` / `ra2::render` | [Render.cmake](cmake/Render.cmake) | 独立 CPU 帧规划；启用 `RA2_BUILD_RENDER_DEVICE` 时包含 Device 渲染器与 SPIR-V |
| `ra2_software_render` / `ra2::software_render` | [SoftwareRender.cmake](cmake/SoftwareRender.cmake) | 可选软件光栅、SurfaceSHP、PCX 和 FileSystemPalette；链接 core，不链接宿主 |
| `ra2_godot` | [Godot.cmake](cmake/Godot.cmake) | GDExtension 与实验场景，链接核心、Threads 和 godot-cpp |

宿主通过 `api/filesystem.hpp` 管理资源句柄，并通过 `game::with_resources` 使用原类。句柄所有者负责串行访问，在销毁前停止并等待工作线程、销毁借用该资源的地图会话、结束图像使用并调用 `Unload_All_Shapes()`，然后销毁文件句柄。文件环境不隐式卸载 SHP；名称缓存位于基础 core，PCX 缓存属于可选组件，使用者须按实际所有权释放或失效化缓存；卸载 SHP 像素不等于销毁仍被借用的 SHPReference。`ReadWholeFile` 返回的缓冲使用 `YRMemory::Deallocate` 释放。需要内存实现的目标复用同一 `Memory.cpp`，用 CRT `malloc/calloc/realloc/free`；core 的 `Allocate`／`AllocateBytes` 执行 New／malloc 的失败恢复与重试，失败恢复通知、new-mode 和请求上限接口仍保留；calloc／realloc 外层策略继续调用 core 的单次操作。跨模块使用这些接口时，分配与释放必须属于匹配的运行时。

`Surface.h` 中的 `BSurface` 已支持独立构造、拥有或借用缓冲、1/2 字节像素、尺寸、Pitch、矩形及嵌套 Lock/Unlock。实现来自固定 EA WWLib 源码并用目标二进制校准；不包含完整线段/椭圆/设备绘制。SHP、MIX 与绘图缓冲的默认状态随原模块定义；`RA2_*_GAME` 分支仍保留，外部适配者负责提供该配置所需的数据地址绑定和服务。

目录中的类声明不代表所有方法均已实现。基础 core 保留 ColorScheme、LightConvertClass 及其 Blitter 的原类对象生命周期。宿主通过绘制接口同步读取对象选择的转换表，并在同一转换器的表更新后刷新缓存。`CC_Draw_Shape`、SurfaceSHP 和 PCX 归可选 `ra2_software_render`，**不作为基础 core 或 Godot 的强制依赖**；共享源码范围见 [SourceSets.cmake](cmake/SourceSets.cmake)。从仓库根目录启用：

```sh
cmake -S code -B out/software -DRA2_BUILD_GODOT=OFF -DRA2_BUILD_MOVIES=OFF -DRA2_BUILD_SOFTWARE_RENDER=ON -DBUILD_TESTING=ON
cmake --build out/software --parallel
ctest --test-dir out/software --output-on-failure --timeout 60
```

## 渲染失败诊断

宿主已配置运行日志时，帧准备或设备处理失败会在 `user://logs/sessions/<run>/` 写入
`render-failure-<id>.txt`，并向同一会话的 `events.log` 写入 `render_failure`、
`render_failure_origin`、`render_failure_values`、`render_failure_errors` 和快照保存结果。
界面错误包含失败编号与快照路径。没有日志目录或写入失败时，绘制失败状态与内存中的诊断仍保留。

快照包括构建、地图、world/generation、绘制帧、相机、画布、请求序号、校验条件、源位置和复制的请求参数。
对象绘制钩子补充原版对象 ID、类型、位置及 SHP 名称；背景事务记录失配的 instance、slot、范围及容量。
未验证的请求指针只记录带 `0x` 前缀的身份，不在诊断格式化时解引用；原对象删除后只读取已复制的元数据。

阶段字段使用 `0=未执行或未知`、`1=成功`、`2=失败`，分别记录 core、end_frame、enqueue、gpu_prepare、
gpu_submit、gpu_complete。GPU 阶段表示宿主设备准备和命令处理的结果；当前没有 GPU 完成回执，
`gpu_complete` 保持未知，不能把 CPU 提交成功当作 GPU 执行完成。渲染线程使用随帧排队的地图、帧号及提交序号，
不回查可能已销毁的世界。

`api/drawing_diagnostics.hpp` 的固定容量记录只保存本次操作的首个失败，查询和输入清理不会清空它。
只有新的绘制操作显式重置诊断；`with_map_view` 只判断上下文绑定及回调执行，不能将其成功返回当作绘制成功。
`unsupported`/`unavailable` 的后端能力协商不会直接成为失败记录，只有最终回退仍失败时才记录。

## 帧与宿主边界

Presentation 将原类的同步绘制请求复制为拥有资源寿命的 `render::Frame`，公开宿主接口使用不透明帧句柄和 POD 视图。Frame 封存后不可修改；异步宿主必须保留帧句柄，直到消费结束。原类请求适配器只存在于 core 私有实现，宿主不再实现逐类绘制回调。

`ra2_render` 通过 `render::Device` 的无异常 POD 函数表创建和使用 GPU 资源。Godot 桥接提供 Device、渲染线程队列及 `Texture2DRD` 发布；帧规划、页面缓存和 GPU pass 调度位于独立 render 库。`RA2_BUILD_GODOT=OFF -DRA2_BUILD_RENDER_DEVICE=ON` 可以构建完整渲染器，不链接 Godot；默认的纯 core 构建仅包含 CPU 帧规划，不要求着色器工具。

可选软件参考通过 `create_presentation(..., true)` 记录拥有 SHP/TMP、转换表和像素存储的参考帧，再调用 `draw_software_frame`。此模式支持 shape、tile、raster、indexed，请求方对其他功能使用已有软件回退；它不消费 GPU 常驻场景或体素任务。生产 Godot 不链接该软件组件。

设备渲染构建默认通过 [third_party/glslang](third_party/glslang/README.md) 准备固定版本的宿主 shader 编译器，无需本机安装 glslang。其下载和编译结果保留在构建目录；只构建 CPU 帧规划时不触发该依赖。
