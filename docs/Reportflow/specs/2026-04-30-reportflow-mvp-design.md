# Reportflow MVP 设计

日期：2026-04-30

## Summary

本设计用于为主程序补齐最小可验证的报告工作流。第一版固定为：

- 使用 `ReportJobBundle` 作为 C++ 与 Python 之间的兼容层
- 主程序负责导出 `SimulationTaskResult`、报告所需图片和任务请求文件
- `Reportflow` 负责读取 bundle，生成纯模板 `Markdown + HTML` 报告
- 第一版不接 LLM，不处理 API key，不做网络依赖

目标不是一次做完整报告系统，而是先验证三件事：

1. `SimulationTaskResult` 是否能稳定收敛为报告消费边界
2. C++ 与 Python 的文件包协议是否足够清晰和可演进
3. 纯模板工作流是否能稳定产出 `md/html`

## 1. 目录边界

### 1.1 C++ 侧目录配置

主程序侧不新建并行子系统，继续沿用现有工程分层。

- `Interface/ReportFlowContract.h`
  - 固定 bundle 版本、标准文件名、阶段状态枚举和 key 常量
- `Utils/Reportflow/ReportContextBuilder.h`
- `Utils/Reportflow/ReportContextBuilder.cpp`
  - 负责从 `SimulationTaskResult` 提取报告上下文
- `Utils/Reportflow/ReportJobExporter.h`
- `Utils/Reportflow/ReportJobExporter.cpp`
  - 负责创建任务目录、导出 `json`、导出图片、写初始状态文件
- `Resource/ui/Simulation.cpp`
  - 后续只负责触发“生成报告”动作，不承接报告业务逻辑

第一版不要求新增独立报告页，不要求在 UI 内嵌 Python 运行状态详情。

### 1.2 Python 侧目录配置

Python 代码统一放在顶层 `Reportflow/` 目录下，和主程序 C++ 代码解耦。

推荐结构如下：

```text
Reportflow/
  specs/
  plans/
  python/
    pyproject.toml
    reportflow/
      __init__.py
      cli.py
      contracts/
        request.py
        status.py
        context.py
      loaders/
        bundle_loader.py
      renderers/
        markdown_renderer.py
        html_renderer.py
      templates/
        report.md.j2
        report.html.j2
      utils/
        fs.py
        errors.py
  fixtures/
    sample-job/
  workdir/
```

说明：

- `Reportflow/python/reportflow/` 是 Python 包根目录
- `Reportflow/fixtures/` 存放最小 bundle 样例，用于 Python 侧自测
- `Reportflow/workdir/` 存放运行时任务目录，不纳入版本控制

### 1.3 运行时工作目录

运行时每个任务一个目录，按 `taskId` 隔离：

```text
Reportflow/workdir/<taskId>/
  request.json
  simulation-result.json
  report-context.json
  status.json
  assets/
    aggregated-field.png
    reference-emitter.png
    scf-matrix.png
    s3i-curve.png
    t-elev.png
    d-desense.png
  outputs/
    report.md
    report.html
  logs/
    reportflow.log
```

## 2. 兼容层设计

### 2.1 为什么不用 `SimulationTaskResult` 直接做 Python 输入

`SimulationTaskResult` 是主程序正式结果对象，但不是最适合的报告消费 DTO。

第一版兼容层采用双层输入：

- `simulation-result.json`
  - 原始、完整、可追溯
- `report-context.json`
  - 报告专用、稳定、收敛后的上下文

这样可以保证：

- Python 侧不需要理解完整结果 schema 的每个细节
- 后续 `SimulationTaskResult` 字段扩张时，不会强迫模板层同步重构
- 报告模板只消费当前真正需要的字段

### 2.2 `ReportJobBundle` 文件清单

第一版 bundle 固定包含：

- `request.json`
- `simulation-result.json`
- `report-context.json`
- `status.json`
- `assets/*.png`

`status.json` 是唯一状态真相源，不能通过“某张图片是否存在”判断任务成功。

### 2.3 版本字段

第一版至少保留以下版本字段：

- `request.json.reportBundleVersion`
- `simulation-result.json.resultSchemaVersion`
- `report-context.json.reportContextVersion`

这样可以把：

- 主程序结果结构演进
- 报告兼容层演进
- Python 模板演进

三者分开管理。

## 3. 文件内容口径

### 3.1 `request.json`

职责：定义这次报告任务如何执行。

第一版字段建议固定为：

```json
{
  "reportBundleVersion": "1.0.0",
  "taskId": "task-001",
  "mode": "template-only",
  "language": "zh-CN",
  "templateId": "default-emc-report",
  "outputFormats": ["md", "html"],
  "inputFiles": {
    "simulationResult": "simulation-result.json",
    "reportContext": "report-context.json"
  },
  "assetFiles": {
    "aggregatedField": "assets/aggregated-field.png",
    "referenceEmitter": "assets/reference-emitter.png",
    "scf": "assets/scf-matrix.png",
    "s3i": "assets/s3i-curve.png",
    "tElev": "assets/t-elev.png",
    "dDesense": "assets/d-desense.png"
  }
}
```

第一版不出现：

- `llm`
- `provider`
- `apiKey`
- 远端 URL

### 3.2 `report-context.json`

职责：给模板层提供稳定、简洁的报告输入。

第一版建议只包含：

- 任务元信息
- 输入快照中的报告相关字段
- 六张主图的标题、副标题、文件映射
- `SCF / S3I / T_elev / D_desense` 的标量摘要
- 主程序已给出的 `summaryText`

第一版不在这里重复保存完整二维数组。

### 3.3 `status.json`

职责：由 Python 侧更新报告流程状态。

第一版状态集固定为：

- `pending`
- `running`
- `succeeded`
- `failed`
- `cancelled`

阶段字段 `stage` 第一版建议至少支持：

- `validate_bundle`
- `render_markdown`
- `render_html`
- `completed`

## 4. 主程序侧流程

第一版主程序侧流程固定为：

1. 用户在主程序中触发“生成报告”
2. 从当前成功的 `SimulationTaskResult` 创建 `job-dir`
3. 导出 `simulation-result.json`
4. 构建 `report-context.json`
5. 导出六张正式图片到 `assets/`
6. 写入 `request.json`
7. 写入初始 `status.json = pending`
8. 调用 Python CLI 处理该任务目录

第一版主程序只负责启动 Python，不负责解释模板业务。

## 5. Python 侧流程

第一版 `Reportflow` CLI 流程固定为：

1. 读取 `job-dir`
2. 校验 `request.json`、`report-context.json`、图片文件是否齐全
3. 将 `status.json` 更新为 `running`
4. 渲染 `report.md`
5. 渲染 `report.html`
6. 更新 `status.json = succeeded`

失败时：

- 写 `status.json = failed`
- 写错误信息到 `errors`
- 保留日志文件

第一版不做：

- LLM 总结
- API 调用
- 增量重试
- 多模板动态插件机制

## 6. 失败与降级规则

### 6.1 C++ 侧

- 若任务结果不是成功态，不允许创建报告任务目录
- 若任一正式图片导出失败，整个报告任务创建失败
- 若 Python CLI 启动失败，保留 bundle，状态记为失败

### 6.2 Python 侧

- 缺少任何必需输入文件时，直接失败
- 不允许在缺图情况下生成“伪完整报告”
- 先写临时输出文件，再原子替换正式 `report.md/html`
- 不回写 `simulation-result.json`

## 7. MVP 范围

### 当前已实现目标

第一版只追求：

- C++ 和 Python 兼容层打通
- 任务目录结构固定
- 纯模板 `Markdown + HTML` 成功输出
- 错误状态可回传

### 明确不做

- LLM 总结
- API key 管理
- PDF 导出
- 主程序内报告浏览页
- Python 反向生成主程序展示图
- 多任务队列与并发调度

## 8. 后续演进口

如果 MVP 跑通，后续按下面顺序扩展：

1. 在 `request.json` 中新增 `llm.enabled/provider/model`
2. 在 Python 侧增加 `context_builder + llm + post_processor`
3. 增加 `outputs/final-report.md` 和 `outputs/final-report.html`
4. 最后再评估是否接 PDF 与 UI 内置查看器

## 9. 自检结论

本设计已经明确：

- C++ 侧目录归属
- Python 侧目录归属
- 运行时工作目录
- 兼容层协议
- MVP 范围与非目标

当前没有保留 `TODO/TBD` 占位项，也没有把 LLM、Python 绘图、UI 浏览器等后续需求混入首版范围。
