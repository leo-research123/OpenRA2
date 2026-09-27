# OpenTS 实现来源

来源：[OpenTS](https://github.com/OpenTS-Developers/OpenTS/tree/44fac744f70235e0d5ddca107364a68f95132ce9)，固定版本 `44fac744f70235e0d5ddca107364a68f95132ce9`。文件映射与哈希见 [source.json](source.json)。

| 上游模块（`code/`） | 本地复用范围 |
|---|---|
| `target.cpp`、`abstract.cpp`、`abstype.cpp` | Target 构造／转换及 Abstract 原类实现参考。 |
| `building.cpp`、`delay.cpp/.h` | 建筑绘制、动画阶段／队列、销毁及 TransitionTimer。 |
| `light.cpp`、`cell.cpp`、`blight.cpp`、`ovrlight.cpp`、`smudtype.cpp`、`xsurface.cpp` | 灯光、光柱、污迹放置及填充圆算法。 |
| `techtype.cpp`、`builtype.cpp`、`cell.cpp`、`building.cpp`、`display.cpp` | 建造规则、INI 与放置判定。 |
| `bullet.cpp`、`tactical.cpp` | 弹丸调色板、选择框、行动线及心灵控制连线绘制。 |
| `object.cpp`、`building.cpp`、`unit.cpp`、`rect.h`、`anim.cpp`、`mainloop.cpp` | 显示入口、可见性、裁剪、对象注册与帧循环顺序。 |

本地实现位于 `core/src/yrpp`，保留 YRpp 接口及原对象状态。TS/YR 差异以固定 `gamemd.exe` 校准（SHA-256：`7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6`），包括布局／ABI、绘制深度、动画计时、光照算术和放置条件。宿主绘制包、临时像素存储与 Godot 适配属于本地代码。

复用只覆盖表中职责；`TacticalClassRender.cpp` 为现有场景流程整理，不代表完整 Render 或 DDraw 缓存复现。

本地建筑适配：RedAlert2Open，2026。实现保留原版权声明；[LICENSE.md](LICENSE.md) 原样保留上游许可、EA Section 7 附加条款及免责说明。
