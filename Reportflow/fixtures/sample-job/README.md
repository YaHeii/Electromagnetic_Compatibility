# sample-job 说明

这是一个最小的 Reportflow 任务目录样例，仅用于说明 Python 侧 MVP 需要读取的 bundle 结构。

## 目录结构

```text
sample-job/
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
  logs/
```

## 运行方式

在 `Reportflow/python/` 目录下安装或直接执行包后，使用以下命令处理任务目录：

```bash
python -m reportflow.cli --job-dir <sample-job-目录>
```

## 约定

- `request.json` 负责描述本次报告任务
- `simulation-result.json` 负责提供仿真结果
- `report-context.json` 负责提供报告上下文
- `status.json` 由 Python CLI 更新为 `running`、`succeeded` 或 `failed`

本样例只说明目录结构，不包含 LLM、API key、PDF 或网络相关能力。
