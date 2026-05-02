# Reportflow Agent 闭环实验实施计划

## 1. 实施目标

基于现有 `template-only` Reportflow，补齐“基准结果诊断 -> 补充实验设计 -> headless 跑批 -> 候选排序 -> 最终报告”闭环能力。

本计划仅定义实施顺序、交付边界和验证口径，不改变既有主程序职责分层。

## 2. 真实目录口径

### 2.1 文档目录

- `docs/Reportflow/specs/`
- `docs/Reportflow/plans/`

### 2.2 Python 目录

- `Reportflow/establishReport/reportflow/`
- `Reportflow/establishReport/templates/`
- `Reportflow/establishReport/tests/`

### 2.3 C++ 目录

- `Interface/`
- `Utils/Reportflow/`
- `entry/`
- 独立 runner 源文件与对应 `CMakeLists.txt` target

## 3. 分阶段任务

### 阶段 A：协议与文档收口

目标：

- 明确 `agent-experiment` 模式 bundle 结构
- 固定 `request.json`、`status.json`、`baseline-input.jsonc` 语义
- 固定 Python / C++ 目录口径

输出：

- `docs/Reportflow/specs/2026-05-01-reportflow-agent-experiment-design.md`
- `docs/Reportflow/plans/2026-05-01-reportflow-agent-experiment-implementation.md`

### 阶段 B：C++ 兼容层补齐

目标：

- 让主程序能导出 `baseline-input.jsonc`
- 新增 headless 仿真入口供 Reportflow 批量调用
- 扩展 Reportflow bundle 协议对象与导出逻辑
- 保持现有 `DataModel -> simSchedulerCtx -> SimulationTaskResult` 主链路不分叉

子任务：

1. 扩展现有 `Interface/ReportFlowContract.h`
2. 在 `Utils/Reportflow/` 收口统一 JSON bridge
3. 新增标准输入导出能力
4. 新增 headless runner
5. 扩展 bundle 目录落盘逻辑

验收：

- 主程序可导出标准输入 `JSONC`
- runner 可读取标准输入并输出 `simulation-result.json`
- `agent-experiment` 模式 bundle 目录完整
- runner 与 GUI 对同一输入输出同口径 `SimulationTaskResult`

### 阶段 C：Python agent workflow MVP

目标：

- 在现有 Reportflow CLI 基础上新增 `agent-experiment` 流程
- 保持模板报告输出能力
- 接入受限实验设计与固定排序

子任务：

1. 读取 bundle 与基准结果
2. 构建结构化实验计划模型
3. 校验候选改动范围
4. materialize 标准输入 `JSONC`
5. 调用 headless runner 执行实验
6. 收集候选结果并排序
7. 渲染最终 Markdown/HTML 报告

验收：

- 单次任务最多生成 `5` 组补充实验
- 每组实验都能落成标准输入 `JSONC`
- 排序结果稳定且不依赖自由文本解释

### 阶段 D：验证与收口

目标：

- 补齐 C++ / Python 两侧最小测试
- 收口失败处理与状态文件
- 明确用户手动验证路径

子任务：

1. C++ 兼容层测试
2. Python workflow 单元测试
3. 端到端 bundle 目录与产物核对
4. 文档同步补充风险与限制

## 4. C++ 侧实施拆分

### 4.1 协议层

需要扩展：

- `request.json.mode`
- `request.json.agent`
- `status.json.stage`
- `baseline-input.jsonc`
- `experiments/` 与 `outputs/` 目录约定

建议职责：

- 继续使用现有 `Interface/ReportFlowContract.h` 作为唯一 contract 头文件
- `Interface/ReportFlowContract.h` 统一定义：
  - 文件名与目录名常量
  - `mode / state / stage` 枚举
  - request/status 相关固定字段名
  - agent-experiment 所需 DTO/结构定义
- `Utils/Reportflow/` 负责序列化、反序列化与目录导出
- `docs/schema/` 同步补充 request/status 等协议 schema 文档

约束：

- 本轮不新增平行的 `ReportflowJsonContract.h`
- `ReportFlowContract.h` 只放协议定义，不放文件读写和 CLI 实现

### 4.2 JSON bridge 工作层

目标：

- 让 C++ 与 Python 的文件协议交换统一走 `Utils/Reportflow/`
- 避免 `EMC_SimRunner`、GUI、bundle exporter 各自手写 JSON

建议职责：

- `SimulationTaskResult -> simulation-result.json`
- `SimulationTaskResult -> report-context.json`
- request/status 对象 -> JSON
- 标准输入 -> `baseline-input.jsonc`
- bundle 目录创建与落盘

落地要求：

- 现有 `ReportJobExporter.cpp` 中的私有 JSON 拼装逻辑应逐步抽到可复用实现
- `EMC_SimRunner` 不直接定义协议字段，不直接复制 JSON 拼装代码

### 4.3 baseline-input 导出

需要新增：

- 从主程序正式输入导出标准 `JSONC` 的能力

约束：

- 输出必须可被现有 `JsonLoader` 再次读回
- 不允许依赖 `SimulationTaskResult.inputSnapshot` 反推
- `baseline-input.jsonc` 直接复用当前主程序标准输入 schema

### 4.4 headless runner

收敛结论：

- 本轮先新增一个薄 `EMC_SimRunner`
- 不额外引入新的共享仿真应用服务
- 直接复用现有 `JsonLoader + DataModel + simSchedulerCtx` 主链路

建议新增独立目标与目录：

- `EMC_SimRunner`
- `entry/EMC_SimRunner.cpp`

CLI 固定：

```text
EMC_SimRunner --input <schema.jsonc> --output-dir <dir>
```

输出最小集合：

- `simulation-result.json`
- 可选 `report-context.json`

第一版默认不导出图片。

实现边界：

- `EMC_SimRunner` 只负责参数解析、加载输入、调用仿真主链路和结果落盘
- 不在 runner 中重写仿真逻辑
- 不在 runner 中内嵌业务排序或报告逻辑

### 4.5 bundle 输出策略

基准任务：

- 保留当前正式图像资产输出

补充实验：

- 默认只输出结果 JSON
- 最终推荐实验再补导出图像资产

### 4.6 当前已知兼容层风险

- 当前 `EMC_Engine` 仍存在固定文件名 CSV 输出副作用，批量实验阶段需要评估是否继续保留
- 现有 `Reportflow` 仍是 `template-only`，因此 runner 打通后仍需后续 Python workflow 配合才能形成完整闭环
- 若 `simulation-result.json` 的序列化逻辑继续留在单个 exporter 私有实现中，后续 runner 与 bundle 导出会出现重复实现风险

## 5. Python 侧实施拆分

### 5.1 workflow 模块

建议拆分为：

- `bundle loading`
- `baseline diagnosis`
- `experiment planning`
- `input materialization`
- `simulation execution`
- `result ranking`
- `report rendering`

workflow 自己维护阶段推进，不让第三方 agent framework 接管。

### 5.2 provider 适配层

第一版采用：

- `LiteLLM`

要求：

- 至少一个真实 provider
- provider 适配独立封装
- provider 失败要显式透出到状态文件

### 5.3 结构化模型

建议使用 `Pydantic` 定义：

- agent 请求配置
- 实验候选对象
- 实验改动集
- 批量计划结果
- 排序摘要

要求：

- `propose_experiment_batch` 返回结构化模型
- 不接受自由文本实验清单直接驱动后续步骤

### 5.4 工具层

需要实现：

- `load_baseline_bundle`
- `summarize_current_metrics`
- `propose_experiment_batch`
- `validate_experiment_candidate`
- `materialize_input_jsonc`
- `run_headless_simulation`
- `load_candidate_results`
- `rank_candidates`
- `render_final_report`

## 6. 实验约束落地要求

### 6.1 允许改动

- 船只平面位置与朝向
- `environment.windSpeed`
- `environment.ductHeight`
- `emcAnalysisConfig` 参考链路
- 发射机：
  - `powerDbm`
  - `antennaPhiDeg`
  - `beamWidthDeg`

### 6.2 禁止改动

- 增删船只
- 增删设备
- 修改 `schemaVersion`
- 修改设备类型、极化、天线类型
- 修改中心频率、带宽
- 绕过 schema 校验
- 超过 `5` 组补充实验

### 6.3 失败处理

任一候选若：

- 越权修改字段
- 输入生成失败
- 仿真失败
- 结果对象校验失败

则应标记失败并进入报告说明，不允许伪装为有效候选。

## 7. 排序策略落地要求

固定排序顺序：

1. 最小化 `D_desense.peakDb`
2. 最小化 `D_desense.coveragePercent`
3. 最小化 `D_desense.adiDbPerSquareMeter`
4. 最小化 `T_elev.maxDb`
5. 最小化 `T_elev.meanDb`

辅助规则：

- `SCF.scalarDb`、`S3I.scalarDb` 只作约束与解释
- 相对基准恶化超过 `0.5 dB` 标记为 `risky`
- `risky` 候选不能作为首推方案
- 平局时选择相对基准改动最小者

## 8. 测试与验收清单

### 8.1 C++ 侧

- `baseline-input.jsonc` 导出后可被现有 `JsonLoader` 读回
- headless runner 输出结果与当前主链路同口径
- `agent-experiment` 模式 bundle 目录完整
- 状态阶段写入正确
- `ReportFlowContract.h` 与 `docs/schema/` 文档字段口径一致
- `EMC_SimRunner` 不依赖 GUI 页面对象即可运行

### 8.2 Python 侧

- 基准 bundle 可成功读取
- 可生成结构化实验计划
- 越权候选会被拦截
- 单次最多落成 `5` 组实验
- 每组实验都能 materialize 为标准输入 `JSONC`
- 仿真失败时 `status.json` 与日志正确回传
- 排序规则稳定

### 8.3 端到端

- 基准 bundle -> 设计实验 -> headless 跑批 -> 收集结果 -> 输出 `final-report.md/html`
- 最终报告必须包含：
  - 基准结论
  - 实验设计理由
  - 各候选实验对比表
  - 推荐实验
  - 风险候选说明

## 9. 风险与注意事项

### 9.1 当前主要风险

- 主程序目前若无独立 headless 入口，则 Python workflow 无法真正闭环
- 若缺少标准输入正式导出能力，agent 将被迫依赖内部快照反推，协议不稳定
- 若排序逻辑混入自由文本判断，winner 可能随模型波动变化
- 若补充实验默认导出全量图片，批量任务成本会显著膨胀

### 9.2 本轮不处理

- 桌面 UI 自动操控
- 预设实验队列
- 验证侧兼容
- Python 接管主程序仿真计算
- 多 schema 版本兼容转换

## 10. 建议执行顺序

建议按以下顺序推进实现：

1. 先扩现有 `Interface/ReportFlowContract.h`，同步补 `docs/schema/`
2. 再收口 `Utils/Reportflow/` 的统一 JSON bridge
3. 再做 C++ 标准输入导出
4. 再新增 `entry/EMC_SimRunner.cpp`
5. 再扩展 bundle 导出逻辑
6. 再做 Python workflow 与 provider 适配
7. 最后补测试与端到端验证

这样可以先稳定 C++ / Python 之间的兼容层，再接入 agent 决策能力，避免边做边改协议。
