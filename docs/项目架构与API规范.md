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
| 验证侧资产层 | `PE_validation/` 子模块 | notebook、实验设计、图表、验证结论 | 验证侧已有，主程序不直接依赖 |

主程序的实际运行主链路是：

```text
JSONC / UI
  -> DataModel
  -> TransferToEngine / TransferToPEdata
  -> EMC_Engine / Propagation_Engine / PEModel
  -> GridMap / UI 展示
```

`PE_validation` 不参与主程序构建，也不参与当前运行时接口解析。它的价值在于定义验证口径与后续能力迁移目标。

## 2. 主程序运行链路

当前一条典型仿真流程如下：

1. UI 或 JSON 输入整理为 `DataModel::DataSnapshot`
2. 接口层执行基础校验：
   - JSON 路径：`JsonLoader`
   - UI 路径：各页面控件负责把文本整理成 DTO，并只做基础格式校验
3. 核心语义校验统一收口到 `DataModel::validateSnapshot()`
4. `TransferToEngine::convertDataModelToFleet()` 把快照转换为 `Fleet`
5. `EMC_Engine::do_PE_computing()` 调度传播计算
6. 结果以 `GridMap` 形式回传 UI 并绘制

当前需要注意：

- `Simulation.cpp` 现在统一消费当前 `DataModel` 快照，JSON 与 UI 两条输入路径在运行前都会收敛到同一份 DTO
- 当前仍缺少“选择外部 JSON 文件并加载”的显式 UI 入口；如需走 JSON 文件路径，仍需在入口处调用 `JsonLoader::LoadFile()`

## 3. 标准输入 schema

### 3.1 标准来源

主程序输入标准由三处共同约束：

- `Interface/SchemaConstants.h`
  - 字段名、固定字符串、枚举值的单一来源
- `Utils/JsonLoader.hpp`
  - 当前运行时真实生效的解析与基础校验规则
- `docs/schema/usv-environment.schema.json`
  - 面向文档和工具的显式 schema

如三者冲突，应优先修正为一致，不能让文档长期偏离运行时代码。

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
| `maxRange` | `maxRange` | m | 最大传播距离 |
| `ductHeight` | `ductHeight` | m | 蒸发波导高度 |
| `windSpeed` | `windSpeed` | m/s | 海面风速 |
| `dx` | `dx` | m | 水平方向步进 |
| `dz` | `dz` | m | 垂直分辨率 |
| `nz` | `nz` | - | 垂直网格数 |
| `angleStepDeg` | `angleStepDeg` | deg | 2D 仿真角度步进 |

当前要求：

- `maxRange`、`dx`、`dz` 必须大于 `0`
- `ductHeight`、`windSpeed` 不能为负
- `nz`、`angleStepDeg` 必须为正整数
- `angleStepDeg` 范围为 `1` 到 `360`

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
| `transceivers` | array | - | 收发一体机列表 |

坐标与角度语义约定：

- 世界坐标原点位于场景左下角
- `x` 轴向右，`y` 轴向上，`z` 轴向上
- 设备 `locationOffset` 表示相对船体中心的偏移
- `antennaPhiDeg` 表示相对正 `z` 轴向下倾斜的角度，范围 `0~180`

发射机字段如下：

| 字段 | 单位 | 说明 |
| --- | --- | --- |
| `ID` | - | 设备唯一标识，当前按全局唯一处理 |
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
| `ID` | - | 设备唯一标识，当前按全局唯一处理 |
| `type` | - | 固定为 `RECEIVER` |
| `gainDbi` | dBi | 增益 |
| `locationOffset` | m | 相对船体中心的三维偏移 |
| `centerFrequencyGHz` | GHz | 中心频率 |
| `bandwidthMHz` | MHz | 带宽 |
| `sensitivityDbm` | dBm | 灵敏度，当前标准固定为负值 |
| `interferenceMarginDb` | dB | 干扰容限或等效带外抑制 |
| `sinrMarginDb` | dB | SINR 裕量 |
| `noiseFigureDb` | dB | 噪声系数 |

收发一体机字段如下：

| 字段 | 单位 | 说明 |
| --- | --- | --- |
| `ID` | - | 设备唯一标识，当前按全局唯一处理 |
| `type` | - | 固定为 `TRANSCEIVER` |
| `gainDbi` | dBi | 共用增益 |
| `locationOffset` | m | 共用相对偏移 |
| `transmitter` | object | - | 发射子对象，字段与发射机参数一致但不再重复 `ID/type/gainDbi/locationOffset` |
| `receiver` | object | - | 接收子对象，字段与接收机参数一致但不再重复 `ID/type/gainDbi/locationOffset` |

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

当前口径：

- DTO 字段名已与新 schema 对齐
- `TRANSCEIVER` 已从“内部私有值”收敛为标准 schema 枚举
- 核心语义校验统一收口到 `DataModel::validateSnapshot()`
- `validateSnapshot()` 统一负责环境范围、至少一艘船、ID 唯一性与船载设备引用一致性

### 4.2 `JsonLoader::LoadFile`

职责：

- 读取 JSONC 文本
- 去除单行 `//` 注释
- 校验顶层结构、环境字段、船只字段、设备字段
- 拒绝未知字段
- 调用 `DataModel::validateSnapshot()` 完成核心语义校验
- 写回 `DataModel::instance()`

边界：

- 只支持新 schema
- 不保留旧格式兼容逻辑
- 是主程序标准 JSON 输入的唯一解析入口

### 4.3 UI 输入页

职责：

- 保持中文界面文案
- 把控件输入整理为统一 DTO
- 只做基础格式校验，例如：
  - 必填项是否为空
  - 数值字段能否成功解析
  - schema 枚举值是否来自受支持选项

约束：

- UI 不应复制 `DataModel` 的核心语义规则
- 设备/船只保存时，应构造候选 `DataSnapshot`
- 跨设备 ID、船只引用一致性、环境范围等核心规则统一交给 `DataModel::validateSnapshot()`

### 4.4 `TransferToEngine::convertDataModelToFleet`

职责：

- 将 DTO 快照转换为 `Fleet`
- 将 `ShipData`、`EquipmentData` 变为领域层对象
- 为后续 `EMC_Engine` 与 `PEModel` 提供可计算输入

### 4.5 `EMC_Engine` 与 `Propagation_Engine`

职责：

- `EMC_Engine::do_PE_computing()`：调度所有发射机仿真并汇总结果
- `Propagation_Engine::PEmodel_computing2D()`：计算单发射机二维传播结果
- `PEModel`：负责传播步进、表面模型和路径损耗求解

当前输出：

- 主程序核心输出仍以二维 `GridMap` 为主
- EMC 指标、报告对象、结构化评估结果尚未成为稳定 API

## 5. 一致性维护要求

后续只要修改输入字段、单位或枚举，必须同步更新以下位置：

1. `Interface/SchemaConstants.h`
2. `Utils/JsonLoader.hpp`
3. `Tests/Test.jsonc`
4. `docs/schema/usv-environment.schema.json`
5. `docs/schema/usv-environment.template.jsonc`
6. 本文档

当前最重要的架构纪律不是继续堆叠 UI，而是保持“schema、运行时代码、验证口径”三者同口径。
