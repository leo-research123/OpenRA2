# R2 图片模块：EA 原样源码

`wwlib/`、`xcc/` 和 `MissionEditorPackLib/` 直接放在 `ea/` 下，保存固定提交中的完整模块目录，文件内容逐字节保持原样，包括注释、许可标头、换行和原工程文件。仓库名、阶段名和 upstream 包装层已移除；目录内部结构保持不变。上游根 README/LICENSE 加来源前缀放在 `ea/` 下，内容保持原样。

实现来源优先 **YRpp → EA 编辑器（PackLib / 内置 XCC）**，以经校准的 YRpp 对象和接口为基础。图像模块对编辑器未覆盖的 PCX、颜色表及原位绘制，已采用下表固定 Renegade WWLib 的最小对应函数体，并补齐 YR 差异。生产构建使用项目目录中的适配实现，不直接构建整套上游库。

| 来源 | 固定提交 | 完整复制范围 | 文件数 |
|---|---|---|---|
| [EA CnC_Renegade](https://github.com/electronicarts/CnC_Renegade/tree/3e00c3a1b97381bb28be89a35b856375e0629a08) | `3e00c3a1b97381bb28be89a35b856375e0629a08` | [Code/wwlib](wwlib)，以及上游根目录 README、LICENSE | 265 |
| [EA CNC_TS_and_RA2_Mission_Editor](https://github.com/electronicarts/CNC_TS_and_RA2_Mission_Editor/tree/6abf0f557469baea73079c6bf6550709e2e3584e) | `6abf0f557469baea73079c6bf6550709e2e3584e` | [3rdParty/xcc](xcc)、[MissionEditorPackLib](MissionEditorPackLib)，以及上游根目录 README、LICENSE | 92 |

合计 357 个上游文件，3,391,950 字节。完整指所列目录中的全部 Git 文件；这是相关模块的完整快照，构建整个上游工程所需的其他模块、第三方库和工具链仍按原工程要求配置。

## 已存快照的职责与用途

**WWLib** 是 Westwood 的通用底层库，这份源码来自 EA 公开的 Renegade，保存为可核验的算法基线。它包含文件读写、容器、颜色与调色板、Surface、普通/RLE 绘制器等运行时基础设施，保留的是较早的结构和平台接口，并非直接发布的 YR 实现。

**XCC** 是 Olaf van der Spek 编写的 C&C 资源格式工具库，这份副本由 EA 的 TS/RA2 编辑器随源码提供。它擅长读取、解码和转换 SHP、PAL、MIX 等文件。R2 主要使用其 TS/RA2 SHP 帧格式和索引像素解码；它不提供 YR 游戏运行时的完整 SHPReference 生命周期。

旁边的 **MissionEditorPackLib** 是编辑器侧的资源读取封装，调用 XCC 等代码给编辑器提供图片数据。保留它是为了查看真实调用流程、文件选择和帧处理方式；它不是另一套通用底层库。

项目对象与接口从 YRpp 派生，PackLib/XCC 提供资源处理流程、文件格式和算法实现；YR 特有的缓存、引用生命周期、对象布局和扩展绘制行为均按目标二进制校准并补齐。

## 快照索引

- [shapeset.h](wwlib/shapeset.h)：帧目录、局部矩形、像素数据访问。
- [rlerle.h](wwlib/rlerle.h)、[blitter.h](wwlib/blitter.h)、[blitblit.h](wwlib/blitblit.h)：RLE/普通像素绘制器。
- [draw.cpp](wwlib/draw.cpp)、[blit.cpp](wwlib/blit.cpp)、[convert.cpp](wwlib/convert.cpp)：图片绘制、裁剪和颜色/效果分派。
- [cc_structures.h](xcc/misc/cc_structures.h)、[shp_ts_file.cpp](xcc/misc/shp_ts_file.cpp)、[shp_decode.cpp](xcc/misc/shp_decode.cpp)：TS/RA2 SHP 格式和解码。
- [palet.cpp](xcc/misc/palet.cpp)：XCC 调色板处理。
- [MissionEditorPackLib.cpp](MissionEditorPackLib/MissionEditorPackLib.cpp)：编辑器图片读取调用方。

## 来源与校验

[image-sources.json](image-sources.json) 记录仓库、完整提交号、目录范围、每个文件的上游路径与本地路径映射、Git 模式、大小、Git blob SHA-1 和 SHA-256。Renegade 文件核对固定 Git 树；编辑器文件直接读取该提交的 Git blob。`.gitattributes` 禁止对快照进行换行、过滤器和 ident 转换。

在仓库根目录复查快照的文件集合、大小、内容和可执行模式：

```sh
python3 scripts/verify_r2_upstream.py
```

历史基础 RLE 算法探针默认直接编译此处未修改的 Renegade 头文件，仅用于复查既有实验：

```sh
out/research/resource-venv/bin/python scripts/probe_ea_r2_rle.py
```

## 许可与适配边界

上游许可原样保存在 [Renegade/LICENSE.md](RENEGADE_LICENSE.md)、[Mission Editor/LICENSE.md](MISSION_EDITOR_LICENSE.md)、[XCC/COPYING](xcc/COPYING)。每个源码文件原有的 EA、Westwood、Olaf van der Spek 等来源标头均保留。

