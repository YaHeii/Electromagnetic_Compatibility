# Simulation Page Result Gallery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将仿真页从单图展示升级为“六类主图卡片总览 + 三类详情视图”的结果浏览页面，并统一基于 `SimulationTaskResult` 与 `QCustomPlot` 绘图。

**Architecture:** 先把 `SimulationTaskResult -> 六张卡片` 的目录抽取逻辑独立出来，再新增统一绘图器负责详情图和缩略图渲染，最后重构 `Simulation` 页面接线。`EMCMetricsCalculator` 不进入 UI 链路，UI 只消费结果对象。

**Tech Stack:** Qt Widgets, ElaWidgetTools, QCustomPlot, Catch2

---

### Task 1: 结果目录抽取层

**Files:**
- Create: `Resource/ui/SimulationResultCatalog.h`
- Create: `Resource/ui/SimulationResultCatalog.cpp`
- Modify: `Resource/ui/Simulation.h`
- Test: `Tests/SimulationResultGalleryTests.cpp`

- [ ] 定义结果卡片枚举与目录结构，固定六类卡片 key，不允许 UI 直接遍历 `emitterResults` 自由长出卡片。
- [ ] 在目录抽取层实现 `SimulationTaskResult -> QList/Vector<ChartCardDescriptor>` 的映射。
- [ ] 明确参考发射机过滤规则：只选 `inputSnapshot.emcAnalysisConfig.referenceTransmitterId` 对应的成功态 `EmitterResult`。
- [ ] 为每张卡片补齐中文标题、副标题、结果类型和原始数据引用信息。
- [ ] 写 Catch2 测试覆盖：
  - 成功态能产出六张卡片
  - 参考发射机卡不会错误回退到其他发射机
  - 失败态/取消态不会产出伪造主图目录

### Task 2: 统一绘图器与缩略图生成

**Files:**
- Create: `Utils/SimulationChartRenderer.h`
- Create: `Utils/SimulationChartRenderer.cpp`
- Modify: `Utils/PaintImage.hpp`
- Test: `Tests/SimulationResultGalleryTests.cpp`

- [ ] 抽出统一绘图入口，至少覆盖：
  - `renderScalarFieldDetail(...)`
  - `renderSeriesDetail(...)`
  - `renderMatrixDetail(...)`
  - `renderPreviewPixmap(...)`
- [ ] 不再把全部二维场图强绑到 `gpJet + [-120, 0]`。
- [ ] 固定配色口径：
  - 总场/参考发射机：`gpJet`
  - `SCF`：自定义 `magma`
  - `T_elev`：自定义 `inferno`
  - `D_desense`：风险分段色带
  - `S3I`：蓝实线、红虚线、灰色差值带
- [ ] 对 `ScalarField2D.noDataValue` 和 `NaN` 使用 `QCPColorMapData::setAlpha(...)` 做透明单元格。
- [ ] `S3I` 详情图使用 `QCPGraph::setChannelFillGraph(...)` 绘制曲线间填充带。
- [ ] `SCF` 详情图使用：
  - `QCPColorMap`
  - `QCPAxisTickerText`
  - `QCPItemText`
  - 必要的网格/边界线
- [ ] 缩略图统一走离屏 `QCustomPlot::toPixmap(...)`，禁止单独维护另一套缩略图绘制逻辑。
- [ ] 写 Catch2 测试或最小渲染验证，至少覆盖：
  - 六类卡片都能拿到非空预览图
  - `noDataValue` 不会被画成最低色块

### Task 3: 仿真页布局重构

**Files:**
- Modify: `Resource/ui/Simulation.h`
- Modify: `Resource/ui/Simulation.cpp`
- Possibly Create: `Resource/ui/SimulationPreviewCard.h`
- Possibly Create: `Resource/ui/SimulationPreviewCard.cpp`

- [ ] 用 `ElaScrollPageArea` + 卡片区替换现有“单一 `_plot` 占满页面”的结果布局。
- [ ] 详情区改为 `QStackedWidget` 管理三类详情视图：
  - `ScalarField2D`
  - `Series1D`
  - `LabeledMatrix2D`
- [ ] 卡片优先使用 `ElaInteractiveCard`；只有在需要显式选中态时才增加极薄包装。
- [ ] 成功态默认选中“总场分布”卡片。
- [ ] 失败态和取消态继续沿用现有状态提示，不展示伪结果卡。
- [ ] 保留现有开始/取消按钮和状态标签，不在本轮改动任务控制语义。

### Task 4: 页面接线与状态切换

**Files:**
- Modify: `Resource/ui/Simulation.cpp`
- Modify: `Resource/ui/Simulation.h`
- Test: `Tests/SimulationResultGalleryTests.cpp`

- [ ] 将 `onWorkerFinished(...)` 从“直接画 `aggregatedField`”改为：
  - 校验结果
  - 缓存 `SimulationTaskResult`
  - 生成目录
  - 生成卡片缩略图
  - 默认选中并渲染详情区
- [ ] 点击卡片时只做本地切换，不触发任何重新求解或指标计算。
- [ ] 保留“当前结果对应旧输入”的提示逻辑。
- [ ] 详情标题和摘要最少包含：
  - 图名
  - 单位
  - 关键指标摘要
  - 参考链路或受害机信息（适用时）
- [ ] 增加最小 UI 行为测试或目录级逻辑测试，覆盖默认选中、切换详情类型和摘要刷新。

### Task 5: 文档与回归清理

**Files:**
- Modify: `docs/项目架构与API规范.md`
- Modify: `docs/风险与改进建议.md`
- Modify: `docs/TODO.md`（如需只补路线说明）
- Modify: `CMakeLists.txt`
- Test: `Tests/SimulationResultGalleryTests.cpp`

- [ ] 在架构文档中补充“仿真页结果总览/详情区”与“三类详情视图”的新边界。
- [ ] 在风险文档中把“结果对象已具备、展示层待扩展”的风险更新为“多图展示框架已落地/待验证”。
- [ ] 如新增测试文件，接入 `CMakeLists.txt`。
- [ ] 让新增测试至少覆盖：
  - 目录抽取
  - 参考发射机过滤
  - 三类视图选择
  - 缩略图生成

## Acceptance Criteria

- 仿真成功后，页面能稳定显示六张固定卡片
- 卡片只包含一个参考发射机结果，不展开全部 `emitterResults`
- 点击任意卡片后，详情区能切换到正确图型
- `SCF` 详情图是矩阵式表格热图，不是普通折线图或纯文本表
- `S3I` 详情图显示双曲线与灰色差值带
- `T_elev` 与 `D_desense` 配色与 notebook 口径一致，不回退到统一 `jet`
- 缩略图由 `QCustomPlot::toPixmap(...)` 生成
- UI 层没有新增对 `EMCMetricsCalculator` 的直接依赖

## Assumptions

- 本轮不处理报告生成、结果导出和历史任务
- 本轮不处理验证侧代码
- 中文文案与校验提示仍保持中文
- 如果 Ela 无现成图表控件，允许使用 `QCustomPlot` 作为图表核心，但卡片和页面容器仍优先使用 Ela 组件
