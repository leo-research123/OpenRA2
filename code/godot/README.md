# ra2opengodot

游戏内 Godot 承载一张画布、原始输入和设备服务，通过 GDExtension 加载独立 C++ 核心；布局、控件、命中、滚动与雷达绘制由原类决定。独立构建见 [核心 README](../README.md)。

- `../core/`：C++ 核心源码，不依赖 Godot；独立构建仅包含已支持的实现，图片等原版专用实现另列目标。
- `../bridge/`：可选的 GDExtension 适配模块；头文件位于 `include/bridge/`，实现位于 `src/`。
- `scripts/App.cs`：启动与页面导航。
- `scripts/display/`：显示配置与窗口控制。
- `scenes/`、`scripts/ui/`、`ui/`：界面、交互与样式。
- `tests/`：核心加载、菜单流程和显示设置验证。

test-ball 万球压力测试验证 C++ 更新运动与颜色、直接通过 RenderingServer 绘制的链路，并记录帧间隔与提交耗时；入口为 `scenes/test-ball.tscn`，无需原版资源。

test-ball-godot 是相同负载的纯 C# 对照，入口为 `scenes/test-ball-godot.tscn`；专用导出不包含项目 C++ 扩展。

test1 万人行走测试在空背景上以 1 倍显示 10,000 个真实 GI，随机移动、边缘反弹并独立播放八方向行走动画，入口为 `scenes/test1.tscn`。Godot 场景、测试运动、图集和 GPU 显示均在 `../bridge`，核心提供资源与帧数据接口。


## 构建与运行

以下是完整 Godot 宿主的构建要求。仅构建 C++ 核心无需 Godot 或 .NET，见 [native 构建契约](../README.md)。全新构建目录默认不启用 Godot，以下命令显式传入 `RA2_BUILD_GODOT=ON`；已有构建目录保留其缓存选项。

需要 CMake 3.24+、支持 C++20 的编译器、Godot 4.7.2 .NET 标准精度版及 .NET SDK（当前验证为 10.0.401）。C# 工程的桌面目标框架为 `net8.0`，并安装 .NET 8 运行时供编辑器工具使用。项目自有运行脚本和 Godot 测试均使用 C#，通过 GDExtension 调用 C++ 核心；第三方 `addons/godot_mcp/` 保留其 GDScript 实现。以下命令从仓库根目录执行。首次构建会下载固定提交的 godot-cpp；原生库输出到本项目 `bin/`，不加入 Git。

```sh
cmake -S code -B out/godot-native -DRA2_BUILD_GODOT=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build out/godot-native --parallel
dotnet build code/godot/ra2opengodot.csproj
godot --editor --import --quit --path code/godot --rendering-method mobile
godot --path code/godot
```

游戏内按物理键盘的 `` ` `` 键打开半透明控制台，按 Enter 执行，按 `` ` `` 或 Esc 关闭。`power` 为当前玩家在本局增加 1000000 电力；`level 0`、`level 1`、`level 2` 将选中的步兵、载具或飞机分别设为新兵、老兵、精英；`life 数字` 将选中单位的生命设为 0 到 1000000 之间的数值（0 会使单位死亡）。多选时对所有选中单位生效，地图切换后电力加成清除。

控制台回归：`python3 code/godot/tests/run_console_tests.py --game-data=/absolute/path/to/RA2`。

着色器唯一源码在 [`../bridge/shaders`](../bridge/shaders/README.md)。CMake 的 `ra2_godot` 构建和 `dotnet build` 都会将其生成到本项目 `shaders/`；该目录及导入元数据由 Git 忽略，不直接编辑。仅刷新资源可执行 `cmake --build out/godot-native --target ra2_godot_shaders`。

修改 bridge 中的 GLSL 后，先运行上述任一构建，再在启动或导出前执行上面的 Godot 导入命令。运行时加载 `.godot/imported` 中的 `RDShaderFile`；仅生成资源或编译 C++ 不会更新已导入的 SPIR-V。导入使用真实渲染设备，不能用无渲染设备的 headless 导入代替着色器编译。

用 .NET 编辑器创建脚本时选择 C#；`.csproj` 和 `.sln` 加入版本控制，生成文件位于已忽略的 `.godot/mono/`。可在编辑器设置的 `Dotnet → Editor → External Editor` 中选择 Rider。导出前安装与编辑器匹配的 **4.7.2 .NET 导出模板**；macOS 预设使用官方 Universal 模板，输出到 `out/release/macos-universal/`，不指定自定义模板。项目同时启用两种架构所需的纹理导入格式，发布时需包含 arm64 和 x86_64 的 C++ 原生库。

资源目录优先级：命令行 `--game-data=` → 场景 `GameDataPath` 属性 → 环境变量 `RA2_GAME_DATA` → 保存的 `user://resources.cfg`。都未指定时，编辑器版 Godot 使用仓库内 `out/reference/RA2MDddcompact`；导出应用尝试可执行文件所在目录。没有挂载任何资源包时，主界面会提示选择游戏资源目录并禁用“开始”；选择成功后自动记住，下次双击应用无需命令行参数。也可通过以下方式指定安装：

```sh
godot --path code/godot -- --game-data=/absolute/path/to/RA2
RA2_GAME_DATA=/absolute/path/to/RA2 godot --path code/godot
```

参数中的相对目录以 Godot 项目目录为基准；也可使用 `res://`、`user://` 路径。MIX 保持原样放在安装目录内，不需要解包。没有原版资源时，使用下方合成资源测试即可验证加载流程。

点击主菜单“开始”时，如果传入了非空的 `--map=文件名`，会直接加载该地图；否则显示地图选择页。例如：

```sh
godot --path code/godot -- --game-data=/absolute/path/to/RA2 --map=SOV02SMD.MAP
```

选择页复用原类地图挂包与 INI 读取，解析 `battle(md).ini`、`mission(md).ini`、`missions(md).pkt` 和资源目录内的其他 `.pkt`，并扫描该目录的 `.map`、`.mpr`、`.yrm` 散文件。文件名忽略大小写去重，只列出能读取且包含有效地图尺寸的候选项；完整地形、对象及依赖资源仍在选定后的正常载图流程中校验。顶部下拉框提供“战役地图”“非战役地图”“所有地图”，每次进入默认选择“战役地图”；分类与核心载图一致，按 `[Basic] MultiplayerOnly` 判断（为真时属于非战役，缺省为假）。切换分类即时筛选已发现的地图并清除旧选择。列表支持键盘选择、双击加载、“加载地图”按钮，以及 Esc／“返回”；未找到地图时提示检查资源目录。每次打开选择页重新扫描，扫描期间可返回菜单。

导出 Release 原生库：

```sh
cmake -S code -B out/godot-native-release -DRA2_BUILD_GODOT=ON -DCMAKE_BUILD_TYPE=Release
cmake --build out/godot-native-release --parallel
```

在 Apple Silicon Mac 上发布 Universal 包时，另行构建 Intel 原生库，再使用官方模板导出：

```sh
cmake -S code -B out/godot-native-release-x86_64 -DRA2_BUILD_GODOT=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Darwin -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build out/godot-native-release-x86_64 --target ra2_godot --parallel
mkdir -p out/release/macos-universal
godot --editor --import --quit --path code/godot --rendering-method mobile
godot --headless --path code/godot --export-release "macOS Universal"
```

## 验证

开发版和导出应用均支持脚本化运行，不依赖桌面点击。启动、资源目录选择、无参数重启、载图/切图、暂停和分辨率回归：

```sh
python3 code/godot/tests/run_startup_tests.py \
  --app out/game-ui-implementation/export/RedAlert2Open.app \
  --game-data out/reference/RA2MDddcompact \
  --output out/godot-startup-check
```

省略 `--app` 可测开发版。

鼠标与图形往返回归使用 `tests/run_camera_tests.py`，参数同上；雷达回归使用 `tests/run_radar_tests.py`，也支持导出应用；最新导出位于 `out/game-ui-implementation/export/RedAlert2Open.app`。

纯核心构建不下载 Godot、不链接 GDExtension，也不依赖原版素材：

```sh
cmake -S code -B out/resource-core -DRA2_BUILD_GODOT=OFF
cmake --build out/resource-core --parallel
ctest --test-dir out/resource-core --output-on-failure
```

编译 Debug 原生库后，验证 Godot 加载进度、菜单、设置、暂停、缺失核心与资源错误。运行器会编译 C# 并导入项目，再启动 `tests/TestApp.tscn`；缺核心用例在不含 GDExtension 的临时工程中运行同一程序集：

```sh
python3 code/godot/tests/run_tests.py --godot /path/to/godot
```

脚本生成临时合成 MIX，不修改原版素材。`TestResourceWorker` 直接验证 RA2Core 的单资源环境、取消、重启和活动任务销毁。C# 测试只编译进 Debug 程序集，导出配置会排除测试代码和测试场景。可追加 `--game-data out/reference/RA2MDddcompact`，同时验证本地原版资源；原生挂载来源报告：

```sh
out/resource-core/ra2_resource_tests --reference out/reference/RA2MDddcompact
```

有窗口的加载页验证和截图（输出目录需提前创建）：

```sh
mkdir -p out/godot-bootstrap
dotnet build code/godot/ra2opengodot.csproj
godot --path code/godot res://tests/TestResourceWindow.tscn -- --capture-dir=/absolute/path/to/out/godot-bootstrap
```

验证实际窗口模式切换、分辨率、设置页与退出按钮：

```sh
godot --path code/godot res://tests/TestDisplayWindow.tscn -- --capture-dir=/absolute/path/to/out/godot-bootstrap
```
