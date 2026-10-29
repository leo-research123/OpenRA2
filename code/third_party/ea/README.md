# EA 实现来源

来源：[CnC_Remastered_Collection](https://github.com/electronicarts/CnC_Remastered_Collection/tree/f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae)，固定版本 `f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae`。文件哈希见 [source.json](source.json)。

## 复用范围

上游文件均位于 `REDALERT/`；本地声明位于 `core/include/yrpp`，实现位于 `core/src/yrpp`。

| 上游文件 | 本地用途 |
|---|---|
| `MIXFILE.H/.CPP`、`LISTNODE.H` | MixFileClass、GenericList：归档索引、缓存所有权、注册与遍历。 |
| `WWFILE.H`、`RAWFILE.H/.CPP`、`BFIOFILE.CPP`、`CDFILE.CPP`、`CCFILE.CPP` | FileClass 文件继承链及磁盘／MIX 读取。 |
| `RULES.CPP` | RulesClassLifecycle、General、AI、Tables：初始化和规则分节读取。 |
| `CONQUER.CPP`、`LOGIC.CPP`、`ANIM.CPP`、`OPTIONS.CPP` | 主循环、Logic 列表、动画阶段与时间控制。 |

本地适配（2026）：保留 YRpp 原类接口，以 YR 二进制校准布局、默认值、CRC、缓存和调度差异；宿主文件会话与路径信息放在对象外部。拥有资源的对象禁止复制，列表遍历避免将哨兵视为派生对象。

RawFile 使用 32 位文件位置、Windows UTF-8 路径转换及 macOS/Linux POSIX I/O；`READ|WRITE` 保留截断行为。打开失败返回 false，Size/Seek 失败返回 -1，I/O 返回实际数量；拒绝超出接口范围的文件。原版致命错误／重试界面未实现，损坏输入的失败行为不作等价承诺。

验证由 `raw_file`、`resource_system` 及离线原指令对照覆盖，不代表真实游戏进程验收。Rules 复用未覆盖完整初始化与类型加载图；这些实现基于公开 Red Alert 源码，并经 YR 差异校准。

图像相关固定版本、许可及哈希清单见 [IMAGE_SOURCES.md](IMAGE_SOURCES.md)；其中源码快照不参与生产构建。

## 许可

[LICENSE.TXT](LICENSE.TXT) 保留上游 `REDALERT/License.txt` 及附加条款；[REPOSITORY_LICENSE.md](REPOSITORY_LICENSE.md) 保留仓库级声明。

Original notice:

> Copyright 2020 Electronic Arts Inc.
>
> TiberianDawn.DLL and RedAlert.dll and corresponding source code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
>
> TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed in the hope that it will be useful, but with permitted additional restrictions under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT distributed with this program. You should have received a copy of the GNU General Public License along with permitted additional restrictions with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
