# Rain Engine

Rain Engine 是一个以 **data-oriented** 为底层设计原则的游戏引擎，强调运行时效率、清晰的数据布局、可扩展的 gameplay framework，以及以 gameplay_tag 为核心的规则驱动玩法系统。

它希望支持**属性与规则驱动的启发式玩法设计**：通过实体的属性、状态、关系与上下文，由通用规则自动匹配和组合行为，让新的对象组合也能复用已有机制。

设计者可以调整数据、Tag 与规则来探索玩法、验证效果并调整方向；引擎持续建设强可扩展性与渲染能力。具体交互例子只是说明机制，不限定引擎题材或功能范围。通用规则框架属于规划方向，当前已实现能力见项目指南。

## 项目入口

- [项目介绍、当前能力与发展路线](PROJECT_GUIDE.md)：适合新对话、Work 和协作者阅读的项目背景。
- [空间查询 V0：接口、同步与样例](docs/Spatial_Query_V0.md)：Raycast、Overlap、鼠标拾取和 Sphere/Box Sweep。
- [协作与变更交付要求](AGENTS.md)：开发和文档交付约定。
- [批次说明与验证记录](docs/)：具体改动、测试结果及任务 diff。
- [历史设计讨论](rain/rain.md)：早期思路；当前状态与路线请先看项目指南。

目前已经有 2D/3D 渲染、PBR、视锥剔除和带 Tag 驱动行为的 3D 物理基础，以及统一的空间查询服务；高级玩法框架与渲染能力仍在逐步完善。采用小步开发节奏，每次交付一个可理解、可验证的能力。

## 构建与样例

当前已验证环境为 Windows / D3D11，使用 CMake 预设：

```powershell
cmake --preset gcc_debug
cmake --build --preset build_gcc_debug -j 8
ctest --preset test_gcc_debug --verbose
```

- `sample_2d_action`：主要交互与物理验证样例，包含 3D 场景、可切换的 2D 覆盖层，支持 `--physics-test`、`--spatial-test` 和 `--query-smoke`。鼠标左键选中，1/2/3 切换 Raycast/Sphere Sweep/Box Sweep，I 切换触发器过滤。
- `sample_3d_pbr`：3D PBR 材质样例。

模块地图、当前限制、详细路线与交接说明见 [PROJECT_GUIDE.md](PROJECT_GUIDE.md)。
