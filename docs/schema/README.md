# USV 输入 Schema 说明

## 1. 目的

本目录用于定义项目后续统一采用的标准输入格式，覆盖：

- `USV` 编队与设备输入
- `EnvironmentData` 环境输入

本标准的目标是为后续的配置校验、解析器改造、阵型模板扩展、EMC 指标计算和报告生成提供稳定输入边界。

## 2. 文件说明

- `usv-environment.schema.json`
  - 正式的 JSON Schema 文件
  - 用于描述字段结构、类型、必填项、范围和枚举
- `usv-environment.template.jsonc`
  - 面向人类阅读和填写的模板文件
  - 保留中文注释、单位说明和初始化示例

## 3. 标准格式原则

本标准采用以下约定：

1. 所有物理量统一使用 JSON number，不再使用字符串数字。
2. 顶层结构统一为：

```json
{
  "schemaVersion": "1.0.0",
  "environment": {},
  "usvs": []
}
```

3. `usvs` 使用数组，而不是旧版 `USV1` / `USV2` 作为根节点对象映射。
4. `location.type` 固定为 `Point3D`。
5. 正式字段名统一采用 camelCase。
6. `ID` 字段保留。

## 4. 与旧格式的关系

当前仓库中的 `Tests/Test.jsonc`、`Tests/Test_A.jsonc`、`Tests/Test_B.jsonc` 仍属于旧输入格式样例，它们可以继续作为：

- 旧解析器的兼容测试样例
- 业务场景样例
- A/B 编队设计样例

但它们不再作为标准输入格式原型。

## 5. 旧字段到新字段映射

| 旧字段 | 新字段 |
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

## 6. 坐标与角度约定

- 船只 `location.coordinates`
  - 表示世界坐标
  - 原点位于左下角
  - `X` 轴向右
  - `Y` 轴向上
  - `Z` 轴向上
- 设备 `locationOffset`
  - 表示相对船体中心的偏移
- `shipOrientationDeg`
  - 表示船只在二维平面的朝向角
  - 取值范围 `0~360`
- `antennaPhiDeg`
  - 表示相对正 `z` 轴向下倾斜的角度
  - 取值范围 `0~180`

## 7. 后续建议

本标准落地后，建议按如下顺序推进：

1. 修改 `JsonLoader`，兼容读取新格式
2. 增加标准输入校验逻辑
3. 逐步把旧格式样例迁移为新格式样例
4. 将主程序仿真链路与新 schema 对齐
