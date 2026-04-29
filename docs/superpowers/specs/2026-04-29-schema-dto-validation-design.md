# Schema DTO 命名与校验收敛设计

## 1. 目标

本设计用于一次性解决以下三个已登记风险：

- `3.2 新 schema 与旧 DTO 命名仍并存`
- `3.3 UI 校验规则与新 schema 语义可能冲突`
- `4.1 EnvironmentData 的代码实现与文档仍有细节不一致`

目标不是修改外部 JSON schema，而是让主程序内部 DTO、UI、解析器和文档对同一套 schema 语义使用一致口径。

## 2. 范围

本次变更只覆盖主程序链路，不涉及 `PE_validation` 子模块：

- `Interface/DataModel.h`
- `Utils/JsonLoader.hpp`
- `Interface/TransferToEngin.cpp`
- `Resource/ui/DeviceWidget.cpp`
- `Resource/ui/shipwidget.cpp`
- `docs/项目架构与API规范.md`
- `docs/风险与改进建议.md`

如需新增最小测试入口，还会涉及：

- `Tests/` 下新增一个轻量测试源文件
- `CMakeLists.txt` 中新增可选测试目标

## 3. 不做的事

本次不处理以下内容：

- `Simulation.cpp` 的绝对路径问题
- `PE_validation` 中 notebook 与主程序结果链路打通
- 阵型模板、报告生成、结构化 EMC 指标迁移
- 大规模拆分 `EquipmentData` 为独立的发射机 / 接收机 DTO

## 4. 现状问题

### 4.1 DTO 命名与 schema 口径脱节

当前 `DataModel.h` 内部仍存在大量历史命名，例如：

- `CentralF_Reciever`
- `Bandwidth_Reciever`
- `Sensitive_reciever`
- `ship_Orienteation`
- `max_range`

而对外 schema 已统一为：

- `centerFrequencyGHz`
- `bandwidthMHz`
- `sensitivityDbm`
- `shipOrientationDeg`
- `maxRange`

这会导致阅读 `JsonLoader`、UI 代码和文档时不断来回做语义映射。

### 4.2 UI 校验仍带旧业务假设

当前校验逻辑与 schema 已经不一致：

- `gainDbi` 被要求不能为负，但物理上允许负增益
- `sensitivityDbm` 被要求不高于 `-90 dBm`，但 schema 只要求负值
- `interferenceMarginDb` 被要求小于 `0`，但 schema 没有限制符号
- `powerDbm` 被要求不能小于 `0`，但 schema 允许一般数值

### 4.3 `EnvironmentData` 注释和校验不完整

当前 `EnvironmentData` 存在三类问题：

- 注释仍残留旧步进说明
- 字段命名与 schema 不一致
- `validate_EnvironmentConfig()` 在非法以外分支没有成功返回值，语义不完整

## 5. 选型结论

采用方案一：内部 DTO 主动向新 schema 命名收敛，同时同步重写验证语义。

不采用“只修注释、不改成员名”的保守方案，原因如下：

- 它不能真正解决 `3.2`
- 后续 `JsonLoader`、UI 和文档仍需长期维护两套口径
- 将来迁移 EMC 指标时还会继续放大理解成本

## 6. 命名收敛方案

### 6.1 EquipmentData

以下成员统一改为新口径的 C++ lowerCamel 命名：

| 旧成员 | 新成员 |
| --- | --- |
| `equipmentID` | `equipmentId` |
| `Gain` | `gainDbi` |
| `X_offset` | `offsetX` |
| `Y_offset` | `offsetY` |
| `Z_offset` | `offsetZ` |
| `CentralF_Reciever` | `receiverCenterFrequencyGHz` |
| `Bandwidth_Reciever` | `receiverBandwidthMHz` |
| `Sensitive_reciever` | `receiverSensitivityDbm` |
| `interferenceMargin` | `receiverInterferenceMarginDb` |
| `SINRMargin` | `receiverSinrMarginDb` |
| `noiseFigure` | `receiverNoiseFigureDb` |
| `CentralF_Transmitter` | `transmitterCenterFrequencyGHz` |
| `Bandwidth_Transmitter` | `transmitterBandwidthMHz` |
| `Power_Transmitter` | `transmitterPowerDbm` |
| `antennaPhi_Transmitter` | `transmitterAntennaPhiDeg` |
| `Beamwidth_Transmitter` | `transmitterBeamWidthDeg` |
| `PolarizationMethod_Transmitter` | `transmitterPolarization` |
| `antennaType_Transmitter` | `transmitterAntennaType` |

`equipmentType` 暂时保留，因为它当前承载 UI 内部分类值 `"发射机" / "接收机" / "收发一体机"`，并不直接等价于 schema 中的 `type`。

### 6.2 ShipData

| 旧成员 | 新成员 |
| --- | --- |
| `shipID` | `shipId` |
| `X_offset` | `worldX` |
| `Y_offset` | `worldY` |
| `Z_offset` | `worldZ` |
| `ship_Orienteation` | `shipOrientationDeg` |
| `ship_Speed` | `shipSpeedMps` |
| `Equipments` | `equipmentRefs` |

`shipType` 保留，原因是当前主程序内部仍用它表示 UI 侧舰船类型标签，不对应 schema 字段。

### 6.3 EnvironmentData

| 旧成员 | 新成员 |
| --- | --- |
| `max_range` | `maxRange` |
| `duct_height` | `ductHeight` |
| `wind_speed` | `windSpeed` |
| `angle_step_deg` | `angleStepDeg` |

`dx`、`dz`、`nz` 已与 schema 足够接近，仅统一注释。

## 7. 校验语义收敛方案

### 7.1 EquipmentData 基础校验

基础校验保留以下约束：

- `equipmentId` 不能为空
- 偏移坐标仍受当前 UI 地图边界约束

移除以下旧约束：

- `gainDbi < 0` 直接报错

原因：负增益本身是合法物理输入，schema 也未限制其符号。

### 7.2 接收机校验

保留：

- `receiverCenterFrequencyGHz > 0`
- `receiverBandwidthMHz > 0`
- `receiverSensitivityDbm < 0`
- `receiverNoiseFigureDb >= 0`

移除：

- `receiverSensitivityDbm <= -90`
- `receiverInterferenceMarginDb < 0`

原因：这两项属于特定业务建议值，不应作为统一硬约束。

### 7.3 发射机校验

保留：

- `transmitterCenterFrequencyGHz > 0`
- `transmitterBandwidthMHz > 0`
- `transmitterAntennaPhiDeg` 位于 `[0, 180]`
- `transmitterBeamWidthDeg` 位于 `[0, 360]`

移除：

- `transmitterPowerDbm < 0` 直接报错

原因：负 `dBm` 发射功率在物理上合法，schema 也允许一般数值。

### 7.4 船只校验

保留：

- `shipSpeedMps >= 0`
- `shipOrientationDeg` 位于 `[0, 360]`
- 世界坐标仍受当前 UI 地图边界约束

同步修正文档注释，明确速度单位为 `m/s`。

### 7.5 EnvironmentData 校验

统一为与 schema 一致：

- `maxRange > 0`
- `ductHeight >= 0`
- `windSpeed >= 0`
- `dx > 0`
- `dz > 0`
- `nz > 0`
- `angleStepDeg` 为正整数且位于 `[1, 360]`

`validate_EnvironmentConfig()` 必须在成功分支显式返回 `{true, ""}`。

## 8. 代码修改点

### 8.1 `Interface/DataModel.h`

职责：

- 完成 DTO 成员重命名
- 修正所有注释到新 schema 口径
- 重写 `validate_EquipmentBaseInfo()`
- 重写 `validate_reciever()`
- 重写 `valiate_Transmitter()`
- 修复 `validate_EnvironmentConfig()`

注意：

- 本次只做字段收敛，不引入新的 DTO 层级拆分
- 仍保留当前结构，避免无关重构

### 8.2 `Utils/JsonLoader.hpp`

职责：

- 将解析结果写入新的 DTO 成员名
- 保持当前 schema 校验逻辑不变
- 保持 `SchemaConstants.h` 为字段与枚举来源

本次不改变：

- 顶层 schema 结构
- 旧格式不兼容的现状

### 8.3 `Interface/TransferToEngin.cpp`

职责：

- 按新 DTO 成员名读取发射机、接收机、船只和环境参数
- 保持对算法层输出对象的语义不变

### 8.4 `Resource/ui/DeviceWidget.cpp`

职责：

- 改为读写新的 `EquipmentData` 成员
- 根据新语义保留 UI 标签和 placeholder
- 不在 UI 层重新引入与 schema 冲突的硬校验

### 8.5 `Resource/ui/shipwidget.cpp`

职责：

- 改为读写新的 `ShipData` 成员
- 使用新的 `equipmentRefs` 集合名
- 注释与日志同步改口径

## 9. 测试策略

仓库当前没有专门测试模块，因此本次采用最小可行测试策略：

### 9.1 新增轻量测试目标

新增一个独立的小型 C++ 测试源文件，放在 `Tests/` 下，例如：

- `Tests/SchemaDtoValidationTests.cpp`

该文件使用标准 C++ 断言或显式返回码，不额外引入测试框架。

### 9.2 测试覆盖点

至少覆盖以下行为：

1. `EquipmentData` 允许负 `gainDbi`
2. 接收机允许 `receiverInterferenceMarginDb == 0`
3. 接收机要求 `receiverSensitivityDbm < 0`
4. 发射机允许负 `transmitterPowerDbm`
5. `EnvironmentData` 正常参数通过校验
6. `EnvironmentData` 在 `maxRange <= 0`、`angleStepDeg > 360` 时失败
7. `JsonLoader` 读取 `Tests/Test.jsonc` 后，关键字段被写入新 DTO 成员

### 9.3 CMake 接入

为避免干扰主程序构建：

- 仅在 `BUILD_TESTING` 打开时构建该测试目标
- 不强制引入 `QtTest`、`gtest` 或其他第三方框架

## 10. 文档同步要求

实现完成后，同步更新以下文档：

- `docs/项目架构与API规范.md`
- `docs/风险与改进建议.md`

更新重点：

- 删除“新 schema 与旧 DTO 命名仍并存”的现状表述
- 删除“UI 校验与 schema 冲突”的现状表述，改为已完成收敛
- 删除 `EnvironmentData` 注释和校验不一致的现状表述
- 保留尚未处理的问题，例如 `Simulation.cpp` 绝对路径

## 11. 风险与回归点

本次改动的主要风险在于“字段更名带来的全链路引用遗漏”，尤其集中在：

- `TransferToEngin.cpp`
- `DeviceWidget.cpp`
- `shipwidget.cpp`
- 可能的日志与调试输出

因此实现顺序必须是：

1. 先有测试
2. 再改 DTO
3. 再改解析器和 UI
4. 最后改文档

## 12. 成功标准

本次任务完成的判定标准如下：

1. `DataModel.h` 中与 schema 对应的关键成员已统一到新命名口径。
2. UI 和 `JsonLoader` 不再依赖旧字段名。
3. `validate_*` 规则与当前 schema 保持一致。
4. `EnvironmentData` 注释、命名和校验逻辑一致。
5. 文档中的三项风险被对应消解，不再作为当前问题保留。
