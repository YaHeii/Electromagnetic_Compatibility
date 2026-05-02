# Reportflow Agent 闭环实验设计

## 1. 背景与目标

当前 `Reportflow` 已具备最小 `template-only` 报告能力：主程序导出一次成功仿真的 bundle，Python 侧读取 `simulation-result.json`、`report-context.json` 与静态资源，生成 Markdown/HTML 报告。

本轮设计目标是在此基础上升级为“结果驱动的实验编排 agent”：

- 输入仍以一次成功仿真的 `SimulationTaskResult` 为基准
- agent 先诊断基准结果，再自主设计最多 `5` 组补充实验
- 每组补充实验必须落成主程序现有标准输入 `JSONC`
- Reportflow 通过主程序新增的 headless runner 批量执行实验
- 最终输出基准分析、候选对比、推荐方案与风险说明

第一版目标固定为“改进建议优先”，不是通用参数扫描器，也不是开放式自动优化器。

## 2. 目录与边界

### 2.1 文档目录

- 设计文档：`docs/Reportflow/specs/`
- 实施计划：`docs/Reportflow/plans/`

### 2.2 Python 真实工作区

第一版 Python 运行时统一使用现有目录：

- `Reportflow/establishReport/`

其中：

- `Reportflow/establishReport/reportflow/`：主 workflow、bundle 读写、provider 适配、模板渲染
- `Reportflow/establishReport/templates/`：Markdown/HTML 模板
- `Reportflow/establishReport/tests/`：Python 侧测试

不再平行引入第二套 `Reportflow/python/` 根目录。

### 2.3 C++ 兼容层目录

第一版 C++ 侧相关能力放在现有主程序结构内：

- `Interface/`：Reportflow 协议结构、结果/请求对象
- `Utils/Reportflow/`：bundle 导出、上下文构建、标准输入导出
- 独立 runner 可执行入口：单独源文件和 CMake target

主程序 GUI 与 Reportflow agent 通过“文件 bundle + headless runner”协作，不引入 IPC 常驻服务。

## 3. 总体工作流

升级后的 Reportflow 工作流固定为三阶段：

1. `baseline diagnosis`
   - 读取基准 `ReportJobBundle`
   - 结合 `SimulationTaskResult`、`report-context.json` 与标准输入 `baseline-input.jsonc`
   - 诊断当前主要问题和可能的改进方向
2. `experiment orchestration`
   - 受限 agent 设计补充实验
   - 落成标准输入 `JSONC`
   - 调用主程序 headless runner 逐个执行
3. `comparison + recommendation`
   - 收集补充实验结果
   - 使用固定排序规则筛选最佳候选
   - 渲染最终 Markdown/HTML 报告

LLM 仅嵌入在少数决策阶段，不接管主流程状态机。

## 4. Bundle 协议演进

### 4.1 基准 bundle 保留项

继续保留现有文件：

- `request.json`
- `simulation-result.json`
- `report-context.json`
- `status.json`
- `assets/*`

### 4.2 新增文件

新增：

- `baseline-input.jsonc`

它必须是主程序标准输入 schema 的原始外部输入，不允许从 `simulation-result.json.inputSnapshot` 反推生成。

### 4.3 request.json 演进

`request.json.mode` 扩展为：

- `template-only`
- `agent-experiment`

`request.json` 新增 `agent` 配置块，第一版至少包含：

- `goalMode`
- `maxExperimentCount`
- `mutationScopes`
- `rankingPolicy`
- `providerProfile`

第一版固定默认：

- `goalMode = improvement`
- `maxExperimentCount = 5`

### 4.4 status.json 阶段

`status.json.stage` 扩展为：

- `validate_bundle`
- `diagnose_baseline`
- `plan_experiments`
- `materialize_inputs`
- `run_experiments`
- `rank_candidates`
- `render_markdown`
- `render_html`
- `completed`

若任一阶段失败，状态文件必须保留失败阶段、失败时间和可读错误摘要。

### 4.5 工作目录结构

第一版 `agent-experiment` 模式目录结构如下：

```text
<job-dir>/
  request.json
  status.json
  baseline-input.jsonc
  simulation-result.json
  report-context.json
  assets/
  experiments/
    plan.json
    <experimentId>/
      input.jsonc
      simulation-result.json
      report-context.json
  outputs/
    comparison-summary.json
    final-report.md
    final-report.html
```

## 5. C++ 主程序侧职责

### 5.1 baseline-input.jsonc 导出

主程序需要新增“从当前标准输入或已加载数据模型导出标准输入 JSONC”的能力，用于生成：

- `baseline-input.jsonc`

该导出能力的目标是确保：

- 重新读回时仍走现有 `JsonLoader`
- agent 补充实验永远基于主程序正式输入格式工作
- Reportflow 不依赖内部快照结构反推外部输入

### 5.2 headless runner

第一版必须新增独立无 UI 仿真执行入口，建议为独立可执行文件，例如：

- `EMC_SimRunner`

CLI 形态固定为：

```text
EMC_SimRunner --input <schema.jsonc> --output-dir <dir>
```

runner 职责固定为：

- 读取标准输入 schema
- 走现有 `DataModel -> simSchedulerCtx -> SimulationTaskResult` 主链路
- 输出标准 `simulation-result.json`
- 必要时输出 `report-context.json`
- 默认不导出全量图片

第一版图片策略：

- 基准任务沿用主程序正式导出的六张图
- 补充实验批跑阶段默认只落结果 JSON
- 仅最终推荐候选补导出正式图像资产

## 6. Python agent 运行时设计

### 6.1 总体原则

Python 侧采用“自建垂直 workflow + provider 适配层 + Pydantic 模型”：

- workflow 自己管理状态和阶段推进
- provider 通过统一适配层接入
- agent 只通过受限工具操作，不自由改任意字段

第一版不接入通用 agent framework 接管整个主流程。

### 6.2 provider 方案

第一版默认采用：

- `LiteLLM` 作为 provider 适配路线

要求：

- 至少接入一个真实 provider
- provider 细节不得散落在业务逻辑中
- 无 provider 或 provider 失败时，流程可退回纯规则路径或明确失败，不允许静默跳过

### 6.3 工具层

第一版工具层固定为：

- `load_baseline_bundle`
- `summarize_current_metrics`
- `propose_experiment_batch`
- `validate_experiment_candidate`
- `materialize_input_jsonc`
- `run_headless_simulation`
- `load_candidate_results`
- `rank_candidates`
- `render_final_report`

其中：

- `propose_experiment_batch` 必须返回结构化 `Pydantic` 模型
- 不接受自由文本实验清单作为流程输入

## 7. 实验设计约束

### 7.1 允许改动范围

第一版允许改动的实验维度固定为：

- 船只平面位置与朝向
- 环境参数中的 `windSpeed`
- 环境参数中的 `ductHeight`
- `emcAnalysisConfig` 中的参考链路选择
- 发射机少量参数：
  - `powerDbm`
  - `antennaPhiDeg`
  - `beamWidthDeg`

### 7.2 禁止改动范围

第一版禁止：

- 增删船只
- 增删设备
- 修改 `schemaVersion`
- 修改设备类型、极化、天线类型
- 修改中心频率、带宽
- 绕开现有 schema 校验
- 生成超过 `5` 组附加实验

### 7.3 约束表达方式

“预设参数功能”在第一版不实现为预置实验队列，而是收敛为：

- 一套显式的实验约束
- 一份 agent 行为 spec
- 一组结构化工具

这意味着 agent 能设计实验，但不能越权修改领域边界。

## 8. 候选排序与推荐规则

候选实验排序规则固定为：

1. 先过滤
   - 输入校验失败直接丢弃
   - headless 仿真失败直接丢弃
   - 结果对象校验失败直接丢弃
2. 主优化目标
   - 优先最小化 `D_desense.peakDb`
   - 其次最小化 `D_desense.coveragePercent`
   - 其次最小化 `D_desense.adiDbPerSquareMeter`
3. 次优化目标
   - 最小化 `T_elev.maxDb`
   - 最小化 `T_elev.meanDb`
4. 诊断与约束指标
   - `SCF.scalarDb` 与 `S3I.scalarDb` 只作为约束/解释，不作为主目标
   - 若任一指标相对基准恶化超过默认阈值 `0.5 dB`，候选实验标记为 `risky`
   - `risky` 候选不作为首推方案，但仍在报告中展示
5. 平局规则
   - 优先选择相对基准改动最小的实验

排序必须是确定性的，不能因 LLM 文本差异改变 winner。

## 9. 最终报告内容

最终报告必须包含：

- 基准结论
- 实验设计理由
- 各候选实验对比表
- 推荐实验
- 风险候选说明

第一版仍输出：

- `final-report.md`
- `final-report.html`

其中实验设计理由允许引用 LLM 解释，但候选排序与推荐结论必须由固定规则得出。

## 10. 失败处理与稳健性

第一版必须显式处理以下失败场景：

- bundle 缺文件或结构不完整
- `baseline-input.jsonc` 无法解析
- provider 不可用或返回结构化结果失败
- 候选实验超出允许改动范围
- headless runner 启动失败
- 单个实验仿真失败
- 所有补充实验均失败
- 报告渲染阶段失败

处理原则：

- 失败写入 `status.json`
- 尽可能保留中间产物
- 单个候选失败不应直接中断整个任务，除非已无可用候选或基准输入损坏

## 11. 非目标

第一版明确不做：

- 桌面 UI 自动操控
- 点击主窗口驱动补充实验
- 全量预设实验库扫描
- 跨 schema 多版本兼容适配
- 让 Python 直接接管主仿真计算
- 让 LLM 自由输出未约束的实验输入文件

## 12. 收敛结论

本方案的核心收敛点如下：

- 主程序继续负责标准输入解析、主链路仿真和正式结果输出
- Reportflow 负责实验编排、候选排序和报告汇总
- C++ / Python 之间通过 bundle 文件和 headless runner 建立兼容层
- 第一版采用受限 agent，而不是开放式自动调参系统
- 实验输入始终回到主程序标准 schema，不引入第二套输入协议
