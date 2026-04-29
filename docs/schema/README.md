# USV 输入 schema 说明

## 1. 目的

本目录用于定义主程序当前采用的标准输入格式，覆盖两类数据：

- 无人艇与设备配置
- `EnvironmentData` 环境参数

当前主程序 `JsonLoader` 只支持这一套新 schema，不再保留旧格式兼容。

## 2. 标准来源

这套输入标准由以下三处共同维护：

| 文件 | 作用 |
| --- | --- |
| `Interface/SchemaConstants.h` | 字段名、固定字符串、枚举值来源 |
| `Utils/JsonLoader.hpp` | 当前运行时真实生效的解析与基础校验规则 |
| `docs/schema/usv-environment.schema.json` | 供工具校验与文档引用的显式 schema |

其中：

- 字段名和枚举值以 `SchemaConstants.h` 为准
- 运行时行为以 `JsonLoader.hpp` 为准
- `schema.json` 与模板文件必须保持同步

## 3. 目录内文件说明

| 文件 | 说明 |
| --- | --- |
| `usv-environment.schema.json` | 正式 JSON Schema 文档 |
| `usv-environment.template.jsonc` | 带中文注释的模板样例 |
| `README.md` | 本说明文件 |

主程序当前活动样例文件为：

- `Tests/Test.jsonc`

以下文件仅作历史或业务参考，不再视为主程序标准输入：

- `Tests/Test_A.jsonc`
- `Tests/Test_B.jsonc`
- `PE_validation/` 子模块内的相关实验资产

## 4. 顶层结构

顶层结构固定为：

```json
{
  "schemaVersion": "1.0.0",
  "environment": {},
  "usvs": []
}
```

基本原则如下：

1. 所有物理量统一使用 JSON `number`
2. 船只集合使用 `usvs` 数组
3. 设备集合拆分为 `transmitters`、`receivers`、`transceivers` 三个数组
4. `location.type` 固定为 `Point3D`
5. 顶层、环境、船只、设备中的未知字段会被主程序拒绝

## 5. 关键语义约定

### 5.1 坐标与角度

- 世界坐标原点位于场景左下角
- `x` 轴向右，`y` 轴向上，`z` 轴向上
- 船只 `location.coordinates` 为世界坐标
- 设备 `locationOffset` 为相对船体中心的偏移
- `shipOrientationDeg` 为船在二维平面的朝向角，范围 `0~360`
- `antennaPhiDeg` 为相对正 `z` 轴向下倾斜的角度，范围 `0~180`

### 5.2 单位与类型

- `gainDbi` 使用 `dBi`
- `centerFrequencyGHz` 使用 `GHz`
- `bandwidthMHz` 使用 `MHz`
- `powerDbm`、`sensitivityDbm` 使用 `dBm`
- `interferenceMarginDb`、`sinrMarginDb`、`noiseFigureDb` 使用 `dB`
- `sensitivityDbm` 当前标准固定为负值

### 5.3 收发一体机

- `transceivers` 是新增的标准数组
- `type` 固定为 `TRANSCEIVER`
- 为避免收发字段冲突，收发一体机使用嵌套子对象：
  - `transmitter`
  - `receiver`
- UI 可以继续使用中文文案，但写入 DTO 的值必须使用 schema 英文枚举

### 5.4 ID 规则

- 船只 `ID` 当前要求唯一
- 设备 `ID` 当前按全局唯一处理
- 推荐使用 `USV1_TX1`、`USV3_RX1`、`USV2_TRX1` 这类可追踪命名

## 6. 解析器当前行为

`JsonLoader` 当前行为需要特别注意：

- 只支持新 schema
- 允许 JSONC 单行 `//` 注释
- 不支持旧格式字段自动兼容
- 对未知字段直接报错
- 对 `schemaVersion`、`type`、枚举值做严格检查

这意味着文档、模板和真实输入文件必须与 `SchemaConstants.h` 同步，否则主程序不会自动兜底。

## 7. 历史字段映射参考

以下映射仅用于理解历史样例，不表示主程序仍兼容旧字段：

| 历史字段 | 当前字段 |
| --- | --- |
| `Location` | `location` |
| `Location_Offset` | `locationOffset` |
| `Central_F` | `centerFrequencyGHz` |
| `Bandwith` / `Bandwidth` | `bandwidthMHz` |
| `Power` | `powerDbm` |
| `Gain` | `gainDbi` |
| `Sensitivity` | `sensitivityDbm` |
| `interferenceMargin` | `interferenceMarginDb` |
| `SINRMargin` | `sinrMarginDb` |
| `BeamWidth` | `beamWidthDeg` |
| `Orientation` | `shipOrientationDeg` |
| `angle` | `antennaPhiDeg` |

## 8. 维护要求

只要修改这套 schema，必须同步检查以下位置：

1. `Interface/SchemaConstants.h`
2. `Utils/JsonLoader.hpp`
3. `Tests/Test.jsonc`
4. `docs/schema/usv-environment.schema.json`
5. `docs/schema/usv-environment.template.jsonc`

如果未来需要恢复旧格式兼容，应作为单独需求处理，而不是继续把历史字段混入当前标准文档。
