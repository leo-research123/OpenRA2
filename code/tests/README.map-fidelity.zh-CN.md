# 地图显示一致性测试与定位指南

本组测试使用上传的原版函数和固定版本 `gamemd.exe` 校准预期，在真实 C++ 原类、MapView、CellClass 和绘制请求上运行。合成 INI/SHP/TMP 只是可控资源，不是重新实现一套对象模型，也不是原版完整画面的替代品。

## 运行

从仓库根目录执行：

```sh
python scripts/run-map-fidelity.py --compiler clang++ --generator Ninja
```

脚本以 `RA2_BUILD_GODOT=OFF` 构建 `ra2_map_fidelity_tests`、`ra2_map_fidelity_probe`，运行标签为 `map-fidelity` 的测试。CTest 为 map fidelity 和 building visual 各运行一次全集；两项三角函数首次初始化检查各用独立进程，并从 visual 全集中排除。启用软件渲染时，`software` 标签运行 visual 全集。单项定位使用下面的 `--case`，不重复注册到常规 CTest。可不指定 `--compiler`、`--generator`，使用本地默认工具链。启用像素测试时添加 `--software`。

精确复现一个断点：

```sh
./build-map-fidelity/ra2_map_fidelity_tests --case F03_rules_turret
./build-map-fidelity/ra2_map_fidelity_tests --case N09_turret_facings_original_table
```

Windows 多配置生成器的可执行文件通常位于 `build-map-fidelity/Debug/`，并带 `.exe` 后缀；CTest/脚本已传配置参数。

运行原有全部测试，不绕过审计：

```sh
python scripts/run-map-fidelity.py --full
```

TypeClass 审计使用 [fixtures/typeclass-reference-index.json](fixtures/typeclass-reference-index.json) 和现有源码。

## 原版预期从哪里来

`fixtures/map_fidelity_reference.json` 保存 EXE 哈希、12 份参考函数的哈希及职责、指令见证和炮塔表。`fixtures/building_turret_reference.txt` 是从目标 EXE 的 `0x007F4890` 提取的表，加上目标指令对应的量化公式，生成的 160 个独立角度/帧样本。样本覆盖每个方向的中点舍入两侧，不从修复后的 C++ 实现生成。

```sh
python scripts/verify-map-fidelity-reference.py \
  --exe /path/to/gamemd.exe \
  --reference-dir /path/to/functions
```

`--reference-dir` 可省略；省略时只验证二进制和样本，不声称验证了参考 C 文件。脚本不执行 EXE。它会拒绝不同版本的 EXE，不会静默更新金标准。

这是**静态版本/预期校准**，并不等于ABI 校验、完整游戏时序或像素检查已经通过。

## 真实地图的诊断入口

```sh
./build-map-fidelity/ra2_map_fidelity_probe \
  --game-dir /path/to/game \
  --map YOURMAP.MAP \
  --ticks 120 \
  --out map-diagnostics.json
```

`--map` 使用现有文件/MIX 服务可解析的名字。默认加载真实游戏资源；仅在使用独立松散测试文件时加 `--loose-files`。没有必要为了诊断先在 Godot 内复制状态或造假贴图。

JSON 每栋建筑包含 `type_id`、`art_section`、`art_image`、`resolved_body`、本体帧数、HP、状态帧、Remapable、炮塔/VXL 标志、炮管文件名，以及 9 类部件和 21 个动画槽的配置/实例/帧状态。`resolved_body` 是最终尝试的候选名字，是否真的加载成功必须同时看 `body_frames`；该字段并非完整的 MIX 来源命中日志。

定位顺序：先核对类型/节/别名与帧数；再看配置是否正确进入类型；再看动画槽是否存在及取帧是否正确；随后检查绘制请求；只有这些正确而实际画面仍错，才继续收缩到宿主深度、调色、遮挡和 UI 几何。`unsupported_voxel_turret=true` 表示已知未实现的 VXL 绘制，不应再误判为 SHP 文件缺失。
