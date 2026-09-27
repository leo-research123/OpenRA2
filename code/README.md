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
| `bridge/shaders` | GPU 绘制着色器唯一源码；宿主构建生成 Godot 项目内的资源副本 |
| `experiments`、`third_party` | 实验代码与固定版本的外部依赖 |

核心只公开 `core/include`，调用方使用 `api/...`、`yrpp/...`。公开头与 Godot 桥接不能依赖核心内部头；测试访问内部头时按目标显式配置。目录依赖由 [CheckYrppLayout.cmake](cmake/CheckYrppLayout.cmake) 检查；[CheckCoreOrganization.cmake](cmake/CheckCoreOrganization.cmake) 补充原模块归属、公开头清单、文件关停与链接依赖检查。

YRpp 头须独立于 `game::`；新增/扩展的公开 ABI 使用基础类型、原版已有类型、指针与长度或有类型的不透明句柄，不能暴露 STL 对象，项目自有 C++ 代码不能使用 `std::optional`。

项目版本由核心持有：`RA2_PROJECT_VERSION` 仅作为 `ra2_core` 的私有编译定义，调用方通过 `api/version.hpp` 的 `game::project_version` 读取。Godot 的 `RA2Core.get_version()` 转发该核心版本，C# 宿主继续通过桥接读取。

## 独立构建

```sh
cmake -S code -B out/resource-core -DRA2_BUILD_GODOT=OFF -DBUILD_TESTING=ON
cmake --build out/resource-core --parallel
ctest --test-dir out/resource-core --output-on-failure --timeout 60
```

以上命令从仓库根目录执行。`RA2_BUILD_GODOT` 默认关闭，此路径不下载 godot-cpp，也不需要 Godot、.NET 或原版素材。真实素材用例另需 `RA2_GAME_DATA`，缺省时相关子用例跳过。

只构建静态库时设置 `BUILD_TESTING=OFF`，并构建 `--target ra2_core`。多配置生成器需为构建指定 `--config Debug`，为测试指定 `-C Debug`。

其他 CMake 工程可通过 `add_subdirectory` 引入本目录，关闭 `RA2_BUILD_GODOT` 和不需要的测试，再链接 `ra2::core`。


## Godot 与 YRpp 声明检查

Godot 需显式启用，此时才配置固定提交的 godot-cpp：

```sh
cmake -S code -B out/godot-native -DRA2_BUILD_GODOT=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build out/godot-native --target ra2_godot --parallel
```

Godot/.NET 要求、运行和导出命令见 [宿主 README](godot/README.md)。

保留的 [clang-cl 工具链](cmake/clang-msvc-windows-x86.cmake) 与 [环境准备脚本](../scripts/setup-msvc-x86.sh) 可用于 Windows x86 的 YRpp 声明和调用约定检查。`BUILD_TESTING=ON` 且目标为 Windows x86 Microsoft ABI 时，`MsvcAbiTests.cmake` 提供布局、虚表和代码生成检查；它们不启动原版游戏。

## 构建范围与约束

源码由以下清单显式选择，不递归收集目录中的所有实现：

| 目标 | 源码清单 | 范围 |
|---|---|---|
| `ra2_core` / `ra2::core` | [Core.cmake](cmake/Core.cmake) | 独立核心、已移植原类算法及宿主服务；包含最小 Surface、色表、名称缓存和绘制请求，不链接 Godot |
| `ra2_software_render` / `ra2::software_render` | [SoftwareRender.cmake](cmake/SoftwareRender.cmake) | 可选软件光栅、Convert/Blitter、Drawing、SurfaceSHP、PCX 和 FileSystemPalette；链接 core，不链接宿主 |
| `ra2_godot` | [Godot.cmake](cmake/Godot.cmake) | GDExtension 与实验场景，链接核心、Threads 和 godot-cpp |

宿主通过 `api/filesystem.hpp` 管理资源句柄，并通过 `game::with_resources` 使用原类。句柄所有者负责串行访问，在销毁前停止并等待工作线程、销毁借用该资源的地图会话、结束图像使用并调用 `Unload_All_Shapes()`，然后销毁文件句柄。文件环境不隐式卸载 SHP；名称缓存位于基础 core，PCX 缓存属于可选组件，使用者须按实际所有权释放或失效化缓存；卸载 SHP 像素不等于销毁仍被借用的 SHPReference。`ReadWholeFile` 返回的缓冲使用 `YRMemory::Deallocate` 释放。需要内存实现的目标复用同一 `Memory.cpp`，用 CRT `malloc/calloc/realloc/free`；core 的 `Allocate`／`AllocateBytes` 执行 New／malloc 的失败恢复与重试，失败恢复通知、new-mode 和请求上限接口仍保留；calloc／realloc 外层策略继续调用 core 的单次操作。跨模块使用这些接口时，分配与释放必须属于匹配的运行时。

`Surface.h` 中的 `BSurface` 已支持独立构造、拥有或借用缓冲、1/2 字节像素、尺寸、Pitch、矩形及嵌套 Lock/Unlock。实现来自固定 EA WWLib 源码并用目标二进制校准；不包含完整线段/椭圆/设备绘制。SHP、MIX 与绘图缓冲的默认状态随原模块定义；`RA2_*_GAME` 分支仍保留，外部适配者负责提供该配置所需的数据地址绑定和服务。

目录中的类声明不代表所有方法均已实现。`CC_Draw_Shape`、Convert/Blitter 的软件像素路径、Drawing 颜色操作和 PCX 归可选 `ra2_software_render`，**不作为基础 core 或 Godot 的强制依赖**；共享源码范围见 [SourceSets.cmake](cmake/SourceSets.cmake)。从仓库根目录启用：

```sh
cmake -S code -B out/software -DRA2_BUILD_GODOT=OFF -DRA2_BUILD_SOFTWARE_RENDER=ON -DBUILD_TESTING=ON
cmake --build out/software --parallel
ctest --test-dir out/software --output-on-failure --timeout 60
```
