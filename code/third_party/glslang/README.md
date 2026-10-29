# glslang host compiler

此目录提供未修改的 Khronos glslang 15.0.0 的构建适配和上游许可。
默认通过 `FetchContent` 下载固定提交 `46ef757e048e760b46601e6e77ae0cb72c97bd2f` 的源码包，并校验 SHA-256；源码和编译结果均留在当前构建目录的 `_deps` 中。无需预装 glslang，也不搜索系统 PATH。

工具通过独立的原生 CMake 工程构建为 Release，只启用 GLSL 到 SPIR-V；关闭 HLSL、优化器、remapper、上游测试和安装，不再下载额外的上游依赖。主工程仅在 `RA2_BUILD_RENDER_DEVICE=ON` 时准备它。增量构建复用已有工具；工具更新后，相应 SPIR-V 会重新生成。

交叉编译时不继承目标工具链、编译器、架构或 sysroot，也清除环境中的 `CMAKE_TOOLCHAIN_FILE`、`CC`、`CXX` 和编译／链接 flags。工具使用构建机 SDK；如需指定宿主编译器，可设置 `RA2_GLSLANG_HOST_C_COMPILER` 与 `RA2_GLSLANG_HOST_CXX_COMPILER`。它们与主工程的目标编译器分别配置。

- 离线构建可设置 `FETCHCONTENT_SOURCE_DIR_RA2_GLSLANG=/absolute/path/to/glslang-15.0.0`，指向包含上游 `CMakeLists.txt` 和 `CHANGES.md` 的解包目录。
- 仅在明确需要外部宿主工具时设置 `RA2_GLSLANG_EXECUTABLE=/absolute/path/to/glslang`；默认留空。
- 旧构建目录如缓存了系统或手工编译器路径，使用 `-U RA2_GLSLANG_EXECUTABLE` 清除后即可回到默认源码构建。
- 版本、官方源码 URL、校验值和配置统一位于本目录的 `CMakeLists.txt`。不改写上游源码。

上游：[Khronos glslang 15.0.0](https://github.com/KhronosGroup/glslang/tree/46ef757e048e760b46601e6e77ae0cb72c97bd2f)。本工具仅供构建使用，不随游戏运行库链接或发布。
