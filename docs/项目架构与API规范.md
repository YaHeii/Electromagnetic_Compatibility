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

### 4.10 Reportflow C++ 兼容层

当前 Reportflow 与主程序之间的 C++ 兼容层固定拆为三部分：

- `Interface/ReportFlowContract.h`
  - 统一定义 bundle 文件名、目录名、`request/status` 字段、`mode/state/stage` 枚举和 DTO
- `Utils/Reportflow/`
  - `ReportFlowJsonIO`：统一序列化 `simulation-result.json`、`request.json`、`status.json`
  - `StandardInputExporter`：把 `DataModel::DataSnapshot` 导出为外部标准输入 `baseline-input.jsonc`
  - `ReportflowCliBridge`：把一次 headless 仿真结果落盘为 `simulation-result.json + report-context.json`
- `entry/EMC_SimRunner.cpp`
  - 提供独立 CLI 入口，复用 `JsonLoader -> simSchedulerCtx -> SimulationTaskResult` 主链路

边界约束：

- `baseline-input.jsonc` 必须保持与主程序正式输入 schema 同口径
- 当前外部标准输入不表达 `equipmentRef.isEnabled`，因此若内部快照含禁用设备引用，`StandardInputExporter` 会直接拒绝导出
- `EMC_SimRunner` 只负责读取标准输入、执行仿真和落盘 JSON，不承担报告编排逻辑

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

## 6. Reportflow Json

### 6.1 收敛结论

当前 C++ 与 Python 之间的通信接口和过程已经**基本收敛**，第一版固定采用：

- `bundle 目录 + JSON/JSONC 文件协议 + headless runner`

不采用：

- 共享内存
- RPC 服务
- Qt / Python 直接嵌入式互调
- 运行时反射内部对象

这意味着当前跨层边界已经明确为：

- C++ 负责导出标准输入、正式结果、报告上下文和任务控制文件
- Python 负责读取 bundle、编排补充实验、调用 headless runner、更新状态并生成最终报告

当前尚未完全闭环的部分不是“接口未定”，而是 Python `agent-experiment` workflow 仍待继续实现。

### 6.2 契约与实现位置

当前接口实现固定分布如下：

- `Interface/ReportFlowContract.h`
  - 统一定义 bundle 文件名、目录名、`mode/state/stage` 枚举和 `request/status` DTO
- `Utils/Reportflow/ReportFlowJsonIO.*`
  - 统一负责 `simulation-result.json`、`request.json`、`status.json` 的 JSON 落盘
- `Utils/Reportflow/StandardInputExporter.*`
  - 负责把 `DataModel::DataSnapshot` 回写为标准输入 `baseline-input.jsonc`
- `Utils/Reportflow/ReportContextBuilder.*`
  - 负责从 `SimulationTaskResult` 生成 `report-context.json`
- `Utils/Reportflow/ReportflowCliBridge.*`
  - 负责把一次 headless 仿真结果导出为 `simulation-result.json + report-context.json`
- `Utils/Reportflow/ReportJobExporter.*`
  - 负责导出基准 bundle、初始化 `request.json / status.json`，并导出正式图片资产
- `entry/EMC_SimRunner.cpp`
  - 负责 headless 读取标准输入、执行仿真并输出 JSON

### 6.3 当前 JSON 文件协议

当前 C++ / Python 沟通所需 JSON 文件按职责分为四组。

基准 bundle 根目录：

- `request.json`
  - 描述任务模式、输入文件、图片资产、输出格式和 `agent` 配置
- `status.json`
  - 描述任务状态、当前阶段、时间戳和错误信息
- `baseline-input.jsonc`
  - 基准实验对应的标准外部输入 schema
- `simulation-result.json`
  - 主程序正式结果对象 `SimulationTaskResult`
- `report-context.json`
  - 面向报告与 agent 的摘要上下文

基准图片资产：

- `assets/aggregated-field.png`
- `assets/reference-emitter.png`
- `assets/scf-matrix.png`
- `assets/s3i-curve.png`
- `assets/t-elev.png`
- `assets/d-desense.png`

`agent-experiment` 扩展目录：

- `experiments/plan.json`
  - Python 侧生成的结构化实验计划
- `experiments/<experimentId>/input.jsonc`
  - Python 侧 materialize 后交给 `EMC_SimRunner` 的标准输入
- `experiments/<experimentId>/simulation-result.json`
  - 单次补充实验的正式结果
- `experiments/<experimentId>/report-context.json`
  - 单次补充实验的报告上下文

最终输出目录：

- `outputs/comparison-summary.json`
  - Python 侧生成的候选对比摘要
- `outputs/final-report.md`
- `outputs/final-report.html`

### 6.4 request.json / status.json 关键字段

`request.json` 当前关键字段固定为：

- `reportBundleVersion`
- `taskId`
- `mode`
  - `template-only`
  - `agent-experiment`
- `language`
- `templateId`
- `outputFormats`
- `inputFiles`
  - `simulationResult`
  - `reportContext`
  - `baselineInput`
- `assetFiles`
- `agent`
  - `goalMode`
  - `maxExperimentCount`
  - `mutationScopes`
  - `rankingPolicy`
  - `providerProfile`

`status.json` 当前关键字段固定为：

- `taskId`
- `state`
  - `pending`
  - `running`
  - `succeeded`
  - `failed`
  - `cancelled`
- `stage`
  - `validate_bundle`
  - `diagnose_baseline`
  - `plan_experiments`
  - `materialize_inputs`
  - `run_experiments`
  - `rank_candidates`
  - `render_markdown`
  - `render_html`
  - `completed`
- `updatedAtUtcMs`
- `startedAtUtcMs`
- `finishedAtUtcMs`
- `errorMessage`
- `errors`

### 6.5 当前通信过程

当前标准通信过程固定为：

1. C++ GUI 或主程序结果侧拿到一次成功仿真的 `SimulationTaskResult`
2. `ReportJobExporter` 导出基准 bundle：
   - `baseline-input.jsonc`
   - `simulation-result.json`
   - `report-context.json`
   - `request.json`
   - `status.json`
   - `assets/*`
3. Python `reportflow` 读取 bundle，先执行 `validate_bundle`
4. 若模式为 `template-only`
   - Python 直接消费 `simulation-result.json + report-context.json + assets/*`
   - 输出 `outputs/final-report.md/html`
5. 若模式为 `agent-experiment`
   - Python 基于 `baseline-input.jsonc` 生成 `experiments/<experimentId>/input.jsonc`
   - Python 调用：
     - `EMC_SimRunner --input <input.jsonc> --output-dir <experiment-dir>`
   - C++ runner 输出：
     - `simulation-result.json`
     - `report-context.json`
   - Python 收集所有候选结果，更新 `status.json`，最终输出：
     - `outputs/comparison-summary.json`
     - `outputs/final-report.md`
     - `outputs/final-report.html`

### 6.6 当前边界与限制

当前已明确的边界如下：

- `baseline-input.jsonc` 必须保持与主程序正式输入 schema 同口径
- 当前外部标准输入不表达 `equipmentRef.isEnabled`
  - 若内部快照含禁用设备引用，`StandardInputExporter` 会直接拒绝导出
- `EMC_SimRunner` 当前只输出 JSON，不负责导出正式图片资产
- Python 侧当前不回写 `simulation-result.json`，只新增实验目录结果、状态文件和最终报告
- 当前 bundle 协议已经收敛，但 Python `agent-experiment` 的完整运行时仍在后续落地中

## 7. 一致性维护要求

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

## 8. 2026-04-30 仿真页结果画廊边界
### 当前已实现
- 仿真页结果展示层开始从“单图直绘”收敛为“固定结果目录 + 统一绘图器 + 详情视图切换”。
- UI 只消费 `SimulationTaskResult`，不在页面点击或切换时直接调用 `EMCMetricsCalculator`。
- 结果目录当前固定为六类主图：
  - 总场分布
  - 参考发射机路径损耗
  - `SCF`
  - `S3I`
  - `T_elev`
  - `D_desense`
- 详情区按三类载荷切换：
  - `ScalarField2D`
  - `Series1D`
  - `LabeledMatrix2D`
- `SimulationResultCatalog` 负责从 `SimulationTaskResult` 抽取卡片目录与详情载荷。
- `SimulationChartRenderer` 负责把三类结果统一渲染到 `QCustomPlot`，并离屏生成卡片缩略图。

### 当前约束
- 参考发射机卡片只读取 `inputSnapshot.emcAnalysisConfig.referenceTransmitterId` 对应结果，不回退到其他发射机。
- `SCF` 详情图采用“矩阵式热图 + 文字标注”，不是普通表格控件。
- `S3I` 详情图采用双曲线 + 差值填充带。
- `noDataValue` 与 `NaN` 在场图中按透明单元格处理，不压成最低色。

### 建议后续实现
- 在主程序编译与手动联调确认后，再补结果页交互细节，例如详情统计摘要、阈值图例说明和导出入口。
- 若后续扩展更多图类，应优先复用 `SimulationResultCatalog + SimulationChartRenderer`，不要把新逻辑重新散回 `Simulation.cpp`。
## 10. Reportflow Python 工作区

### 当前已实现
- `Reportflow/` 下已建立独立的 Python 工作区根，当前成员包含 `establishReport` 和 `pythonPlot`。
- `establishReport` 是当前 `reportflow` CLI 与模板渲染包的目录，Python 导入包名保持为 `reportflow`。
- `pythonPlot` 预留给后续的 `matplotlib` 和 `pybind11` 绘图子项目。

### 当前边界
- Python 工作区使用独立虚拟环境 `Reportflow/.venv/`，不进入主程序 `CMake + vcpkg` 构建链路。
- C++ 主工程不直接依赖 Python 环境，也不把 Python 运行时当作编译期依赖。
- `reportflow` 负责 bundle 读取、状态更新和模板输出；`pythonPlot` 负责后续可选的绘图与扩展模块。

### 当前接口口径
- C++ 侧当前通过 `ReportFlowContract + bundle 目录` 约定 `request.json`、`status.json`、`baseline-input.jsonc`、`simulation-result.json`、`report-context.json`、`assets/*`、`experiments/*` 和 `outputs/*` 的布局。
- Python 侧消费 bundle，补充实验阶段只新增 `experiments/<experimentId>/input.jsonc`、候选结果目录、`status.json` 更新和最终输出，不回写基准 `simulation-result.json`。
- 最终报告输出固定写入 `outputs/final-report.md` 和 `outputs/final-report.html`。

### 建议后续实现
- 若后续要接入 `pybind11`，优先把编译逻辑限制在 `pythonPlot` 子项目内，不要回写到主 C++ 构建流程。
- 若需要从 C++ 触发 Python 绘图，优先采用外部进程调用或文件输出消费，不要直接把 Python 运行时嵌进主工程。
- 若后续 `reportflow` 增加 LLM 或多模板能力，先扩展 `report-context.json` 和 `request.json`，不要破坏当前 bundle 约定。
