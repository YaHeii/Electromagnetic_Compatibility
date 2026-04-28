# 项目架构与 API 规范

## 1. 架构总览

当前仓库可以拆成六层，其中前五层属于主程序链路，第六层属于验证侧子模块：

| 层级 | 主要目录 | 作用 | 当前状态 |
| --- | --- | --- | --- |
| 输入层 | `Tests/`、`Resource/ui/` | 提供 JSONC 配置与界面输入 | 主程序已接入新 schema |
| 数据建模层 | `Interface/` | 保存 `DataModel`、快照、schema 常量 | 已实现 |
| 领域对象层 | `Models/` | 表达船只、设备、天线、编队对象 | 已实现 |
| 仿真调度层 | `Simulation/` | 负责环境建模、PE 求解、EMC 调度 | 已实现 |
| 结果展示层 | `Resource/ui/`、导出逻辑 | 展示二维结果与日志 | 已实现基础能力 |
| 验证侧资产层 | `PE_validation/` 子模块 | notebook、实验设计、图表、论文式结论 | 验证侧已有，主程序未直接依赖 |

主程序真正的运行主链路是：

```text
JSONC / UI
  -> DataModel
  -> TransferToEngine / TransferToPEdata
  -> EMC_Engine / Propagation_Engine / PEModel
  -> GridMap / UI 展示
```

`PE_validation` 不参与主程序构建，也不参与当前运行时接口解析。它的价值在于定义验证口径与后续功能迁移目标。

## 2. 主程序运行链路

当前一次典型仿真流程如下：

1. `Resource/ui/Simulation.cpp` 在 `Simulation::on_StartSimulate_clicked()` 中触发一次仿真。
2. 该入口当前直接调用 `JsonLoader::LoadFile()` 读取 `Tests/Test.jsonc`，但文件路径仍为硬编码绝对路径。
3. `Utils/JsonLoader.hpp` 读取 JSONC 文本，移除单行 `//` 注释，按新 schema 严格解析。
4. 解析结果写入 `DataModel::instance()`，包含：
   - `allShips`
   - `allEquipments`
   - `environmentConfig`
5. `DataModel::createSnapshot()` 创建一次仿真快照，供后续线程安全读取。
6. `TransferToEngine::convertDataModelToFleet()` 将 DTO 转换为 `Fleet` 及其内部 `ship / Equipment / Antenna` 对象。
7. `EMC_Engine::do_PE_computing()` 调度所有发射机，调用 `Propagation_Engine::PEmodel_computing2D()` 计算二维传播结果。
8. 结果以 `GridMap` 形式回传 UI，并由 `QCustomPlot` 绘制。

这条链路当前已经打通，但配置入口仍偏实验性质，不适合作为最终用户接口。

## 3. 标准输入 schema

### 3.1 标准来源

主程序输入标准当前由三处共同约束：

- `Interface/SchemaConstants.h`
  - 字段名、固定字符串、枚举值的单一来源
- `Utils/JsonLoader.hpp`
  - 当前运行时真实生效的解析与校验规则
- `docs/schema/usv-environment.schema.json`
  - 面向文档和自动校验工具的显式 schema

如果三者冲突，应优先修正为一致，不能让文档长期偏离运行时代码。

### 3.2 顶层结构

当前主程序只支持如下顶层结构：

```json
{
  "schemaVersion": "1.0.0",
  "environment": {},
  "usvs": []
}
```

约束如下：

- `schemaVersion` 必须为 `"1.0.0"`
- `environment` 必须存在
- `usvs` 必须存在且至少包含一艘船
- 未声明字段会被 `JsonLoader` 直接判定为非法

### 3.3 环境参数接口

`environment` 节点与 `EnvironmentData` 对应关系如下：

| schema 字段 | 内部字段 | 单位 | 说明 |
| --- | --- | --- | --- |
| `maxRange` | `max_range` | m | 最大传播距离 |
| `ductHeight` | `duct_height` | m | 蒸发波导高度 |
| `windSpeed` | `wind_speed` | m/s | 海面风速 |
| `dx` | `dx` | m | 水平方向步进 |
| `dz` | `dz` | m | 垂直分辨率 |
| `nz` | `nz` | 无量纲 | 垂直网格数 |
| `angleStepDeg` | `angle_step_deg` | deg | 2D 仿真角度步进 |

当前要求：

- `maxRange`、`dx`、`dz` 必须大于 `0`
- `ductHeight`、`windSpeed` 不能为负
- `nz`、`angleStepDeg` 必须为正整数
- `angleStepDeg` 取值范围为 `1` 到 `360`

### 3.4 USV 与设备接口

`usvs` 为数组，每个元素代表一艘船，字段如下：

| 字段 | 类型 | 单位 | 说明 |
| --- | --- | --- | --- |
| `ID` | string | - | 船只唯一标识 |
| `location.type` | string | - | 固定为 `Point3D` |
| `location.coordinates` | number[3] | m | 船只世界坐标 `[x, y, z]` |
| `speed` | number | m/s | 船速 |
| `shipOrientationDeg` | number | deg | 船在二维平面的朝向角，范围 `0~360` |
| `transmitters` | array | - | 发射机列表 |
| `receivers` | array | - | 接收机列表 |

坐标与角度语义约定：

- 世界坐标原点位于场景左下角
- `x` 轴向右，`y` 轴向上，`z` 轴向上
- 设备 `locationOffset` 表示相对于船体中心的偏移
- `antennaPhiDeg` 表示相对于正 `z` 轴向下倾斜的角度，范围 `0~180`

发射机字段如下：

| 字段 | 单位 | 说明 |
| --- | --- | --- |
| `ID` | - | 设备唯一标识，当前要求全局唯一 |
| `type` | - | 固定为 `TRANSMITTER` |
| `gainDbi` | dBi | 增益 |
| `locationOffset` | m | 相对船体中心的三维偏移 |
| `centerFrequencyGHz` | GHz | 中心频率 |
| `bandwidthMHz` | MHz | 带宽 |
| `powerDbm` | dBm | 发射功率 |
| `antennaPhiDeg` | deg | 下倾角 |
| `beamWidthDeg` | deg | 波束宽度，范围 `0~360` |
| `polarization` | - | `VERTICAL` 或 `HORIZONTAL` |
| `antennaType` | - | `OMNI`、`DIRECTIONAL`、`HORN`、`SHAPED_BEAM`、`REFLECTOR` |

接收机字段如下：

| 字段 | 单位 | 说明 |
| --- | --- | --- |
| `ID` | - | 设备唯一标识，当前要求全局唯一 |
| `type` | - | 固定为 `RECEIVER` |
| `gainDbi` | dBi | 增益 |
| `locationOffset` | m | 相对船体中心的三维偏移 |
| `centerFrequencyGHz` | GHz | 中心频率 |
| `bandwidthMHz` | MHz | 带宽 |
| `sensitivityDbm` | dBm | 灵敏度，当前标准固定为负值 |
| `interferenceMarginDb` | dB | 干扰容限或等效带外抑制 |
| `sinrMarginDb` | dB | SINR 裕量 |
| `noiseFigureDb` | dB | 噪声系数 |

### 3.5 样例文件状态

| 文件 | 用途 | 状态 |
| --- | --- | --- |
| `Tests/Test.jsonc` | 主程序当前标准样例 | 当前已实现 |
| `Tests/Test_A.jsonc` | 历史场景样例 | 历史参考，不是主程序标准输入 |
| `Tests/Test_B.jsonc` | 历史场景样例 | 历史参考，不是主程序标准输入 |
| `PE_validation/` 内相关样例 | 验证侧实验资产 | 验证侧已有，主程序不直接解析 |

## 4. 内部数据模型与 API 边界

### 4.1 `DataModel`

`DataModel` 是主程序当前的全局数据容器，内部包含：

- `std::vector<ShipData> allShips`
- `std::vector<EquipmentData> allEquipments`
- `EnvironmentData environmentConfig`

注意：

- `DataModel` 内部成员命名仍保留大量历史字段名，例如 `CentralF_Reciever`、`ship_Orienteation`
- 新 schema 字段会在 `JsonLoader` 中映射到这些旧成员
- 这意味着“外部配置命名”与“内部 DTO 命名”目前不是同一套口径

### 4.2 `JsonLoader::LoadFile`

职责：

- 读取 JSONC 文本
- 去除单行 `//` 注释
- 校验顶层结构、环境字段、船只字段、设备字段
- 拒绝未知字段
- 保证船只 ID 与设备 ID 当前全局唯一
- 写回 `DataModel::instance()`

当前边界：

- 只支持新 schema
- 不保留旧格式兼容逻辑
- 是主程序标准输入的唯一解析入口

### 4.3 `DataModel::createSnapshot`

职责：

- 将当前全局数据复制为一次仿真快照
- 为仿真线程提供稳定输入边界

约定：

- 仿真链路不应在运行中反向修改 `DataModel`
- 新增仿真任务接口时，应继续沿用快照边界，避免 UI 状态被后台线程直接读写

### 4.4 `TransferToEngine::convertDataModelToFleet`

职责：

- 将 DTO 快照转换为 `Fleet`
- 将 `ShipData`、`EquipmentData` 变为领域层对象
- 为后续 `EMC_Engine` 与 `PEModel` 提供可计算输入

注意：

- 当前转换阶段仍需依赖旧 DTO 命名
- 如果未来直接暴露主程序 API，建议新增面向 schema 的中间对象，减少转换期语义漂移

### 4.5 `EMC_Engine` 与 `Propagation_Engine`

职责：

- `EMC_Engine::do_PE_computing()`：调度所有发射机仿真并汇总结果
- `Propagation_Engine::PEmodel_computing2D()`：计算单发射机二维传播结果
- `PEModel`：负责传播步进、表面模型和路径损耗求解

当前输出：

- 主程序输出核心仍是二维 `GridMap`
- EMC 指标、报告对象、结构化评估结果尚未成为主程序稳定 API

## 5. 对外接口现状

### 当前已实现

- Qt UI 录入船只、设备和仿真启动操作
- 新 JSONC schema 配置加载
- 二维传播结果绘图与日志输出

### 验证侧已有

- `PE_validation` 子模块中的实验 notebook
- 实验 1 到实验 3 的图表、设计说明和结论汇总
- A/B 编队场景与 EMC 指标验证口径

### 建议后续实现

- 配置文件选择器或仓库相对路径入口
- 结构化 EMC 指标 API
- 阵型模板接口
- 统一结果导出与报告生成接口

## 6. 一致性维护要求

后续只要修改输入字段、单位或枚举，必须同步更新以下位置：

1. `Interface/SchemaConstants.h`
2. `Utils/JsonLoader.hpp`
3. `Tests/Test.jsonc`
4. `docs/schema/usv-environment.schema.json`
5. `docs/schema/usv-environment.template.jsonc`
6. 本文档与 `docs/风险与改进建议.md`

当前最重要的架构纪律不是继续扩展 UI，而是保持“schema、运行时代码、验证结论”三者同口径。否则即使编译通过，也会在后续指标迁移和报告生成阶段反复出现语义分叉。
