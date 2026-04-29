# 仿真结果对象与结果 Schema 设计

日期：2026-04-29

## 目标

为主程序补齐稳定的“结果对象边界”，解决当前仿真结果过度依赖 `GridMap + 绘图函数` 的问题，使一次仿真任务完成后能够：

- 以统一结构体保存任务级结果
- 支撑 UI 多样化展示，而不只是一张热力图
- 为后续派生指标计算提供稳定输入
- 为后续报告生成、结果导出和场景对比提供统一入口

本轮设计只定义结果侧对象、职责边界、内部校验和文档 schema 口径，不要求同步实现结果 JSON 导入链路。

## 当前问题

当前实现中：

- `Simulation/EMC_Engine.h` 仅暴露 `_LossGrid` 与 `lossGrid()`
- `Resource/ui/Simulation.cpp` 直接把 `_LossGrid` 当作绘图输入
- `Utils/PaintImage.hpp` 只接受 `GridMap`

这导致结果在架构上偏“绘图导向”，缺少以下能力：

- 缺少任务级结果根对象
- 缺少结果元数据与状态描述
- 缺少按发射机拆分的子结果
- 缺少为后续指标和报告保留的结构化承接层
- 缺少结果一致性校验入口

## 设计结论

本轮采用以下组合：

1. 强类型结果结构体：
   - `SimulationTaskResult`
   - `ScalarField2D`
   - `EmitterResult`
   - `DerivedMetrics`
2. 结果侧内部校验：
   - 每个结果对象提供 `validate()`
   - 根对象统一执行跨字段一致性校验
3. 文档 schema：
   - `docs/schema/simulation-result.schema.json`
   - `docs/schema/simulation-result.template.jsonc`
4. 暂不实现结果 `JsonLoader` / `ResultLoader`

推荐理由：

- 当前首要问题是先建立稳定结果边界，而不是立即做可回读的结果序列化体系
- 先让 UI、指标、报告都消费同一个结果根对象，再决定是否增加结果导入/导出链路，风险更低

## 职责分层

### `PEPropagationSolver`

由当前 `Propagation_Engine` 演进而来，职责收敛为：

- 单发射机 1D/2D PE 求解
- 不关心任务状态
- 不组装任务级结果
- 不直接面向 UI

它是“数值求解器”，不是结果根对象的拥有者。

### `EMC_Engine`

职责收敛为“具体计算引擎”：

- 持有冻结后的 `Fleet` 和输入快照
- 调度多个发射机的传播求解
- 完成总场聚合
- 产生按发射机拆分的原始结果
- 不直接生成 `SimulationTaskResult`

它负责计算，不负责任务级结果编排。

### `simSchedulerCtx`

新增任务级调度与结果组装层，职责如下：

- 接收冻结后的 `DataSnapshot`
- 接收编队来源信息
- 后续若存在“预设编队改造输入”的逻辑，在此层执行
- 调用 `TransferToEngine` 与 `EMC_Engine`
- 统一记录任务时间、状态、错误信息和摘要
- 组装并返回 `SimulationTaskResult`

命名说明：

- 本轮按用户确认，将原提议的 `PropagationEngine` 统一命名为 `simSchedulerCtx`
- `simSchedulerCtx` 是任务级 façade，不等价于单发射机传播求解器

### UI 层

`Resource/ui/Simulation.cpp` 后续只消费 `SimulationTaskResult`：

- 主图绘制读取 `aggregatedField`
- 后续单发射机视图读取 `emitterResults`
- 后续指标面板和报告入口读取 `derivedMetrics`

UI 不再自行拼装结果对象，也不再直接依赖裸 `GridMap` 作为唯一结果边界。

## 核心类型设计

### `FormationSource`

用于描述当前结果来源于手工输入还是预设编队改造：

- `ManualInput`
- `PresetFormation`

### `SimulationResultStatus`

用于描述任务级最终状态：

- `Succeeded`
- `Failed`
- `Cancelled`

### `EmitterResultStatus`

用于描述单发射机结果状态：

- `Succeeded`
- `Failed`
- `Cancelled`
- `Skipped`

### `ScalarFieldQuantity`

用于区分二维标量场的物理语义：

- `AggregatedPowerDbm`
- `PathLossDb`

本轮先覆盖当前主链路已出现的两类量，后续可扩展。

### `ScalarField2D`

用于承接所有二维标量场，不再直接暴露裸 `GridMap`。

建议字段：

- `fieldId`
- `displayName`
- `quantity`
- `valueUnit`
- `axisXUnit`
- `axisYUnit`
- `rows`
- `cols`
- `originX`
- `originY`
- `stepX`
- `stepY`
- `std::optional<double> noDataValue`
- `std::vector<double> values`

设计约定：

- `values` 使用一维连续数组，按 row-major 存储
- `rows * cols` 必须与 `values.size()` 一致
- `originX / originY + stepX / stepY` 共同定义物理坐标系
- `valueUnit` 由 `quantity` 约束，不允许任意填值

本轮不再推荐结果侧继续使用 `std::vector<std::vector<double>>` 作为长期边界类型。

### `EmitterResult`

用于保存单个发射机的原始结果和最小元数据。

建议字段：

- `emitterId`
- `shipId`
- `status`
- `centerFrequencyGHz`
- `transmitPowerDbm`
- `worldX`
- `worldY`
- `worldZ`
- `errorMessage`
- `ScalarField2D field2D`

设计约定：

- 成功态必须包含合法 `field2D`
- 失败、取消或跳过态允许没有有效场数据，但必须能解释原因
- 第一版只保留最少必要参数，不在此层塞入全部设备 DTO

### `DerivedMetrics`

第一版只作为派生指标容器壳结构，不立即定义 `SCF / S3I / T_elev / D_desense` 字段。

建议字段：

- `available`
- `metricsSchemaVersion`
- `note`

设计约定：

- `available == false` 时允许为空壳
- `available == true` 时至少要求有版本信息
- 具体指标字段在后续指标设计轮次中单独补齐

### `SimulationTaskResult`

任务级结果根对象，是后续 UI、派生指标、报告和导出的唯一入口。

建议字段：

- `resultSchemaVersion`
- `taskId`
- `modelType`
- `status`
- `formationSource`
- `std::optional<int> presetFormationId`
- `startedAtUtcMs`
- `finishedAtUtcMs`
- `durationMs`
- `errorMessage`
- `summaryText`
- `DataModel::DataSnapshot inputSnapshot`
- `ScalarField2D aggregatedField`
- `std::vector<EmitterResult> emitterResults`
- `DerivedMetrics derivedMetrics`

设计约定：

- 必须保留 `inputSnapshot`，确保结果可追溯、可复现、可用于报告
- 任务成功时，`aggregatedField` 与 `emitterResults` 都必须可用
- 任务失败或取消时，不强制要求存在主图数据
- 编队来源必须使用 `formationSource + optional<int> presetFormationId` 联合表达

## 校验设计

结果侧采用“文档 schema + C++ 内部校验”的双层约束，而不是完全复制输入侧 `JsonLoader` 链路。

### `ScalarField2D::validate()`

至少校验：

- `fieldId` 非空
- `rows > 0`
- `cols > 0`
- `stepX > 0`
- `stepY > 0`
- `values.size() == rows * cols`
- `quantity` 与 `valueUnit` 匹配

推荐匹配规则：

- `AggregatedPowerDbm -> dBm`
- `PathLossDb -> dB`

### `EmitterResult::validate()`

至少校验：

- `emitterId` 非空
- `shipId` 非空
- `Succeeded` 时 `field2D` 必须合法
- `Failed` / `Cancelled` / `Skipped` 时允许没有有效场，但 `errorMessage` 不应为空

### `DerivedMetrics::validate()`

至少校验：

- `available == false` 时允许为空
- `available == true` 时 `metricsSchemaVersion` 非空

### `SimulationTaskResult::validate()`

至少校验：

- `resultSchemaVersion` 非空
- `taskId` 非空
- `startedAtUtcMs <= finishedAtUtcMs`
- `durationMs >= 0`
- `formationSource == ManualInput` 时，`presetFormationId` 必须为空
- `formationSource == PresetFormation` 时，`presetFormationId` 必须有值
- `Succeeded` 时：
  - `aggregatedField` 必须合法
  - `emitterResults` 至少有一项
- `Failed` 时 `errorMessage` 必须非空
- `Cancelled` 时允许没有场数据

## 结果 Schema 设计

### 文件

新增：

- `docs/schema/simulation-result.schema.json`
- `docs/schema/simulation-result.template.jsonc`

### 目标

这份 schema 主要承担：

- 固定字段名
- 固定字段类型
- 固定枚举值
- 固定可空策略
- 为后续导出结果 JSON 提供口径

本轮它不承担完整运行时解析链路，也不承担复杂跨字段一致性约束。

### 适合放在 schema 中的规则

- 顶层字段是否存在
- `formationSource` 枚举值
- `presetFormationId` 为 `integer` 或 `null`
- `aggregatedField.values` 为 number 数组
- `rows` / `cols` / `durationMs` 等基础类型与范围

### 不适合只靠 schema 解决的规则

以下规则应由 C++ `validate()` 负责：

- `rows * cols == values.size()`
- 状态与字段出现关系
- `formationSource` 与 `presetFormationId` 的联合语义
- `quantity` 与 `valueUnit` 的组合约束

## 文件组织

建议新增或拆分为：

- `Interface/SimulationResult.h`
- `Simulation/PEPropagationSolver.h`
- `Simulation/PEPropagationSolver.cpp`
- `Simulation/EMC_Engine.h`
- `Simulation/EMC_Engine.cpp`
- `Simulation/simSchedulerCtx.h`
- `Simulation/simSchedulerCtx.cpp`
- `docs/schema/simulation-result.schema.json`
- `docs/schema/simulation-result.template.jsonc`

## 对现有代码的影响

### `Simulation/EMC_Engine.*`

- 保留“多发射机计算与聚合”的角色
- 不再把 `_LossGrid` 作为唯一公开结果边界
- 后续可改为返回“计算原始结果集合”，交由 `simSchedulerCtx` 组装

### 当前 `Propagation_Engine`

- 后续改名为 `PEPropagationSolver`
- 不再承担任务级含义
- 专注单发射机数值求解

### `Resource/ui/Simulation.cpp`

后续改为：

- 启动 `simSchedulerCtx`
- 获取 `SimulationTaskResult`
- 主图从 `aggregatedField` 绘制
- 保留“结果是否过期”的快照比较逻辑
- 不再把 `_LossGrid` 当作唯一状态载体

### `Utils/PaintImage.hpp`

后续建议新增接受 `ScalarField2D` 的绘图入口，而不是直接接受 `GridMap`。

## 非目标

本轮不做：

- 不实现结果 JSON 回读
- 不实现 `DerivedMetrics` 具体四个指标字段
- 不实现预设编队改造逻辑本体
- 不实现报告生成
- 不处理验证侧与主项目的兼容

## 测试建议

后续实现时建议新增 Catch2 测试，覆盖：

1. `ScalarField2D` 尺寸与数组长度一致性校验
2. `SimulationTaskResult` 对 `formationSource / presetFormationId` 联合规则的校验
3. `Succeeded` / `Failed` / `Cancelled` 三类状态的字段约束
4. `simSchedulerCtx` 能把 `EMC_Engine` 原始计算结果组装为根结果对象
5. UI 从 `SimulationTaskResult.aggregatedField` 绘图而不是直接依赖裸 `GridMap`

## 后续推进顺序

1. 先新增 `Interface/SimulationResult.h`
2. 再拆分 `Propagation_Engine -> PEPropagationSolver`
3. 新增 `simSchedulerCtx` 组装任务级结果
4. 调整 `Simulation.cpp` 消费 `SimulationTaskResult`
5. 最后补 `simulation-result.schema.json`、样例和测试
