# 项目架构与 API 规范

## 1. 架构总览

当前项目可以抽象为五层：

```text
输入配置层
  ├─ UI 页面输入
  └─ JSONC 场景文件

数据建模层
  ├─ DataModel
  ├─ EquipmentData / ShipData / EnvironmentData
  └─ DataSnapshot

领域对象层
  ├─ Fleet
  ├─ ship
  ├─ Equipment / Transmitter / Receiver / Transceiver
  └─ Antenna

传播与仿真层
  ├─ TransferToEngine / TransferToPEdata
  ├─ EMC_Engine
  ├─ Propagation_Engine
  └─ PEModel / AtmosphereModel / JONSWAPSurfaceGenerator

展示与验证层
  ├─ QCustomPlot / UI 页面
  ├─ CSV 输出
  └─ PE_validation notebook / 图表 / 报告
```

这五层中，主项目真正的主链路是“输入配置层 -> 数据建模层 -> 领域对象层 -> 传播与仿真层 -> 展示层”，而 `PE_validation/` 更多承担验证和论文资产角色。

## 2. 模块分层说明

### 2.1 UI 与展示层

主要目录：`Resource/ui/`、`ModelView/`、`ExamplePage/`

职责：

- 提供主窗口和页面容器。
- 录入船只、设备和仿真参数。
- 启动仿真线程。
- 将二维结果绘制为图表。
- 输出日志和用户提示。

当前核心页面：

- `MainWindow`：应用主窗口与页面容器。
- `ShipWidget`：船只信息录入与船载设备绑定。
- `DeviceWidget`：设备参数录入。
- `Simulation`：启动仿真并展示二维场分布。

### 2.2 数据建模层

主要目录：`Interface/`

职责：

- 承接 UI 与 JSON 解析后的原始业务数据。
- 提供统一的数据快照。
- 将界面层数据转换为算法层对象。

当前核心类型：

- `EquipmentData`
- `EquipmentOnShip`
- `ShipData`
- `EnvironmentData`
- `DataModel`

### 2.3 领域对象层

主要目录：`Models/`

职责：

- 表达“船只、设备、天线、编队”这些领域概念。
- 将参数从 DTO 形式转为可计算对象。

当前核心类型：

- `Fleet`
- `ship`
- `Equipment`
- `Transmitter`
- `Receiver`
- `Transceiver`
- `Antenna` 及其派生类

### 2.4 传播与仿真层

主要目录：`Simulation/`

职责：

- 建立大气、海面与传播环境模型。
- 组织一维/二维传播计算。
- 聚合多发射源结果。
- 输出可供展示的网格结果。

当前核心类型：

- `AtmosphereModel`
- `MillerBrownModel`
- `JONSWAPSurfaceGenerator`
- `PEModel`
- `Propagation_Engine`
- `EMC_Engine`

### 2.5 验证与论文层

主要目录：`PE_validation/`

职责：

- 实验设计与研究叙事。
- `Two-Ray` / `FDTD` / `PE-PLST` 对比验证。
- A/B 编队设计与四指标结果展示。

这一层当前不是主程序运行时依赖，但它定义了项目的研究口径和后续产品化方向。

## 3. 运行时主流程

当前主程序的一次典型运行链路如下：

1. 用户在 `ShipWidget`、`DeviceWidget` 中录入数据，或通过 `JsonLoader` 读取 JSONC 文件。
2. 数据进入 `DataModel` 单例。
3. `Simulation` 页面创建 `DataSnapshot`。
4. `TransferToEngine::convertDataModelToFleet()` 将快照转为 `Fleet`。
5. `EMC_Engine` 调用 `EquipmentConvertToMatrix()` 提取发射机传播输入。
6. `Propagation_Engine::PEmodel_computing2D()` 对每个发射机计算二维传播损耗。
7. `EMC_Engine::do_PE_computing()` 将多个发射源在线性功率域聚合后再转回 dBm 网格。
8. 结果通过 `peComputationFinished` 回传 UI，并由 `QCustomPlot` 绘图。

## 4. 核心数据结构规范

### 4.1 EquipmentData

`EquipmentData` 是界面层和配置层使用的设备 DTO，包含三类字段：

#### 基础字段

- `equipmentID`
- `equipmentType`
- `Gain`
- `X_offset`
- `Y_offset`
- `Z_offset`

#### 接收机字段

- `CentralF_Reciever`
- `Bandwidth_Reciever`
- `Sensitive_reciever`
- `interferenceMargin`
- `SINRMargin`
- `noiseFigure`

#### 发射机字段

- `CentralF_Transmitter`
- `Bandwidth_Transmitter`
- `Power_Transmitter`
- `antennaPhi_Transmitter`
- `Beamwidth_Transmitter`
- `PolarizationMethod_Transmitter`
- `antennaType_Transmitter`

#### 规范建议

- `equipmentID` 应在全局唯一，至少应在“船只 ID + 设备 ID”组合下唯一。
- 坐标单位统一为米。
- 角度统一为度。
- 功率统一为 dBm。
- 灵敏度建议统一使用负 dBm。

### 4.2 ShipData

`ShipData` 表示单艘船只的基础信息：

- `shipID`
- `shipType`
- `X_offset`
- `Y_offset`
- `Z_offset`
- `ship_Orienteation`
- `ship_Speed`
- `Equipments`

规范建议：

- 船只坐标统一为世界坐标，单位米。
- 航向角统一使用 `[0, 360]` 度。
- `Equipments` 表示该船挂载的设备引用关系。

### 4.3 EnvironmentData

`EnvironmentData` 描述传播环境：

- `max_range`
- `duct_height`
- `wind_speed`
- `dx`
- `dz`
- `nz`
- `angle_step_deg`

规范建议：

- `max_range`、`duct_height`、`dx`、`dz` 单位为米。
- `wind_speed` 单位为 m/s。
- `angle_step_deg` 单位为度。
- 该结构应允许从配置文件显式加载，而不仅依赖默认值。

### 4.4 算法层对象

#### Fleet

- 表示一个编队。
- 负责维护 `ship` 集合。

#### ship

- 表示单艘船只。
- 持有绝对位置、航向、航速和设备列表。

#### Equipment 及派生类

- `Transmitter`：提供发射功率、频率、波束等参数。
- `Receiver`：提供灵敏度、噪声系数、干扰阈值等参数。
- `Transceiver`：收发一体设备。

#### Antenna

- `OmniAntenna`
- `DirectionalAntenna`
- `HornAntenna`
- `ShapedBeamAntenna`
- `ReflectorAntenna`

不同天线类型决定垂直场初始化分布和传播初始条件。

### 4.5 Transmitter_PE_data

这是传播计算直接使用的扁平化输入结构，包含：

- 发射机所属船只与设备名称
- 发射机绝对位置
- 天线类型
- 发射功率
- 天线高度
- 波束宽度
- 天线角度
- 中心频率
- 结果功率网格

它的作用是隔离 UI/领域对象结构与数值计算结构，减少算法层对上层对象的耦合。

## 5. 输入配置规范

### 5.1 当前有效输入形式

当前主项目实际支持的输入形式是 JSONC，对应 `Tests/` 与 `PE_validation/` 下的 `Test*.jsonc` 文件。

根节点结构建议如下：

```json
{
  "USV1": {
    "ID": "USV1",
    "Location": {
      "type": "Point_3D",
      "coordinates": [720, 0, 0]
    },
    "Speed": 6,
    "Orientation": 0,
    "Transmitter1": {
      "ID": "USV1_TX1",
      "type": "TRANSMITTER",
      "Gain": "20",
      "Location_Offset": [1, 0, 2],
      "Central_F": "1",
      "Bandwith": "100",
      "Power": "43",
      "angle": "30",
      "BeamWidth": "20",
      "PolarizationMethod": "垂直极化",
      "AntennaType": "喇叭天线"
    },
    "Receiver1": {
      "ID": "USV1_RX1",
      "type": "RECEIVER",
      "Gain": "20",
      "Location_Offset": [-1, 0, 2],
      "Central_F": "1",
      "Bandwith": "100",
      "Sensitivity": "-100.0",
      "interferenceMargin": "-20",
      "SINRMargin": "0",
      "noiseFigure": "3"
    }
  }
}
```

### 5.2 建议统一的字段口径

为避免主程序、UI 和 notebook 口径冲突，建议后续统一如下：

| 字段 | 建议单位 | 建议语义 |
| --- | --- | --- |
| `Central_F` | GHz | 发射机/接收机中心频率 |
| `Bandwith` | MHz | 信号带宽 |
| `Power` | dBm | 发射功率 |
| `Gain` | dBi 或 dB | 天线/设备增益，需在代码中明确口径 |
| `Sensitivity` | dBm | 接收机灵敏度，建议为负值 |
| `interferenceMargin` | dB | 干扰容限或等效带外抑制，需明确物理定义 |
| `Location_Offset` | m | 相对船体坐标 |
| `coordinates` | m | 船只绝对坐标 |
| `angle` / `BeamWidth` | degree | 方位角 / 波束宽度 |

## 6. 内部 API 规范

### 6.1 配置解析接口

#### `JsonLoader::LoadFile(const QString& filePath)`

职责：

- 读取 JSONC 文件。
- 清除单行注释。
- 解析船只与设备对象。
- 写入 `DataModel::instance()`。

约束：

- 输入路径应为仓库相对路径或用户选择路径，不应依赖固定绝对路径。
- 解析器需要与 `EnvironmentData`、阵型模板等后续配置扩展兼容。

### 6.2 快照接口

#### `DataModel::createSnapshot()`

职责：

- 从全局数据模型创建一次可拷贝快照。
- 避免仿真线程直接读写 UI 层状态。

约束：

- 快照应被视为仿真任务的输入边界。
- 仿真过程中不应反向修改 `DataModel`。

### 6.3 领域对象转换接口

#### `TransferToEngine::convertDataModelToFleet(const DataSnapshot&)`

职责：

- 将 DTO 快照转换为 `Fleet`。

约束：

- 设备绑定关系必须准确，不能因为 ID 冲突把不同船上的设备错误映射到同一个对象。

#### `EquipmentConvertToMatrix(Fleet*)`

职责：

- 从 `Fleet` 中提取所有发射机并展开为 `Transmitter_PE_data`。

约束：

- 输入对象中的设备位置必须已经是可解释的物理坐标。
- 若未来接收机也参与链路级评估，应扩展专门的接收端输入结构。

### 6.4 传播计算接口

#### `Propagation_Engine::PEmodel_computing1D(...)`

职责：

- 计算单发射机在一维距离方向上的传播损耗曲线。

#### `Propagation_Engine::PEmodel_computing2D(...)`

职责：

- 计算单发射机在二维平面上的传播损耗分布。

实现要点：

- 先在极坐标下按角度并行求解。
- 再把极坐标矩阵重采样为笛卡尔网格。
- 接收点高度会按海面高度动态修正。

#### `EMC_Engine::do_PE_computing()`

职责：

- 调度所有发射机的二维传播计算。
- 在 mW 线性域完成多发射源功率叠加。
- 将结果转换为 dBm 网格后交给 UI 展示。

### 6.5 核心数值接口

#### `AtmosphereModel`

- 根据蒸发波导高度生成修正折射率和折射率剖面。

#### `JONSWAPSurfaceGenerator`

- 根据风速生成粗糙海面谱分量。
- 提供任意 `(x, y, t)` 处的海面高度。

#### `PEModel`

职责：

- 管理 FFTW 计划与传播状态。
- 预计算衍射项和顶部吸收层。
- 使用 `step_Miller_Brown` 或 `step_PLST` 进行步进。
- 使用 `initializeGaussian` 初始化天线垂直场。
- 使用 `getPathLoss` 提取指定高度与距离处的路径损耗。

## 7. 当前对外接口

从“工程使用者”角度看，当前项目实际存在四类对外接口：

### 7.1 UI 录入接口

- 船只页面
- 设备页面
- 仿真页面

### 7.2 JSONC 配置接口

- `Tests/Test.jsonc`
- `Tests/Test_A.jsonc`
- `Tests/Test_B.jsonc`
- `PE_validation/Test*.jsonc`

### 7.3 结果输出接口

- `GridMap` 二维场强结果
- `CSV` 导出文件
- 图表展示页面

### 7.4 验证与报告接口

- `PE_validation/*.ipynb`
- `PE_validation/*.md`
- `PE_validation/*.html`

这说明项目当前还没有形成“稳定统一的业务 API”，更多是“GUI + 文件配置 + notebook 验证”的组合接口形态。

## 8. 后续 API 规范建议

为支撑 TODO 中的阵型接口、四指标计算和报告生成功能，建议后续逐步补齐以下接口边界：

1. **统一配置接口**
   - 把舰船、设备、环境、阵型模板整合为统一配置 schema。
2. **统一仿真任务接口**
   - 明确“输入配置 -> 仿真任务 -> 指标结果 -> 报告结果”的主流程对象。
3. **统一指标接口**
   - 在主项目中正式定义 `SCF`、`S3I`、`T_elev`、`D_desense` 的计算 API。
4. **统一报告接口**
   - 输出结构化评价结果，而不只是图和 CSV。
5. **统一语义与单位接口**
   - 消除当前频率、带宽、灵敏度、干扰阈值的命名和单位混乱。

## 9. 架构结论

当前项目的架构本质上是：

- 上层：Qt 输入与展示
- 中层：`DataModel` 与对象转换
- 下层：`PE-PLST` 传播求解
- 旁路：Python notebook 验证与论文资产

下一阶段的关键不是继续堆 UI，而是把“验证侧已有的 EMC 指标体系”转化为“主项目中可复用、可调用、可报告的统一 API”。
