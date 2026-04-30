# 仿真页结果总览与详情展示设计

日期：2026-04-30

## Summary

本设计聚焦仿真页的结果展示层，不改动传播计算和指标计算职责边界。页面统一消费已经计算完成的 `SimulationTaskResult`，以“上方缩略图卡片总览 + 下方大图详情区”的方式展示六类主图：

- 总场分布
- 参考发射机路径损耗
- `SCF`
- `S3I`
- `T_elev`
- `D_desense`

本轮的关键结论如下：

- UI 不直接调用 `EMCMetricsCalculator`
- UI 只消费 `SimulationTaskResult`
- `ScalarField2D` 不是全部图的唯一承载类型
- 详情区按 `ScalarField2D / Series1D / LabeledMatrix2D` 三类结果拆分
- `SCF` 详情图采用“矩阵式表格热图”，不是普通折线图或纯文本表
- 统一使用 `QCustomPlot` 完成详情图和缩略图渲染，尽量复用 Ela 卡片与布局控件

## 1. 数据边界与图表目录

### 1.1 结果来源

仿真页展示层只读取 `SimulationTaskResult`：

- `aggregatedField`
- `emitterResults`
- `derivedMetrics`

`EMCMetricsCalculator` 只属于仿真成功后的结果组装阶段，由 `simSchedulerCtx` 负责调用。展示层不得在点击卡片或切换详情时重新计算指标。

### 1.2 六类主图映射

本轮只展示六类主图，不扩展到诊断切片图、导出图和对比图：

1. 总场分布
   - 来源：`taskResult.aggregatedField`
2. 参考发射机路径损耗
   - 来源：`taskResult.emitterResults`
   - 过滤规则：只保留 `taskResult.inputSnapshot.emcAnalysisConfig.referenceTransmitterId` 对应的成功态 `field2D`
3. `SCF`
   - 来源：`taskResult.derivedMetrics.scf.couplingMatrix`
4. `S3I`
   - 来源：`taskResult.derivedMetrics.s3i.calmCurve` 与 `currentCurve`
5. `T_elev`
   - 来源：`taskResult.derivedMetrics.tElev.field`
6. `D_desense`
   - 来源：`taskResult.derivedMetrics.dDesense.field`

如果 `SimulationTaskResult` 合法且成功，则这六类图应被视为当前版本的完整主图目录。

## 2. 页面结构

### 2.1 总体布局

仿真页保留现有任务控制区，并将结果展示区重构为两层：

- 上层：结果缩略图卡片总览
- 下层：单一常驻大图详情区

推荐结构：

- 顶部：开始仿真、取消仿真、状态提示
- 中部：`ElaScrollPageArea` 包裹卡片总览区
- 底部：单一详情区，使用 `QStackedWidget` 在三类详情视图间切换

成功态首次进入结果页时，默认选中“总场分布”卡片并显示其详情。

### 2.2 卡片总览

卡片总览固定生成六张卡，不按设备数量无限扩展：

- 总场分布
- 参考发射机路径损耗
- `SCF`
- `S3I`
- `T_elev`
- `D_desense`

每张卡片必须具备：

- 中文小标题
- 缩略图预览
- 一行简短副标题
- 点击后切换下方详情区

卡片优先使用 `ElaInteractiveCard`。如果现有控件无法表达“选中态高亮”，允许增加一个极薄的派生封装，例如只补：

- 选中态边框或背景
- 当前卡片 key
- 点击后发出选中信号

不允许在卡片层重复发明新的结果数据结构。

### 2.3 详情区

详情区不是“六张图六套完全独立页面”，而是三类结果视图共用一个外壳：

- `ScalarField2D` 详情视图
- `Series1D` 详情视图
- `LabeledMatrix2D` 详情视图

三类视图由 `QStackedWidget` 管理。切换卡片时：

- 先根据卡片类型切换栈页
- 再向对应视图灌入标题、单位、统计摘要和图表数据

## 3. 绘图方案

### 3.1 总原则

统一使用仓库内的 [qcustomplot.cpp](/d:/code/Electromagnetic_compatibility/Resource/ui/qcustomplot.cpp) / `QCustomPlot` 绘图，不引入 web 图表或额外绘图库。

原因：

- 当前工程已有 `QCustomPlot`
- 现有仿真页已使用这套图表能力
- `QCustomPlot` 已具备本轮所需的全部关键能力：
  - `QCPColorMap`
  - `QCPColorGradient`
  - `QCPColorScale`
  - `QCPAxisTickerText`
  - `QCPItemText`
  - `QCPGraph::setChannelFillGraph`
  - `QCustomPlot::toPixmap`
  - `QCPColorMapData::setAlpha`

### 3.2 缩略图生成

缩略图不在卡片内嵌真实图表控件，而是采用离屏渲染：

1. 用临时 `QCustomPlot` 按缩略图模式绘制
2. 调用 `QCustomPlot::toPixmap(...)`
3. 将结果 `QPixmap` 填入 Ela 卡片的 `CardPixmap`

这样可以保证：

- 卡片展示与详情区使用同一套绘图逻辑
- 不需要在卡片内部再放一个原生 `QWidget` 图表
- 六张卡的渲染成本可控

### 3.3 场图类：总场 / 参考发射机 / `T_elev` / `D_desense`

这四类图统一基于 `QCPColorMap` 绘制，但每类图使用不同的配色和色标规则。

#### 总场分布与参考发射机路径损耗

参考 `experiment3.ipynb` 早期二维场图口径，采用 `jet` 风格。

- `QCPColorGradient::gpJet`
- 缩略图默认隐藏详细轴标签
- 详情图保留轴单位和色条
- 数据范围默认按当前结果自适应

说明：

- `aggregatedField` 与 `emitterResults[].field2D` 的物理量不同，色条标题必须跟随 `displayName + valueUnit`
- 不再沿用现有 `PaintImage.hpp` 中固定 `[-120, 0]` 的硬编码范围去强行绘制全部场图

#### `T_elev`

参考 [experiment3.ipynb](PE_validation/experiment3.ipynb) 中的 `inferno` 风格，表现“热度抬升”。

- 使用自定义 `QCPColorGradient` 近似 `inferno`
- 详情图区保留白色关键等值线
- 缩略图可以省略等值线，只保留热图轮廓

#### `D_desense`

参考 [experiment3.ipynb](PE_validation/experiment3.ipynb) 中的分段风险色带。

- 使用自定义分段梯度，颜色顺序固定为：
  - `#e9f5e9`
  - `#fff3e0`
  - `#ffcc80`
  - `#ffab91`
  - `#f44336`
  - `#b71c1c`
  - `#4a148c`
- 详情图区需显式表达风险阈值语义
- 缩略图允许省略阈值说明文字，但不能改色带语义

### 3.4 曲线类：`S3I`

`S3I` 详情图使用 `QCPGraph` 双曲线方案：

- 平静海况：蓝色实线 `#1f77b4`
- 当前海况：红色虚线 `#d62728`
- 两条曲线之间用灰色半透明填充
- 使用 `QCPGraph::setChannelFillGraph(...)` 构造差值带

细节口径：

- 横轴：距离
- 纵轴：归一化场强或指标单位
- 图例固定在左下
- 详情区显示参考链路与基准/当前海况摘要

缩略图策略：

- 仅保留双曲线形状和差值带
- 隐藏图例、坐标轴说明和长标题

### 3.5 矩阵类：`SCF`

`SCF` 详情图采用“矩阵式表格热图”，而不是普通折线图或 Qt 表格控件。

实现口径：

- 底图：`QCPColorMap`
- 配色：参考 [experiment3.ipynb](PE_validation/experiment3.ipynb) 的 `magma`
- 横纵轴标签：`QCPAxisTickerText`
- 单元格数值：`QCPItemText`
- 表格边界：用轴刻度和网格线表达，必要时增加单元格边界线

布局要求：

- 横轴是发射机标签
- 纵轴是接收机标签
- 单元格内叠加数值文本
- 文本颜色根据背景亮度自适应切换深浅色
- 详情区显示 `SCF` 标量、热噪声底和链路数摘要

缩略图策略：

- 仍然显示热图矩阵形状
- 可以隐藏大部分文本标签，只保留高层次形状识别

### 3.6 `noDataValue` 与透明单元格

若 `ScalarField2D` 存在 `noDataValue` 或 `NaN`：

- 渲染层不得把它直接压成最低色
- 应通过 `QCPColorMapData::setAlpha(...)` 将对应单元格设为透明

这样可以利用 `QCustomPlot` 的 alpha map 机制，避免无效值污染热图区视觉判断。

## 4. 交互与状态

### 4.1 结果切换

用户点击任一卡片后：

1. 更新当前选中卡片状态
2. 切换详情区到对应视图类型
3. 重新绑定标题、副标题、摘要和大图数据

不在卡片点击时触发任何重新计算。

### 4.2 成功态、失败态与取消态

- 成功态：展示六类结果卡和详情区
- 失败态：保留状态信息，不展示伪结果卡
- 取消态：保留上一份成功结果，不用本次半成品覆盖结果区

### 4.3 脏数据提示

继续沿用当前 `SimulationTaskResult.inputSnapshot` 与 `DataModel::createSnapshot()` 的对比逻辑。结果展示重构后，仍需保留“当前结果对应旧输入”的提示能力。

## 5. 文件边界建议

为避免把全部展示逻辑重新堆进 `Simulation.cpp`，建议分成三层：

1. 结果目录抽取层
   - 从 `SimulationTaskResult` 生成六张卡的展示描述
2. 图表渲染层
   - 将 `ScalarField2D / Series1D / LabeledMatrix2D` 渲染到 `QCustomPlot`
   - 负责离屏缩略图生成
3. 页面接线层
   - 布局、卡片点击、详情切换、状态刷新

## 6. Test Cases

- 成功态结果可稳定抽取六张固定卡片
- `referenceTransmitterId` 缺失匹配时，参考发射机卡不应错误回退到其他发射机
- 点击不同卡片时，详情区能正确切换到三类视图之一
- `SCF` 详情区能显示矩阵标签和单元格文本
- `S3I` 详情区能同时显示两条曲线和灰色差值带
- `T_elev` 与 `D_desense` 使用各自独立配色，不回退到统一 `jet`
- `noDataValue` 或 `NaN` 单元格能按透明处理
- 缩略图与详情图消费同一份结果对象，不触发二次指标计算

## Assumptions

- 本轮不处理任务历史、结果持久化和报告导出
- 本轮不处理验证侧代码，不做验证侧兼容
- 本轮只覆盖六类主图，不扩展多受害机或多参考链路浏览器
- `SCF` 详情图使用矩阵式表格热图，这是已锁定方案
- Ela 控件优先；仅在 Ela 无现成图表能力时，允许增加极薄的本地封装
