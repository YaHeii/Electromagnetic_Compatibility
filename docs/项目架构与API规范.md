# 项目架构与API规范
**注意: 不需要修改验证侧代码, 不需要处理验证侧与主项目的兼容性, 验证侧仅用于上下文参考**

## 1. 架构概览

当前仓库可以按主程序运行链路拆成六层：

| 层级 | 主要目录 | 职责 |
| --- | --- | --- |
| 输入层 | `Tests/`、`Resource/ui/` | 提供 JSONC 样例与 Qt 编辑界面 |
| DTO / schema 层 | `Interface/` | 维护 `DataModel`、快照结构、schema 常量与转换接口 |
| 领域对象层 | `Models/` | 表达船只、设备、天线、编队等领域对象 |
| 仿真求解层 | `Simulation/` | 负责传播求解、总场聚合、派生指标计算与任务调度 |
| 展示层 | `Resource/ui/` | 消费结构化结果对象，展示场图与任务状态 |
| 验证侧资产层 | `PE_validation/` | 保存 notebook、实验设计、图表和验证结论 |

`PE_validation/` 不参与主程序构建，也不是运行时依赖。它的作用是提供验证口径和后续迁移的上下文参考。

## 2. 主程序运行链路

当前主程序的主链路为：

```text
JSONC / UI
  -> DataModel::DataSnapshot
  -> DataModel::validateSnapshot()
  -> TransferToEngine::convertDataModelToFleet()
  -> EMC_Engine
  -> PEPropagationSolver / PEModel
  -> EMCComputationResult
  -> EMCMetricsCalculator
  -> SimulationTaskResult
  -> UI / 后续报告 / 后续导出
```

关键说明：

- JSON 导入和 UI 编辑都会先收敛到同一份 `DataModel::DataSnapshot`
- 语义校验统一收口到 `DataModel::validateSnapshot()`
- `EMC_Engine` 只负责多发射机传播与总场聚合，不承接四个派生指标业务逻辑
- `EMCMetricsCalculator` 在成功仿真后统一计算 `SCF / S3I / T_elev / D_desense`
- UI 已不再直接消费裸 `GridMap`，而是消费 `SimulationTaskResult`

## 3. 标准输入 schema

### 3.1 统一来源

主程序输入标准由以下位置共同约束：

1. `Interface/SchemaConstants.h`
2. `Utils/JsonLoader.hpp`
3. `docs/schema/usv-environment.schema.json`
4. `docs/schema/usv-environment.template.jsonc`

其中：

- 字段名、固定值和枚举值以 `SchemaConstants.h` 为准
- 运行时解析与基础格式校验以 `JsonLoader.hpp` 为准
- 文档 schema 与模板必须与运行时代码保持同步

### 3.2 顶层结构

当前标准输入顶层结构固定为：

```json
{
  "schemaVersion": "1.0.0",
  "environment": {},
  "emcAnalysisConfig": {},
  "usvs": []
}
```

约束如下：

- `schemaVersion` 必须为 `"1.0.0"`
- `environment` 为必填
- `emcAnalysisConfig` 为必填
- `usvs` 为必填，且必须至少包含一艘船
- 未声明字段会被 `JsonLoader` 直接判定为非法

### 3.3 环境参数

`environment` 对应 `EnvironmentData`：

| schema 字段 | 内部字段 | 单位 | 说明 |
| --- | --- | --- | --- |
| `maxRange` | `maxRange` | m | 最大传播距离 |
| `ductHeight` | `ductHeight` | m | 蒸发波导高度 |
| `windSpeed` | `windSpeed` | m/s | 当前海况风速 |
| `dx` | `dx` | m | 水平步进 |
| `dz` | `dz` | m | 垂直分辨率 |
| `nz` | `nz` | - | 垂直网格数 |
| `angleStepDeg` | `angleStepDeg` | deg | 2D 方位角步进 |

运行时约束：

- `maxRange`、`dx`、`dz` 必须大于 `0`
- `ductHeight`、`windSpeed` 不能为负
- `nz` 必须为正整数
- `angleStepDeg` 必须位于 `[1, 360]`

### 3.4 EMC 分析配置

`emcAnalysisConfig` 对应 `EMCAnalysisConfig`：

| schema 字段 | 内部字段 | 单位 | 说明 |
| --- | --- | --- | --- |
| `fieldPlaneHeightM` | `fieldPlaneHeightM` | m | 主结果平面高度 |
| `referenceTransmitterId` | `referenceTransmitterId` | - | 参考发射机 ID |
| `referenceReceiverId` | `referenceReceiverId` | - | 参考接收机 ID |
| `s3iBaselineWindSpeedMps` | `s3iBaselineWindSpeedMps` | m/s | S3I 基准海况风速 |

运行时约束：

- `fieldPlaneHeightM > 0`
- `fieldPlaneHeightM` 不能超过 `environment.dz * (environment.nz - 1)`
- `referenceTransmitterId` 必须解析到启用的发射机或收发一体机发射端
- `referenceReceiverId` 必须解析到启用的接收机或收发一体机接收端
- `referenceTransmitterId -> referenceReceiverId` 必须是跨平台链路
- `s3iBaselineWindSpeedMps >= 0`

### 3.5 USV 与设备输入

`usvs` 数组中的每个元素表示一艘船，主要字段为：

- `ID`
- `location`
- `speed`
- `shipOrientationDeg`
- `transmitters`
- `receivers`
- `transceivers`

设备侧统一使用：

- `TRANSMITTER`
- `RECEIVER`
- `TRANSCEIVER`

收发一体机采用嵌套 `transmitter / receiver` 子对象，避免把发射参数与接收参数摊平到同一层。

## 4. 核心对象与职责

### 4.1 `DataModel`

`DataModel` 是主程序的全局 DTO 容器，当前核心成员包括：

- `std::vector<EquipmentData> allEquipments`
- `std::vector<ShipData> allShips`
- `EnvironmentData environmentConfig`
- `EMCAnalysisConfig emcAnalysisConfig`

`DataModel::DataSnapshot` 是仿真任务冻结输入的标准结构。
`DataModel::validateSnapshot()` 统一负责：

- 环境参数值域校验
- `emcAnalysisConfig` 联合校验
- 设备与船只 ID 唯一性
- 船载设备引用一致性
- 参考发射机 / 接收机解析
- 跨平台参考链路约束

### 4.2 `JsonLoader::LoadFile`

职责：

- 读取 JSONC 文本
- 去除单行注释
- 拒绝未知字段
- 解析顶层对象、环境参数、分析配置、船只与设备
- 调用 `DataModel::validateSnapshot()` 做核心语义校验
- 成功后回写 `DataModel::instance()`

边界：

- 当前只支持新 schema
- 不保留旧格式兼容逻辑

### 4.3 UI 编辑页

当前环境页、设备页、船只页统一遵循：

- `loadFromModel()`
- `saveToModel()`
- `setReadOnly(bool)`
- `dirty` 状态

分层约束：

- UI 只做基础格式校验和 DTO 组装
- 核心语义校验统一交给 `DataModel::validateSnapshot()`
- 导入成功后统一回写 `DataModel`，再回填页面草稿

### 4.4 `TreeView` / `T_TreeViewModel`

职责：

- 从 `DataModel` 生成只读总览树
- 展示环境参数、分析配置、船只和设备信息
- 提供刷新、展开、折叠和关键字查找

边界：

- 当前只承担只读投影
- 不支持编辑、删除、跳转或导航编排

### 4.5 `TransferToEngine::convertDataModelToFleet`

职责：

- 把 `DataSnapshot` 转换为 `Fleet`
- 把 `ShipData / EquipmentData` 转为领域对象
- 为仿真引擎提供可计算输入

### 4.6 `PEPropagationSolver`

职责：

- 单发射机 1D / 2D PE 求解
- 支持显式传入接收高度 / 结果平面高度
- 返回路径损耗场或路径损耗曲线

边界：

- 不关心任务状态、结果对象和派生指标

### 4.7 `EMC_Engine`

职责：

- 持有冻结后的 `Fleet` 和 `DataSnapshot`
- 调度全部发射机的传播求解
- 聚合总场
- 输出 `EMCComputationResult`

边界：

- 不直接计算 `SCF / S3I / T_elev / D_desense`
- 主结果平面高度通过 `inputSnapshot.emcAnalysisConfig.fieldPlaneHeightM` 输入

### 4.8 `EMCMetricsCalculator`

职责：

- 消费 `DataSnapshot + EMCComputationResult`
- 统一计算四个派生指标
- 输出完整 `DerivedMetrics`

边界：

- 不负责传播求解
- 不读取全局 `DataModel`
- 只消费冻结快照与原始结果

### 4.9 `simSchedulerCtx`

职责：

- 冻结任务输入
- 构建 `Fleet`
- 驱动 `EMC_Engine`
- 在成功仿真后调用 `EMCMetricsCalculator`
- 组装 `SimulationTaskResult`

## 5. 结果对象 API

### 5.1 `SimulationTaskResult`

任务级根结果对象，主要字段包括：

- 任务元数据：`taskId`、`modelType`、`status`
- 输入来源：`formationSource`、`presetFormationId`
- 时间信息：`startedAtUtcMs`、`finishedAtUtcMs`、`durationMs`
- 结果说明：`errorMessage`、`summaryText`
- 冻结输入：`inputSnapshot`
- 主场图：`aggregatedField`
- 单发射机结果：`emitterResults`
- 派生指标：`derivedMetrics`

### 5.2 `ScalarField2D`

通用二维标量场类型，当前正式承载：

- `AggregatedPowerDbm`
- `PathLossDb`
- `NoiseElevationDb`
- `DesenseDb`

### 5.3 `EmitterResult`

单发射机结果对象，承载：

- 发射机与船只 ID
- 发射机参数摘要
- `field2D`
- 失败时的 `errorMessage`

### 5.4 `DerivedMetrics`

当前已正式承载四类指标：

- `scf`
  - `scalarDb`
  - `thermalNoiseFloorDbm`
  - `linkCount`
  - `couplingMatrix`
- `s3i`
  - `scalarDb`
  - `referenceTransmitterId`
  - `referenceReceiverId`
  - `baselineWindSpeedMps`
  - `currentWindSpeedMps`
  - `calmCurve`
  - `currentCurve`
- `tElev`
  - `field`
  - `maxDb`
  - `meanDb`
- `dDesense`
  - `field`
  - `victimReceiverId`
  - `peakDb`
  - `coveragePercent`
  - `adiDbPerSquareMeter`

约束说明：

- `DerivedMetrics` 不再单独维护 `metricsSchemaVersion`
- `DerivedMetrics` 跟随 `resultSchemaVersion` 演进
- `inputSnapshot` 中已经保留 `emcAnalysisConfig`，结果侧不再重复保存同一份配置

## 6. 一致性维护要求

只要修改以下任一内容，就必须同步更新文档和样例：

1. `Interface/SchemaConstants.h`
2. `Utils/JsonLoader.hpp`
3. `Tests/Test.jsonc`
4. `docs/schema/usv-environment.schema.json`
5. `docs/schema/usv-environment.template.jsonc`
6. `docs/schema/simulation-result.schema.json`
7. `docs/schema/simulation-result.template.jsonc`
8. 本文档

当前最重要的纪律不是继续堆叠页面，而是保持以下三者长期同口径：

1. schema 文档
2. 主程序运行时代码
3. `PE_validation` 的验证结论
