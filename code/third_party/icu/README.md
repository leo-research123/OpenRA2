# ICU4C

此目录维护 ICU 78.3 的 CMake 构建适配、数据嵌入工具和上游许可，不改写上游源码。

默认由 `FetchContent` 下载固定的官方源码包并校验 SHA-256，下载与编译结果保存在当前构建目录的 `_deps` 中；后续增量构建复用缓存。`common` 和 `i18n` 直接使用上游 `sources.txt` 清单，数据使用源码包内的 `icudt78l.dat`，以连续、对齐的 C 数组静态链接。整个流程只需要项目编译器与 Python 3，无需预装 ICU、运行上游 configure 或安装 GNU make。

旧构建目录若曾缓存 `ICU_ROOT`，可用 `-U 'ICU_*'` 清除旧路径后切换到默认源码构建。

目标继承主工程的架构、macOS deployment target 和 MSVC CRT，并支持多配置构建。当前项目支持的桌面架构均为小端；大端目标需要另一份上游数据包。生成文件只写入构建目录。

- 默认无需 `ICU_ROOT`；仅启用 Godot 宿主时准备 ICU，核心构建不触发下载。
- 离线可设置 `FETCHCONTENT_SOURCE_DIR_RA2_ICU=/absolute/path/to/icu`，指向解包后包含 `source/` 与 `LICENSE` 的 ICU 78.3 根目录。
- 可用 `ICU_ROOT` 显式选择现成静态 ICU 70+，或由父工程提供 `ICU::i18n`、`ICU::uc` 和 `ICU::data`。
- 版本、URL 和校验值统一位于本目录的 `CMakeLists.txt`。升级时须同步更新源码检查、数据文件名和许可。

上游：[ICU 78.3](https://github.com/unicode-org/icu/releases/tag/release-78.3)。随游戏导出的许可从本目录 `LICENSE.txt` 同步。
