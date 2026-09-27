# 离线原版参考与 YRpp 声明检查

- `generate_*_reference.py` 生成测试参考；`compare_image_*` 提供共用模拟器服务，入口／虚表数据见 `../fixtures/original_*_entries.json`。
- `msvc_abi_contract_tests.cpp`、`msvc_abi_codegen.cpp` 经 [MsvcAbiTests.cmake](../../cmake/MsvcAbiTests.cmake) 检查 Windows x86 Microsoft ABI 的声明、虚表和调用约定。
- 接受 `--dll` 的比较器需调用者提供匹配探针；本仓库不构建该接入产物。离线检查不代表真实游戏运行验证。

依赖见 [requirements.txt](requirements.txt)。
