# OpenRA2

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

test-ball（C++ 版）和 test1 使用的 `RA2TestBall`、`RA2TestInfantry` 属于实验类：生产扩展默认不编译也不注册它们，只有以 `-DRA2_BRIDGE_EXPERIMENTS=ON` 配置的构建才包含，上述两类入口场景也只能在这样的构建中运行。


## 构建与运行

战役侧栏影片使用核心的可选解码模块，Godot 构建默认启用 `RA2_BUILD_MOVIES`。配置需要 pkg-config 和 FFmpeg 7 开发库（libavformat/libavcodec 61、libavutil 59、libswscale 8、libswresample 5；已验证 7.1.3）。macOS 可安装 `ffmpeg@7` 并设置 `PKG_CONFIG_PATH="$(brew --prefix ffmpeg@7)/lib/pkgconfig"`。运行时仍需对应动态库；当前构建没有把这些库打包进导出产物，分发时须另行提供其依赖及许可。库的最低系统版本也须与部署目标匹配。仅构建核心可保持 `RA2_BUILD_MOVIES=OFF`，无需 FFmpeg。

以下是完整 Godot 宿主的构建要求。仅构建 C++ 核心无需 Godot 或 .NET，见 [native 构建契约](../README.md)。全新构建目录默认启用 `RA2_BUILD_GODOT` 和 `RA2_BUILD_MOVIES`；已有构建目录保留其缓存选项，修改默认值不会覆盖已有的显式关闭设置。

需要 CMake 3.24+、支持 C++20 的编译器、Godot 4.7.2 .NET 标准精度版及 .NET SDK（当前验证为 10.0.401）。C# 工程的桌面目标框架为 `net8.0`，并安装 .NET 8 运行时供编辑器工具使用。项目自有运行脚本和 Godot 测试均使用 C#，通过 GDExtension 调用 C++ 核心；第三方 `addons/godot_mcp/` 保留其 GDScript 实现。以下命令从仓库根目录执行。ICU 由 [`../third_party/icu`](../third_party/icu/README.md) 统一管理：首次配置自动下载并校验固定的 ICU 78.3 源码，首次构建编译静态库，之后复用构建缓存，无需安装 ICU 或指定 `ICU_ROOT`。ICU 与主工程使用同一编译器、架构、macOS 最低系统版本和 Windows CRT；仅构建核心无需 ICU。离线或使用已有静态库的入口见该目录说明。ICU 静态链接进扩展，运行成品无需另装 ICU；许可随导出包含在 `third_party/icu/LICENSE.txt`。首次构建会下载固定提交的 godot-cpp；原生库在构建目录链接，成功后发布到仓库的 `out/debug/bin/` 或 `out/release/bin/`，不加入 Git。

```sh
cmake -S code -B out/godot-native -DCMAKE_BUILD_TYPE=Debug
cmake --build out/godot-native --parallel
dotnet build code/godot/OpenRA2.csproj
godot --editor --import --quit --path code/godot --rendering-method mobile
godot --path code/godot
```

进入遭遇战：等待资源加载完成，在主菜单点击 **遭遇战**，选择地图、国家和至少一名电脑对手，再点击 **开始游戏**。玩家颜色和出生点可指定或随机；资金、初始部队、AI 难度、队伍及复选项会传入核心的原版 Session 配置。当前开放普通 **作战** 模式，尚未接通的特殊模式会显示为不可选。玩家数不能超过地图出生点数，也不能让所有玩家加入同一队。地图加载失败时按 Esc 返回大厅，保留设置后可重试。

战役难度：主菜单点击 **开始**，选择战役地图后，在列表下方选择 **简单／普通／困难**，然后加载地图或双击地图。默认普通，返回列表保留本次选择；难度用于原版场景、阵营加成及任务触发条件，并在本局重试、下一关中保持。选择非战役或测试地图时不显示此选项。

修改原生库后请退出正在运行的游戏并重新启动；GDExtension 未启用热重载。

游戏内按物理键盘的 `` ` `` 键打开半透明控制台，按 Enter 执行，按 `` ` `` 或 Esc 关闭。打开控制台不暂停游戏，`help` 显示当前注册命令，`clear` 清屏。

| 命令 | 行为 |
| --- | --- |
| `power` | 当前玩家固定增加 1000000 电力，重复执行不叠加。 |
| `fog [0\|1]` | 无参数切换迷雾显示，0 隐藏，1 恢复；保留真实探索和 AI 视野。 |
| `radar [0\|1]` | 无参数切换强制雷达，1 无需建筑或电力即可开启雷达地图，0 恢复正常规则；不改变探索状态，可配合 `fog 0` 使用，换地图重置。 |
| `money [数字]` | 给当前玩家加钱，默认 10000；只接受正整数，溢出时拒绝。 |
| `tp [0\|1]` | 无参数切换持续瞬移模式。之后的移动点击将当时选中且可控制的单位立即传送；多选沿用原版群体移动的目标格分配（与超时空兵相同），在点击点附近安排落点和步兵站位，不保留远处单位的完整队形。建筑不传送。 |
| `level <0..2>` | 选中对象（含建筑）设为新兵、老兵、精英。 |
| `hp <0..1000000>` | 设置选中对象生命，包含建筑和有生命的地形；0 使用调试移除流程。`life` 已更名为 `hp`。 |
| `xp <数字>` | 设置原版绝对经验，支持小数；1 为老兵，2 为精英，上限为当前规则 `VeteranCap`。包含建筑，不改变类型是否可训练。 |
| `atk <1..1000000>` / `atk reset` | 设置每个选中作战对象的攻击伤害，包含建筑。在攻击者加成之后、目标防御之前覆盖，不修改共用武器；reset 恢复原有计算。治疗和特殊功能弹头保持原行为。 |
| `god` | 选中作战对象设为精英，生命和攻击均为 1000000；仍会受到伤害。 |
| `gamespeed <1..9>` | 1–6 使用已有定时档位，7/8 为 120/240 逻辑 FPS，9 不限速；只修改本局，不自动保存设置。 |
| `select <1..3>` | 1 默认，2 可选其他阵营非中立单位，3 包含中立单位；不框选建筑。 |
| `control <1..3>` | 1 默认，2 可控制其他阵营非中立对象，3 包含中立对象；与选择权限独立，不改变对象归属。 |
| `perf [0\|1]` | 无参数切换左上角半透明性能面板，0 关闭，1 开启。小字号显示最近两秒的画面/逻辑 FPS、CPU 平均/P95 耗时、游戏速度、逻辑预算受限比例及单位/建筑数量，每秒刷新一次，鼠标操作穿透。绘制和提交统计为 CPU 耗时。 |

属性命令对所有适用的选中对象生效。调试模式、属性攻击覆盖和电力加成在地图切换后清除。瞬移取消旧路径及移动状态：不适应水面的地面载具和步兵落海后逐渐沉没，步兵不生成陆地尸体；桥面和能够游泳的步兵保持原有行为。被占用的落点会尝试附近可用位置；越界、附近无空位或处于运输、部署等不适合直接传送的状态时会跳过并反馈。

控制台的游戏命令通过独立核心 [`api/debug_commands.hpp`](../core/include/api/debug_commands.hpp) 查询并同步执行。宿主 [`GameConsole.cs`](scripts/ui/GameConsole.cs) 管理输入、历史、`help` 和 `clear`，从运行时注册表中的命令描述生成帮助；每条游戏命令只有一次桥接执行调用，不向 C# 导出全地图对象用于筛选。每条命令在 `core/src/debug/commands/Debug<Name>Command.cpp` 内独立实现描述、参数解析和完整执行逻辑，并使用 `GAME_REGISTER_DEBUG_COMMAND` 自注册。CMake 只自动发现这个限定目录的源文件，并将对象文件传播到最终链接，确保静态库及链接器清理未引用代码时仍保留注册对象；不生成或维护命令表。新增或删除文件后普通构建自动重新配置；运行中的程序遍历已经注册的命令对象，不读取 C++ 源文件。

新增调试命令只需增加命令文件及对应测试，无需修改集中表、公共枚举、解析器、执行器、桥接或 C# 执行分支。实现内部的 `DebugConsoleCommand` 提供 `descriptor()`、`parse()`、`execute()`：

- `descriptor()` 返回该命令自己的静态 POD 描述及英文显示后备文本。新增 ID 在本文件内通过 `static_cast<DebugCommandId>(0x...)` 声明，非零且全局唯一；既有 `power`、`level`、`hp`（原 `life`）的 ID 保持不变。名称使用小写 ASCII 字母、数字、`_`、`-` 或 `.`；`help`、`clear` 保留给宿主。
- `parse()` 使用 `DebugArgumentReader` 按本命令的参数顺序读取整数、实数、布尔或文本，填入请求自有的 `DebugCommandArguments`。每次最多 16 个参数，输入总长仍为 512 字节；数字保留前导 `+` 及严格范围校验，实数使用独立 C 数字区域设置，布尔接受 `true`、`false`、`0`、`1`，文本支持双引号及 `\"`、`\\` 转义。公共解析器负责检查余下参数。描述中的 `argument`、`minimum`、`maximum` 仅保留旧单整数命令的摘要；多参数命令通过 `usage` 展示完整用法，实际规则由自己的 `parse()` 决定。
- `execute()` 获得一次地图作用域内的 `DebugCommandContext`，负责完整操作及明确状态。直接修改前设置 `writes_started`；共享 `execute_selected_debug_objects()` 负责目标快照、过滤、失效检查、计数及异常目标记录。纯查询允许成功返回且修改数量为零。已有原版玩家操作应复用原版接口，不修改 `CommandClass` 的虚表或输入契约。

命令对象和注册节点具有静态生命周期，注册不分配内存；生产注册在启动时完成。注册表按数值 ID 枚举，按名称查找，不依赖不同文件的静态初始化顺序。重复 ID、重复名称或非法描述会拒绝整个目录并记录 `command_registry_failure`，不会任选一条执行。中文翻译仍可单独补充，未知命令的描述和反馈使用英文后备文本。

命令绑定当前地图的 world 标识；切换或重启后拒绝旧会话请求。核心在同一次地图访问中固定目标，依据原对象 `IsSelected` 标志和命令自己的类型过滤器筛选。非破坏性修改直接使用该次访问的原对象指针，`hp 0` 在每次操作前重新解析 ID，以处理失效回调移除后续目标。`hp 0` 使用原有调试失效通知与退出地图流程，建筑额外释放地基、生产和归属状态，不产生正常战斗死亡效果，不能复活已死亡对象。

核心返回明确状态与结构化结果：请求号、world、命令、执行帧、参数、候选／成功／跳过数量及首个失败目标和原因。既有描述及结果 ABI 布局保留；实数和多行反馈通过请求号绑定的独立 POD 反馈接口获取。准备失败不修改游戏状态；执行失败不会回滚，`applied` 只计已完成的操作，失败目标自身可能已有部分副作用。批量操作中存在跳过目标时返回 `partial_success`，没有适用目标时返回 `no_targets`。新增公开数据仅使用固定底层类型、原有 `MapObjectId` 和固定字符缓冲，异常不跨核心接口；原类回调既有的终止失败语义保留。

运行日志 `user://logs/runtime.log` 中，`command_begin`／`command_end` 使用相同请求号记录尝试及结果，必要时增加首个目标失败记录。自动化读取 `console_result` 的状态和计数，不依赖中文反馈或控制台内部节点路径。

无需 Godot 或真实素材的核心回归：

```sh
cmake --build out/resource-core --target ra2_debug_commands_tests --parallel
ctest --test-dir out/resource-core -R '^(debug_commands|debug_command_discovery)$' --output-on-failure
```

`debug_command_discovery` 在独立临时工程中只增加／删除命令文件，再进行普通构建，验证文件自动纳入、运行时注册，以及启用链接器未引用代码清理后仍能通过普通静态库消费者查找和解析命令。

设置页的“游戏速度”从 1 到 9 递增：1～6 是原版最慢至较快，5 为默认；7、8 分别以每秒 120、240 个逻辑帧为目标；9 不设目标帧率。原版内部的反向等待值仅由核心转换。实际速度取决于机器负载，高速模式逐帧运行游戏逻辑，每次宿主刷新最多提交一幅画面；切换到后台后游戏继续运行，失焦仅清理鼠标和按键输入并保持既有后台静音策略。按 Esc 打开菜单后暂停，从菜单进入设置页也保持暂停，切出或切回窗口不会解除暂停；关闭菜单后恢复且不积累补帧。战役与遭遇战均可在暂停菜单中修改，点击“应用”保存到 `user://game.cfg`，返回放弃未应用修改。测试时可用 `--game-config=/absolute/path/to/game.cfg` 隔离配置。

着色器源码在 [`../render/shaders`](../render/shaders/README.md)，按 `.comp`、`.vert`、`.frag` 阶段维护。CMake 默认通过 [`third_party/glslang`](../third_party/glslang/README.md) 下载固定版本源码并自动构建宿主工具，再编译 SPIR-V 并嵌入原生库，无需系统预装。交叉编译也使用独立构建的宿主可执行文件。只有显式选择外部工具时才需设置 `-DRA2_GLSLANG_EXECUTABLE=/path/to/glslang`；旧缓存可用 `-U RA2_GLSLANG_EXECUTABLE` 清除。修改 C++ 或 GLSL 后构建 `ra2_godot`，`dotnet build` 不更新原生渲染器。

运行时只使用 `RDShaderSPIRV` 和 `shader_create_from_spirv`，无需 Godot 导入地图绘制 shader。普通图片、场景与 C# 工程仍按常规 Godot 流程导入。`RA2_GODOT_PROJECT` 和 `RA2_GODOT_OUTPUT_ROOT` 分别指定附带地图等资源的工程目录与库发布根目录；原生库先在构建目录完成链接，再发布到运行目录。

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

地图选择页保留全部战役、非战役、所有地图和测试地图，并提供“标准版”和“尤里的复仇”的全部战役及盟军／苏军战役分类，还有“其他战役”。内置文件名索引明确记录四类官方任务：标准版盟军 12 张、标准版苏军 12 张、尤里的复仇盟军 7 张、尤里的复仇苏军 7 张，忽略大小写精确匹配。安装目录中的任务索引仅用于发现候选地图和读取名称，混合索引不影响分类；不在四类索引中的战役（包括教学任务）统一归入“其他战役”。详情显示分类；地图仍按现有核心流程加载。

任务完成或失败后，关闭结果提示会返回任务选择页。同次运行中保留上次的分类、选中地图和列表位置，从主菜单重新进入也保留；重新加载游戏资源时清理旧地图列表并重新扫描，但不重置分类。

任务名称由原生 C++ 使用 ICU 解码，C# 只读取 Unicode 文本。每个 INI 从已解析的旧编码字段提取文本样本（排除 ASCII 压缩数据与 UTF-8 字段），识别一次编码，支持 GBK/GB18030、Big5、Shift-JIS、Windows-1251 等 ICU 编码；UTF-8 编辑字段可与旧编码并存。检测置信度低于 30 时，保留中文资源的 GB18030 默认编码；自动识别无法保证短文本或单文件混用多个旧编码时准确。非法字节转换失败时返回缺省值，避免将其传入 Godot 的 UTF-8 解析器。松散 `missionmd.ini` 覆盖包内索引且遗漏原有任务时，内置索引中的官方地图仍可回退到 CSF 中的任务名称；已提供的自定义名称保持优先。

主菜单“快速测试”加载内置的盟军、苏军陆军测试地图。盟军在左、苏军在右，两套基地按相同顺序分行：建造厂与作战实验室、矿场与雷达、战车工厂与兵营、两座电厂；盟军雷达使用空指部。全部建筑归玩家控制，中央留出通道，工厂出口保持畅通。拥有 100 万资金及足够电力，可以直接从原侧栏生产 GI、工程师、灰熊坦克、IFV，以及动员兵、防空步兵、犀牛坦克和防空履带车；双方作战实验室已放好，也可直接生产光棱坦克和天启坦克。地图内按阵营匹配兵营和战车工厂，双方单位共用玩家的生产队列；苏军工程师等国家限制仍保留。镜头对准基地，地图南方有静止苏军靶标；没有剧情触发器或定时进攻。初始不放矿车，也不依赖采矿补充资金，科技前置、扣款和生产队列使用正常规则。

玩家基地右侧另放有一座核电站（`NANRCT`）；建筑下方并排放置两座光棱塔（`ATESLA`）和两座磁暴线圈（`TESLA`），同类两座靠近摆放，再往下依次是步兵和载具阵列。新增建筑均归玩家控制，避开工厂出口。

使用 `--quick-test` 可在资源加载成功后自动进入，无需点击菜单；与 `--map=` 同时提供时优先快速测试。首次缺少原版素材时仍会提示选择资源目录，选择成功后自动进入。按 Esc 打开暂停菜单，点击“重启测试”恢复初始基地和资金，并清除生产队列、生成单位及本局控制台加成。返回主菜单后可再次进入；普通地图不显示“重启测试”。

```sh
godot --path code/godot -- --game-data=/absolute/path/to/RA2 --quick-test
```

地图唯一源码为 [`../data/maps/quick_test.map`](../data/maps/quick_test.map)。CMake 宿主构建和 `dotnet build` 会复制到已忽略的 `maps/`；修改地图后运行任一构建。导出预设包含 `maps/*.map`，应用将内置地图按内容版本释放到 `user://maps/`，再交给原生文件服务读取，无需修改原版资源目录。此入口复用单机场景加载流程；普通遭遇战请使用主菜单的遭遇战大厅。

“开始”→“测试地图”提供快速测试地图和“万人美国大兵测试地图”。后者为 256×256 温带平地，中央紧密排列 10,000 个真实 GI：填满相邻的交错格行，每格使用三个正常步兵位置，共占 3,334 格。全部归红色美国玩家控制，初始满血、静止。画面左下角有普通盟军建造厂、电厂、兵营和空指部，基地旁另有一座核电站和一座间谍卫星，开局镜头对准基地，资金为 10,000；可通过雷达点击中央查看步兵阵列。内置测试地图都支持暂停菜单中的“重启测试”，恢复初始对象和资金。

万人地图源码为 [`../data/maps/gi_10000.map`](../data/maps/gi_10000.map)，由同目录的 `generate_gi_10000.py` 确定性生成。修改布局时运行 `python3 code/data/maps/generate_gi_10000.py`，使用 `--check` 检查生成文件是否同步。此地图使用现有地图容量和普通对象加载流程；静止单位的显示负载与下达移动命令后的逻辑、寻路负载不同。

“测试地图”另提供“2000 犀牛独立小循环测试地图”：256×256 温带平地上预放 2,000 台玩家所属的犀牛坦克（`HTNK`），排列为屏幕横向 50 列、纵向 40 行的等距实心方阵。每台坦克只在自身初始位置旁独立走一格见方的小循环，全部以相同相位开始。开局触发器为每台坦克创建单成员原版小队，按原版移动到格子和跳转脚本循环，无需框选下令。坦克保持正常速度、碰撞和寻路规则；拐角可能出现让行和短暂停顿，此负载还包括 2,000 个小队的脚本调度。画面左下角放置核电站（`NANRCT`）和间谍卫星（`GASPYSAT`），使用正常电力和全图揭示机制；初始镜头对准方阵。暂停菜单“重启测试”会恢复全部坦克和循环脚本。

犀牛地图源码为 [`../data/maps/rhino_2000.map`](../data/maps/rhino_2000.map)，生成器为同目录的 `generate_rhino_2000.py`。运行 `python3 code/data/maps/generate_rhino_2000.py` 重新生成，追加 `--check` 验证同步。地图随 CMake 宿主构建或 `dotnet build` 打包，也可直接通过 `--map=/absolute/path/to/rhino_2000.map` 加载。

“测试地图”还提供“VERA20k 2 万犀牛／30 阵营”的待命版和混战版。两张地图均为 250×250 温带地形，初始放置 20,000 台犀牛坦克，玩家控制 P01 的 667 台，其余 29 个阵营由电脑控制，无基地、生产和经济。待命版使用 Guard 任务，可手动下令；混战版使用 Hunt 任务自动交战，单位阵亡后数量会下降，大规模索敌可能造成明显卡顿。这是本地场景测试，地图选择时归入“测试地图”。地图原件及来源说明见 [`../data/maps/VERA20k.md`](../data/maps/VERA20k.md)，无需外部启动器或 Downloads 目录。

选择页复用原类地图挂包与 INI 读取，将内置索引中的 38 张官方任务加入候选项，并解析 `battle(md).ini`、`mission(md).ini`、`missions(md).pkt` 和资源目录内的其他 `.pkt`，并扫描该目录的 `.map`、`.mpr`、`.yrm` 散文件。文件名忽略大小写去重，只列出能读取且包含有效地图尺寸的候选项；完整地形、对象及依赖资源仍在选定后的正常载图流程中校验。顶部下拉框提供通用分类、四类官方战役及“其他战役”，首次进入默认选择“战役地图”。普通地图按 `[Basic] MultiplayerOnly` 判断（为真时属于非战役，缺省为假）；内置测试地图使用独立的目录标记，只出现在“测试地图”和“所有地图”中，其核心加载模式仍由地图 INI 决定。内置条目无需扫描原版资源目录或读取完整单位列表，选中加载时才释放地图文件。切换分类即时筛选并清除旧选择。列表支持键盘选择、双击加载、“加载地图”按钮，以及 Esc／“返回”。同一资源目录复用已扫描的列表，并保留分类、选中地图和列表位置；重新加载资源时重新扫描，扫描期间可返回菜单。

导出 Release 原生库：

```sh
cmake -S code -B out/godot-native-release -DCMAKE_BUILD_TYPE=Release
cmake --build out/godot-native-release --parallel
```

在 Apple Silicon Mac 上发布 Universal 包时，另行构建 Intel 原生库，再使用官方模板导出：

```sh
cmake -S code -B out/godot-native-release-x86_64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Darwin -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build out/godot-native-release-x86_64 --target ra2_godot --parallel
mkdir -p out/release/macos-universal
godot --editor --import --quit --path code/godot --rendering-method mobile
godot --headless --path code/godot --export-release "macOS Universal"
```

## 验证

公共宿主测试只使用自己生成的临时输入，覆盖资源线程、菜单、配置、文本、地图目录和遭遇战大厅。先完成上述 Debug 原生构建，再运行：

```sh
python3 code/godot/tests/runners/run_tests.py --godot /path/to/godot --dotnet /path/to/dotnet
```

有桌面会话时追加 `--windowed --capture-dir out/host-captures`，验证实际窗口模式、显示尺寸、加载页及退出。逐场景输入与期望依据见 [公共宿主测试说明](tests/README.md)。测试只编译进 Debug，导出排除测试代码和场景。

纯核心公共测试默认关闭，显式启用后不需要 Godot、ICU 或安装素材：

```sh
cmake -S code -B out/tests-core -DBUILD_TESTING=ON -DRA2_BUILD_GODOT=OFF -DRA2_BUILD_MOVIES=OFF -DRA2_BUILD_RENDER_DEVICE=OFF
cmake --build out/tests-core --parallel
ctest --test-dir out/tests-core --output-on-failure
```
