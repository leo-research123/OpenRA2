# CMake 文件职责

构建命令见 [核心 README](../README.md)。源码由显式清单选择。

| 文件 | 职责 |
|---|---|
| `Core.cmake` | 独立核心、默认存储、公开头路径和数值选项 |
| `SoftwareRender.cmake` | 可选软件绘图与 PCX；默认关闭 |
| `TextEncoding.cmake` | 原生文本解码；默认通过 `third_party/icu` 自动准备固定 ICU 依赖 |
| `Godot.cmake` | 固定 godot-cpp 依赖、桥接、着色器与输出配置 |
| `SourceSets.cmake` | 原类算法的共享源码清单 |
| `CompilerOptions.cmake` | C++20、异常与匹配的 MSVC CRT |
| `CoreIncludes.cmake` | 实现与测试的私有包含路径 |
| `CheckYrppLayout.cmake` | 公开头、核心与宿主之间的依赖检查 |
| `CheckCoreOrganization.cmake`、`YrppSourceOwnership.cmake`、`YrppPublicHeaders.txt` | 原模块归属、公开头清单、文件关停和链接依赖检查 |
| `Tests.cmake`、`GoogleTestSupport.cmake` | 默认关闭的公共测试入口和上游 GoogleTest；组件注册在 `../tests/`，每个目标显式登记内部头访问 |
| `clang-msvc-windows-x86.cmake`、`ClangMsvcLinkRules.cmake` | clang-cl/xwin 交叉工具链与链接规则 |

`core_full_link` 和 `software_render_link` 验证对应静态库的整库链接；算法、资源生命周期与宿主行为由各自测试验证。
