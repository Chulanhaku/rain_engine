# Spatial Query + Raycast + Mouse Picking + Sphere/Box Sweep V0

更新：2026-10-10。状态：已实现并通过本批次回归。

## 1. 这一步提供什么

Physics 除了推进模拟，现在还提供主动查询的空间服务。调用者复用 `physics_world_3d::queries()`，无需各自遍历 ECS 或编写相交算法。武器射线、AI 视线、相机避障和技能检测可以接入此接口；这些玩法系统本身不在本批次实现范围内。

- Raycast：最近命中、任意命中、全部命中。
- Sphere/Box Sweep：沿直线平移，返回首次接触距离；支持最近、任意、全部命中。
- Sphere/Box Overlap：范围检测，支持任意命中、全部命中。
- 统一过滤：目标 layer、可选双向 mask、trigger 策略、忽略实体列表、本地 Tag 的 all/any/none 条件。
- Mouse Picking：相机与视口生成裁剪范围内的世界射线，样例演示选中、命中点、法线和 Sweep 停止位置。

源文件入口：[查询 API](../engine/runtime/include/rain/runtime/spatial_query_3d.hpp)、[查询实现](../engine/runtime/src/spatial_query_3d.cpp)、[屏幕射线](../engine/render/include/rain/render/picking_3d.hpp)、[样例系统](../samples/sample_2d_action/src/sample_spatial_query.cpp)。

## 2. 快照与同步约定

`spatial_query_3d` 拥有独立的只读快照：实体完整 ID、世界形状、过滤数据、Tag 和 AABB 树，不保存 ECS 组件指针。同步从 Collider 组件池读取数据。没有 Rigid Body 的 Collider 同样可查询。

```text
变换 / Collider / Tag / layer 或实体生命周期发生改变
                ↓
physics.sync_queries(world)
                ↓
构造查询快照 + AABB 树
                ↓
同一批系统进行多次只读查询
```

- `physics_world_3d::step()` 在积分、求解和事件生成之后自动同步，查询读到求解后的姿态。
- 初始化后、手动移动/编辑后，以及外部系统改动组件或 Tag 后，需要显式同步再查询。
- `physics_query_sync_system_3d` 可注册到变量帧的 `post_update`，处理一帧没有固定物理步的情况。
- 样例顺序：变换层级系统 priority 100 → 查询同步 priority 75 → 拾取 priority 50 → render_prepare。调度器数值较大的 priority 先执行。
- 同步之前读到旧快照是明确行为，不做隐藏的按需同步。命中后若世界已经改变，使用前应检查 `world::is_alive(hit.entity)`，必要时重建快照。
- `reset()` 清除物理状态和查询快照；`revision()` 可观察快照更新，`collider_count()` / `node_count()` 可用于调试。
- 只读查询可并发，但必须保证没有同时执行 `sync/clear/step/reset`；各线程不得共享输出 vector 的写入。

快照拥有自己的 Tag 副本；查询不会增加实体 Tag 计数、消耗 Tag 事件或生成碰撞事件。样例点击产生的 `state.selected` 属于样例交互状态，后续同步会将它纳入新快照。

## 3. 调用方式

```cpp
#include <rain/runtime/physics_world_3d.hpp>

// 初始化或本批 ECS 修改完成后同步一次；随后复用快照进行多次查询。
physics.sync_queries(target_world);
const auto& spatial = physics.queries();

rain::spatial_query_filter_3d filter;
filter.layer_mask = rain::collision_layer_3d::world |
                    rain::collision_layer_3d::enemy;
filter.ignored_entities.push_back(owner_entity);
filter.triggers = rain::query_trigger_mode_3d::exclude;

const auto ray_hit = spatial.raycast({
    .origin = {0, 2, 0}, .direction = {0, 0, 1}, .max_distance = 30
}, filter);

const auto sphere_hit = spatial.sweep_sphere(
    {{0, 2, 0}, 0.3f}, {0, 0, 1}, 10, filter);
const auto box_hit = spatial.sweep_box(
    {{0, 2, 0}, {0.3f, 0.5f, 0.3f}}, {0, 0, 1}, 10, filter);

std::vector<rain::spatial_query_hit_3d> nearby;
spatial.overlap_sphere({{0, 2, 0}, 3.0f}, nearby, filter);
const bool occupied = spatial.overlap_box_any(
    {{0, 2, 0}, {1, 1, 1}}, filter);
```

最近命中返回 `std::optional<spatial_query_hit_3d>`。`*_any` 返回 bool 并可提前停止。`*_all` 和 `overlap_*` 写入调用方可复用的 vector，先清空旧结果，按 distance、entity.index、entity.generation 排序；同一 Collider 最多返回一次。

## 4. 命中结果与边界

| 字段 | 语义 |
| --- | --- |
| `entity` | 包含 generation 的目标实体 ID |
| `point` | 世界坐标中的目标 Collider 表面点，不是被扫掠形状的中心 |
| `normal` | 指向查询形状的目标表面单位法线 |
| `distance` | 从查询起点沿归一化方向移动的世界距离 |
| `fraction` | distance / max_distance；零距离查询为 0 |
| `started_overlapping` | 起点已经接触或重叠，包含恰好贴面；此时 distance=0 |
| `trigger` | 此目标在快照中是否为 trigger |

方向会在内部归一化。方向长度不改变查询范围。所有查询中心、尺寸、方向和最大距离必须有限；查询半径、half_extents 和距离必须非负。正距离要求非零方向，零距离允许零方向并退化为 Overlap。无效输入返回空命中/false，并清空输出列表。

距离上限包含端点，但不会自动延长射线。查询使用几何接触边界，不额外加上模拟求解器的接触容差。起点包含在目标内部时，会返回稳定的代表法线/表面点；它不是最小脱离向量（MTD）或去穿透求解结果，调用方不能直接用它替代位置修正。

## 5. 过滤语义

| 条件 | 行为 |
| --- | --- |
| `layer_mask` | 与目标 `collision_filter_3d_component::layer` 有交集才参与 |
| `source_layer == 0` | 默认不检查目标的模拟 mask，使查询与接触响应策略可以独立 |
| `source_layer != 0` | 目标 mask 也必须接纳 source_layer，实现双向过滤 |
| `triggers` | `exclude` 默认排除、`include` 包含、`only` 只检测 trigger |
| `ignored_entities` | 精确匹配完整实体 ID；忽略父实体不自动忽略其子实体 |
| `tags` | 查询快照中的实体本地 Tag；支持 all/any/none，不自动进行名称前缀或实体层级继承 |
| 生命周期 | 非活动实体、缺少 Transform/Collider、无效几何不进入快照 |
| `physics.disabled` | 沿实体层级检查，禁用对象不进入查询快照 |
| `physics.trigger` | 沿实体层级检查，与模拟保持一致 |

查询不要求 `physics.dynamic`、`physics.static` 或 `physics.collider` Tag。冻结、静态和运动学对象仍可查询。样例在通用查询之上额外检查渲染可见性，隐藏父节点不会遮挡后方可见对象的拾取；核心空间服务不依赖渲染模块。

## 6. 几何与性能实现

模拟和查询共用 `physics_geometry_3d.hpp` 中的当前世界矩阵及形状转换：局部偏移、父变换继承、local matrix override、负缩放、非均匀缩放和剪切的保守球体界限遵循同一语义。

- 盒体 Collider 使用世界 AABB，与当前模拟一致；球体缩放使用保守界限。
- Sweep 查询球体的 radius 和查询盒体的 half_extents 已是世界单位。
- Sphere→Sphere、Sphere→Box、Box→Sphere、Box→Box 均有检测路径。
- 球体与盒体的 Sweep 通过“点到 AABB 的平方距离”分段求解二次方程，检测面、边和角；不把扩大后的 AABB 直接当作最终命中。
- Box→Sphere 使用相对运动反向计算，再恢复目标表面法线和点。
- 查询采用中位数分割的 AABB 树减少精确测试候选；边界向外取一个可表示的浮点单位，避免粗筛舍入漏检。
- 求交中间量使用 double；并不改变引擎变换和结果仍为 float 的精度范围。

V0 的 `sync()` 会重建树并复制过滤/Tag 数据，构建包含 O(N log N) 工作和可能的分配；最近/任意命中不构造临时命中数组，全部命中允许调用方复用容量。它不是已经完成静态/动态分离、增量 refit 或大世界性能优化的实现。先测量实体规模和查询负载，再决定是否优化同步频率或换索引。

## 7. 鼠标拾取与样例

`build_camera_frame_3d()` 供渲染和拾取共用。`screen_point_to_ray_3d()` 反投影视口位置与 D3D [0,1] 深度，射线从近裁剪面开始，到远裁剪面结束；命中 distance 因而从近裁剪面计量。

- 屏幕坐标和 viewport 使用同一单位，左上角为原点，支持 viewport x/y 偏移；右/下边界不属于视口。
- 通过实际 view_projection 反投影，支持透视矩阵，也通过了正交矩阵回归；当前引擎相机组件仍为透视相机。
- `rain_window::mouse_position_normalized()` 用 GLFW 逻辑窗口尺寸归一化鼠标，再映射到渲染像素范围；失焦、最小化或客户区外不发起拾取。
- 无效 FOV、视口、退化相机基、不可逆/非有限矩阵均返回失败，调用方必须检查结果。

运行 `sample_2d_action`：

| 操作 | 效果 |
| --- | --- |
| 鼠标左键 | 选中最近可见 Collider，黄色高亮并持有一个 `state.selected` Tag；点击空白清除 |
| `1` / `2` / `3` | Raycast / Sphere Sweep / Box Sweep |
| `I` | 是否包含触发器查询 |
| 鼠标移动 | 蓝色路径预览；Sweep 以半透明形状显示停止位置；白点是目标表面命中点，橙线是法线 |
| `R` | 原有场景重置，并清理选中状态 |
| WASD / Q E / 方向键 | 保留相机平移与观察 |

预览长度上限为 40 世界单位，Sweep radius/half_extents 为 0.45，仅属于样例配置。标记物没有 Collider，不污染查询，也不改变原有物理测试场景。移除 Collider、销毁/禁用/隐藏选中对象会清理选择；物理触发反馈颜色在取消选择后恢复。

## 8. 构建和验证

```powershell
cmake --preset gcc_debug
cmake --build --preset build_gcc_debug -j 8
ctest --preset test_gcc_debug --verbose
.\build\gcc_debug\samples\sample_2d_action\sample_2d_action.exe --spatial-test
.\build\gcc_debug\samples\sample_2d_action\sample_2d_action.exe --query-smoke
```

`rain_build_tests=ON` 且构建样例时，CTest 注册 `rain_spatial_query_regressions`。`--spatial-test` 不创建窗口；`--query-smoke` 创建图形窗口并在 180 帧内自动投影目标、切换三种模式，若未在每种模式命中目标则返回失败；图形烟测不加入默认无窗口逻辑测试。

本批次 Windows / GCC Debug 验证：

- 全目标构建成功；最终新增/修改代码编译无新增告警。
- Spatial Query：532 项检查、0 失败，包含 400 组独立连续距离参考算法交叉验证，以及 30 组混合形状场景的 BVH 对照。
- 既有 Physics sample：124 项检查、0 失败。
- CTest：4/4 通过，包含空间查询、物理、3D 渲染与 MSAA。
- 图形烟测：180 次查询，11 个查询 Collider，三种模式命中位集合为 7，进程返回 0；这是完整渲染路径烟测，不等同于人工视觉验收。

日志在 `build/spatial_query_20261010_ctest.log`、`build/spatial_query_20261010_smoke.log` 和 `.err`。人工建议补查拖动/缩放窗口、高 DPI 显示器切换，以及贴近盒角时球体 Sweep 的命中位置。

## 9. V0 范围和后续候选

本次没有实现 OBB/旋转 Sweep、三角网格射线、Capsule/Convex、目标运动预测、CCD 自动积分、滑动与多次反弹解算、相机控制器避障策略或角色控制器。旋转盒体拾取的是其保守 AABB，可能早于可见网格；这是与当前物理一致的明确限制。

建议下一批只选一个小目标：用 Sphere Sweep 实现实际相机避障（明确初始重叠和安全间距），或针对空间服务做碰撞体 DebugDraw/查询统计。随后再按测量考虑静态/动态索引、增量同步、批量查询；避免一次扩展到完整控制器和通用约束求解。
