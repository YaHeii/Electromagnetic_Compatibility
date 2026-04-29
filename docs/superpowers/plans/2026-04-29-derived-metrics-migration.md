# DerivedMetrics 正式迁回主程序设计

## Summary

- 当前已实现：主程序输入 schema 已正式承载 `environment`，`DataModel -> simSchedulerCtx -> EMC_Engine -> SimulationTaskResult` 的结果边界已稳定，`DerivedMetrics` 仍是壳结构。
- 验证侧已有：`SCF / S3I / T_elev / D_desense` 的定义、公式、图表口径和摘要统计已在 `PE_validation` 中沉淀。
- 本轮设计目标：把四个指标正式迁回主程序结果链路，同时把主链路残留硬编码参数改为从 `DataModel` 冻结快照读取，不把指标逻辑重新塞回 `EMC_Engine` 本体。

## Key Changes

### 1. 输入侧新增正式分析配置

新增顶层必填字段 `emcAnalysisConfig`，对应 `EMCAnalysisConfig`，纳入 `DataModel::DataSnapshot`，并同步更新主输入 schema、样例和文档。

字段固定为：

- `fieldPlaneHeightM`
- `referenceTransmitterId`
- `referenceReceiverId`
- `s3iBaselineWindSpeedMps`

设计约束：

- `emcAnalysisConfig` 为 schema 必填，不保留默认回退。
- `referenceTransmitterId`、`referenceReceiverId` 必须解析到启用设备。
- `referenceTransmitterId -> referenceReceiverId` 必须是跨平台链路。
- `fieldPlaneHeightM > 0`，且必须与当前垂直网格上限相容。
- `s3iBaselineWindSpeedMps >= 0`，`S3I` 当前海况直接使用 `environment.windSpeed`。

### 2. 职责边界保持分层，不把四个指标塞回 `EMC_Engine` 本体

采用三层职责：

- `EMC_Engine`
  - 继续只负责核心传播、按发射机求解和总场聚合。
  - 主链路里的 `25m` 接收平面硬编码改为消费 `emcAnalysisConfig.fieldPlaneHeightM`。
- 传播求解层
  - `PEPropagationSolver / PEModel` 里与主链路相关的 `25m` 近似参数改为显式传递观测平面/接收高度，不再依赖常量。
- 新增指标计算层
  - 在 `Simulation/` 下新增独立的 `EMCMetricsCalculator`。
  - 输入为冻结后的 `DataSnapshot`、`EMCAnalysisConfig` 和 `EMC_Engine` 原始结果。
  - 由 `simSchedulerCtx` 在主传播完成后统一触发，并写回 `SimulationTaskResult::derivedMetrics`。

这样四个指标仍然属于主程序正式计算链路，但传播引擎、任务编排、指标评估保持清晰分层。

### 3. `DerivedMetrics` 从壳结构升级为正式字段

`DerivedMetrics` 跟随 `resultSchemaVersion` 演进，删除单独的 `metricsSchemaVersion`。

第一版正式字段按“核心值 + 必要摘要 + 图表级原始数据”设计：

- `SCF`
  - 标量值
  - 热噪声底
  - 跨平台干扰链路数
  - 可直接绘图的耦合矩阵数据
- `S3I`
  - 标量值
  - `referenceTransmitterId`
  - `referenceReceiverId`
  - `baselineWindSpeedMps`
  - `currentWindSpeedMps`
  - 可直接绘图的距离轴、平静海况曲线、当前海况曲线
- `T_elev`
  - `ScalarField2D` 热图
  - `maxDb`
  - `meanDb`
- `D_desense`
  - `ScalarField2D` 热图
  - `victimReceiverId`，固定等于 `referenceReceiverId`
  - `peakDb`
  - `coveragePercent`
  - `adiDbPerSquareMeter`

为承载图表级数据，结果侧补充通用类型：

- 一个带标签的二维矩阵类型，用于 `SCF` 耦合矩阵
- 一个一维序列类型，用于 `S3I` 曲线

`SimulationTaskResult.inputSnapshot` 自动带上 `emcAnalysisConfig`，不在 `DerivedMetrics` 里重复保存同一份输入配置。

### 4. 计算入口与口径

四个指标默认在每次成功仿真后都计算，不做可选开关。

计算口径固定为：

- `SCF`
  - 遍历全部跨平台 TX/TRX 发射端到 RX/TRX 接收端的非目标链路
- `S3I`
  - 仅对 `referenceTransmitterId -> referenceReceiverId` 这条跨平台参考链路计算
  - 比较 `s3iBaselineWindSpeedMps` 与 `environment.windSpeed`
- `T_elev`
  - 基于主结果平面 `fieldPlaneHeightM` 的总干扰场生成
- `D_desense`
  - 基于同一平面场和 `referenceReceiverId` 的接收参数生成

验证侧 notebook 不修改；主程序只迁移口径与能力，不依赖验证侧运行时资产。

## Test Plan

- 输入/Schema
  - `emcAnalysisConfig` 缺失时报错
  - 四个字段类型、必填性、值域正确校验
  - `referenceTransmitterId`、`referenceReceiverId` 可解析且启用
  - 参考链路必须跨平台
  - `fieldPlaneHeightM` 超出垂直网格上限时报错
- 计算链路
  - 主仿真不再使用 `receiverHeight = 25.0` 常量
  - 与主链路相关的 `25m` 近似不再写死在传播求解路径
  - `simSchedulerCtx` 成功态会组装完整 `DerivedMetrics`
- 结果对象
  - `DerivedMetrics` 校验覆盖四类指标字段完整性
  - `SCF` 矩阵、`S3I` 曲线、`T_elev/D_desense` 场图都可通过结果校验
  - `inputSnapshot` 中保留 `emcAnalysisConfig`
- 测试组织
  - 使用 Catch2，放在 `Tests/`
  - 同步更新测试目标接入

## Assumptions

- `emcAnalysisConfig` 是正式输入的一部分，不做旧格式兼容。
- `D_desense` 第一版只支持单受害接收机，且受害机固定为 `referenceReceiverId`。
- `DerivedMetrics` 不再单独维护 `metricsSchemaVersion`，统一跟随 `resultSchemaVersion`。
- 图表级原始数据本轮一起进入结果对象，后续 UI/报告直接消费，不再为画图二次重算。



按这份 plan 对照，当前不是“未开始”，也不是“已完成”。主干已经做到 `DataModel / DerivedMetrics 结构 / 指标计算层 / 调度写回 / 大部分测试`，但最关键的主链路平面高度接入还没收口，`docs/schema` 和项目文档也还停在旧口径，所以现在应判定为“部分完成”。

我之前删掉的文件已经恢复并确认都在工作区里，包括 [Simulation/EMCMetricsCalculator.cpp](/d:/code/Electromagnetic_compatibility/Simulation/EMCMetricsCalculator.cpp:1)、[Simulation/simSchedulerCtx.cpp](/d:/code/Electromagnetic_compatibility/Simulation/simSchedulerCtx.cpp:1)、[ModelView/T_TreeViewModel.cpp](/d:/code/Electromagnetic_compatibility/ModelView/T_TreeViewModel.cpp:1)、[Resource/ui/EnvironmentWidget.cpp](/d:/code/Electromagnetic_compatibility/Resource/ui/EnvironmentWidget.cpp:1)、[Tests/TreeViewModelTests.cpp](/d:/code/Electromagnetic_compatibility/Tests/TreeViewModelTests.cpp:1)、[Tests/Test.jsonc](/d:/code/Electromagnetic_compatibility/Tests/Test.jsonc:1)、[Interface/DataModel.h](/d:/code/Electromagnetic_compatibility/Interface/DataModel.h:1)、[Interface/SimulationResult.h](/d:/code/Electromagnetic_compatibility/Interface/SimulationResult.h:1)。

**当前已实现**
- `emcAnalysisConfig` 已进入运行时链路：字段常量、JSON 解析、`DataSnapshot`、快照校验都已经落地。[Interface/SchemaConstants.h](/d:/code/Electromagnetic_compatibility/Interface/SchemaConstants.h:10) [Utils/JsonLoader.hpp](/d:/code/Electromagnetic_compatibility/Utils/JsonLoader.hpp:109) [Interface/DataModel.h](/d:/code/Electromagnetic_compatibility/Interface/DataModel.h:317)
- `DataModel::validateSnapshot()` 已覆盖值域、垂直网格上限、启用设备解析和跨平台参考链路校验。[Interface/DataModel.h](/d:/code/Electromagnetic_compatibility/Interface/DataModel.h:381)
- `DerivedMetrics` 运行时结构已经不是壳了，`SCF / S3I / T_elev / D_desense` 正式字段都在，代码里也没有 `metricsSchemaVersion`。[Interface/SimulationResult.h](/d:/code/Electromagnetic_compatibility/Interface/SimulationResult.h:194) [Interface/SimulationResult.h](/d:/code/Electromagnetic_compatibility/Interface/SimulationResult.h:291)
- 四指标计算被放在独立层，没有塞回 `EMC_Engine`；成功仿真后由 `simSchedulerCtx` 统一计算并写回结果对象，这个职责边界符合 plan。[Simulation/EMCMetricsCalculator.cpp](/d:/code/Electromagnetic_compatibility/Simulation/EMCMetricsCalculator.cpp:136) [Simulation/simSchedulerCtx.cpp](/d:/code/Electromagnetic_compatibility/Simulation/simSchedulerCtx.cpp:113)
- UI 和只读总览已经接入 `emcAnalysisConfig`。[Resource/ui/EnvironmentWidget.cpp](/d:/code/Electromagnetic_compatibility/Resource/ui/EnvironmentWidget.cpp:164) [ModelView/T_TreeViewModel.cpp](/d:/code/Electromagnetic_compatibility/ModelView/T_TreeViewModel.cpp:172)
- 测试已经补到 loader、snapshot 校验、结果对象校验、scheduler 组装、UI、TreeView；测试目标也接进了 CMake。[Tests/SchemaDtoValidationTests.cpp](/d:/code/Electromagnetic_compatibility/Tests/SchemaDtoValidationTests.cpp:346) [Tests/SimulationResultTests.cpp](/d:/code/Electromagnetic_compatibility/Tests/SimulationResultTests.cpp:210) [Tests/SimulationSchedulerTests.cpp](/d:/code/Electromagnetic_compatibility/Tests/SimulationSchedulerTests.cpp:145) [CMakeLists.txt](/d:/code/Electromagnetic_compatibility/CMakeLists.txt:99)

**部分完成 / 有风险**
- `PEPropagationSolver` 已经支持显式传入接收高度，但主仿真入口并没有把 `fieldPlaneHeightM` 接进去；[Simulation/EMC_Engine.cpp](/d:/code/Electromagnetic_compatibility/Simulation/EMC_Engine.cpp:163) 现在仍然写死 `receiverHeight = 0.0`。这意味着 plan 里“用 `emcAnalysisConfig.fieldPlaneHeightM` 替换主链路平面高度硬编码”还没真正完成。
- 因为这一点，当前 `aggregatedField`、各发射机 `field2D`，以及建立在它们之上的 `T_elev / D_desense`，都还不能证明是在用户配置的结果平面上算出来的。
- 你的额外约束“校验提示和字符串必须使用中文”目前也没收尾，核心校验消息仍有英文。[Interface/DataModel.h](/d:/code/Electromagnetic_compatibility/Interface/DataModel.h:325) [Interface/SimulationResult.h](/d:/code/Electromagnetic_compatibility/Interface/SimulationResult.h:106)

**未完成**
- 输入文档 schema 没同步。运行时代码已经强制要求 `emcAnalysisConfig`，但 [docs/schema/usv-environment.schema.json](/d:/code/Electromagnetic_compatibility/docs/schema/usv-environment.schema.json:8) 和 [docs/schema/usv-environment.template.jsonc](/d:/code/Electromagnetic_compatibility/docs/schema/usv-environment.template.jsonc:1) 还是旧格式。
- 结果文档 schema/template 也没同步。[docs/schema/simulation-result.schema.json](/d:/code/Electromagnetic_compatibility/docs/schema/simulation-result.schema.json:231) 和 [docs/schema/simulation-result.template.jsonc](/d:/code/Electromagnetic_compatibility/docs/schema/simulation-result.template.jsonc:131) 还保留 `metricsSchemaVersion` / `note` 占位，没有正式的四指标结构，也没把 `inputSnapshot.emcAnalysisConfig` 写进去。
- 总览文档仍在描述“`DerivedMetrics` 是壳结构/后续实现”。[docs/项目概览.md](/d:/code/Electromagnetic_compatibility/docs/项目概览.md:115) [docs/项目架构与API规范.md](/d:/code/Electromagnetic_compatibility/docs/项目架构与API规范.md:317) [docs/风险与改进建议.md](/d:/code/Electromagnetic_compatibility/docs/风险与改进建议.md:215)

**验证侧已有**
- `PE_validation` 里四个指标的定义、口径和图表结论仍然在，和 plan 的假设一致。当前主程序已经有迁回实现骨架，但还没把“结果平面高度配置”这一段主链路打通。

**建议后续实现**
- 第一优先级：把 [Simulation/EMC_Engine.cpp](/d:/code/Electromagnetic_compatibility/Simulation/EMC_Engine.cpp:163) 改成真正消费 `inputSnapshot.emcAnalysisConfig.fieldPlaneHeightM`，并补一条能证明主结果平面随这个值变化的 Catch2 测试。
- 第二优先级：同步 `docs/schema/*` 和三份项目文档，消除 runtime 与文档分叉。
- 第三优先级：把新加的校验提示统一改成中文。

我没有运行 `cmake` 或测试；以上结论基于代码审计。