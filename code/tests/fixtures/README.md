# 测试夹具索引

本目录保存合成输入、原版指令输出及离线比较元数据。原版参考使用固定 `gamemd.exe`，SHA-256：
`7b8a068535d6af06845edf95ae829b113d00c02909330e16f197426cd7db94b6`。

可用生成器与依赖见 [x86 工具说明](../x86/README.md)；参数以各脚本 `--help` 为准。文本记录的字段以消费测试的解析代码为准，配套 JSON 保存摘要或分支证据。

## 用途索引

下表省略通用后缀 `_reference.txt`；花括号表示同组文件。

| 文件 | 用途 |
|---|---|
| `encrypted.mix.hex` | 合成 A.TXT/B.TXT（alpha/beta）的加密 MIX；由 Python pow 和 Blowfish ECB 生成。标志为 `0x30000`，摘要留零，读取器不校验摘要。 |
| `map_{floor,iterator,projection,camera}`、`building_coordinates` | 地形几何、投影、镜头边界及建筑坐标。 |
| `map_radar`、`map_radar_pixels.bin`、`map_radar_world` | 雷达缩放、RGB565 像素、地图区域判断和屏幕拾取。 |
| `overlay_draw`、`cell_overlay`、`sprite_draw` | 覆盖物、阴影、Anim/Terrain 绘制请求；后两组附分支清单与 JSON 证据。 |
| `building_{health,scene,gaps,parts,stage}`、`transition_timer` | 血条／选择框、排序、墙体、容量／电力、部件帧和动画计时。 |
| `building_lights`、`spotlight_motion` | 建筑光照算术、灯光运动及绘制参数。 |
| `abstract`、`swizzle`、`logic_layer` | Target 转换、距离／方向、指针重映射及列表排序。 |
| `weapon_stream`、`{weapon,warhead,bullet}_type`、`weapon_selection` | 保存输出、类型／CRC 计算和武器选择条件。 |
| `time`、`mission_orders`、`object_mission_update` | 主循环／暂停／动画调度、任务切换及对象／任务帧状态。 |
| `infantry_{frame,action,frame_update,idle,fear_fire}` | 动画帧、动作切换、待命、任务中断、恐惧及开火判定。 |
| `infantry_{occupation,expiration,arrival}`、`foot_arrival` | 占位、引用失效、到格状态和回调。 |
| `walk_{locomotion,process}`、`foot_reach`、`infantry_speed` | 移动控制、步进、地形可达性与速度。 |
| `map_{region_threat,zones}`、`astar_{hierarchy,cost,regular}`、`{cell,infantry}_passability` | 威胁、区域图、路径搜索成本与通行判定。 |
| `techno_discovery`、`visibility` | 发现、遮挡邻居和扫描偏移。 |

其他专项夹具按文件名对应功能；覆盖范围由消费测试及配套元数据定义。

## 二进制与 GPU 格式

- **ABuffer**：[abuffer_reference.bin](abuffer_reference.bin) 以 8 字节 `ABUFREF1` 开头，随后依次为 shroud、fog、AlphaShape 三个 65,536 字节数组，索引为 `old * 256 + source`。由 [生成器](../x86/generate_abuffer_reference.py) 执行原版表初始化和像素循环得到。`TestABufferGpuReference.tscn` 默认读取此文件，支持 `--reference=<绝对路径>`，比较全部 196,608 个结果。
- **光照批次**：`ra2_lighting_gpu_fixture <output.bin>` 从合成输入生成几何和空间分箱，交给同一 GPU 测试的 `--lighting-batch=<绝对路径>`。`LIGHTBT1` 文件保存尺寸／计数、初始像素、20 字包、源图集、空间分箱及批次头偏移；数值使用小端 32 位字。
- **雷达像素**：[map_radar_pixels.bin](map_radar_pixels.bin) 为 `RDR1` 容器，含 8 组输入及 87,388 个输出像素。每组先存四个小端 uint32（输入宽／高、输出宽／高），再存 RGB 输入及 RGB565 输出。
- **SHP 像素**：[shape_gpu_reference.txt](shape_gpu_reference.txt) 保存合成图像／调色板及 102 组原版输出。`ra2_shape_gpu_fixture` 生成生产绘制包，`TestShapeGpuReference.tscn` 通过 `--packets=<绝对路径>` 比较 78,336 个颜色和 Z 值。
- **光照算术期望**：`generate_building_light_gpu_fixture.py` 的 28 组 RGB565／通道掩码／深度用例根据恢复的算术计算，未执行原版 EXE 指令。

## 元数据与验证范围

- `original_{file,image,blitter}_entries.json` 保存地址、指令字节、历史导出名及虚表信息，供离线比较器使用。
- [typeclass-reference-index.json](typeclass-reference-index.json) 记录 490 个函数的历史导出种类、摘要和大小，仅作资料索引。
- 原版输出通常在受控输入和替身服务下采集；GPU 用例使用合成素材。数值、状态、绘制请求及分支对照各自只证明测试覆盖的契约，不代表完整 ABI、原版进程运行或完整游戏流程验收。
- 范围限制：Weapon 保存未覆盖完整 Load／失败路径；区域图未覆盖桥／隧道连接；步兵到格未覆盖建筑／运输／C4 效果；开火判定未覆盖完整弹丸／伤害链；建筑帧和任务测试未覆盖完整生产、出售结算及 Infantry 帧链。
