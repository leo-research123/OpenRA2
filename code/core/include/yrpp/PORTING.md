# 本项目中的 yrpp

这个目录保留可直接修改的本地头文件和来源记录，不是 Git 子模块，也不参与外部仓库同步。原类实现的 `.cpp` 统一放在 [`../../src/yrpp`](../../src/yrpp)，暂不按职责细分。

- 来源仓库及固定提交：[`SOURCE.json`](SOURCE.json)。
- C++ 编码约束：仓库根目录 [AGENTS.md](../../../../AGENTS.md)。
- 构建入口：[`../../../CMakeLists.txt`](../../../CMakeLists.txt)。只显式编译已迁移的源码。

相对 `SOURCE.json` 的固定 YRpp 基线，本目录存在四个新增头：`FileClass.h`、`RawFileClass.h`、`DrawingBuffers.h`、`platform/ABI.h`。新增头文件和接口变更须遵守仓库编码约束。

`FileClass.h` 和 `RawFileClass.h` 从原 `CCFileClass.h` 拆出；旧入口继续包含两者，并保留 BufferIOFile/CDFile/CCFile 的继承声明。`Drawing.h` 继续包含 `DrawingBuffers.h`。迁入的 EA/XCC 代码保留原来的通知和许可，不为整个 YRpp 推定同一许可。

`ArrayClasses.h` 已恢复上游 `Memory.h` 的 DLL 数组接口，`IndexClass.h` 恢复 Game 数组分配域；不再依赖自建数组分配辅助头。`Memory` 的模板算法保持固定上游实现，非模板 `YRMemory` 方法统一在 `core/src/yrpp/Memory.cpp` 定义，直接封装共享 CRT 分配族。core 的 `Allocate`／`AllocateBytes` 负责 New／malloc 的恢复重试，compat 注册通知、new-mode 查询和请求上限，calloc／realloc 外层策略调用 core 的单次操作。

## 源码归属规则

- 已修改头文件中的普通非模板方法体放在对应 `.cpp`；通用模板和需要在调用点可见的 `constexpr` 定义保留在头文件。Blitter 仅支持本项目已校准的 `BYTE/WORD` 实例：头文件保留原布局与声明，全部方法体和像素规则隐藏在 `core/src/yrpp` 并显式实例化。其析构实现也放在 `.cpp`，由布局断言、虚调用测试和原版差分校验；其他 `= default`、`= delete` 和成员默认值保留原声明处。
- 本目录保留固定版本上游声明和对声明的必要校准，不允许放置 `.cpp` 文件。对应原类的成员实现、配套算法及其辅助头文件和数据表统一放在 `core/src/yrpp`。包括为保留原版 ABI 而采用显式对象参数的生命周期实现；不因函数位于项目 namespace 就分散到其他目录。
- 实现文件以原类名命名；同一类拆分多个实现文件时使用“类名 + 职责”命名，例如 `ConvertClassLifecycle.cpp`、`ConvertClassBlitters.cpp`、`MixFileClassBootstrap.cpp`。原有多类共用头文件对应的文件名（如 `ArrayClasses.cpp`、`BasicStructures.cpp`）保留。上游 `StaticInits.cpp` 按已有同名头文件拆为 `SlaveManagerClass.cpp`、`HouseClass.cpp`、`TechnoClass.cpp`、`BuildingClass.cpp`，方法体保持不变；这些待迁移实现尚未纳入 CMake 目标。
- 不在 `namespace yrpp` 下另造项目服务 API，也不通过删除 namespace 或改成全局函数来掩盖错误归属。
- `platform/ABI.h` 是平台兼容支撑头：它为原类声明提供 BYTE/WORD/DWORD/BOOL 的固定宽度、Win32 名称兼容、x86 调用约定宏和原有 `noinit_t` 构造标记，不拥有宿主状态或调度逻辑。`noinit_t` 从 `YRPPCore.h` 移入，定义不变，避免可移植声明间接引入 Syringe。
