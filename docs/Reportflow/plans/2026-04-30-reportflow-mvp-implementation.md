# Reportflow MVP 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 基于现有 `SimulationTaskResult` 落地 Reportflow MVP，使主程序可以导出标准 `ReportJobBundle`，并由 Python 侧生成模板化 `Markdown + HTML` 报告。

**Architecture:** C++ 侧负责导出正式结果 JSON、图像资源和收敛后的 `report-context.json`，Python 侧只消费 bundle 协议并生成报告输出，不反向修改主结果对象。兼容层通过 `request.json / simulation-result.json / report-context.json / status.json / assets/*` 固定目录结构解耦。

**Tech Stack:** Qt6/C++17、QJson、QDir/QFile、Catch2、Python 3、Jinja2 风格模板、Markdown + HTML

---

## 文件结构

- Create: `docs/Reportflow/plans/2026-04-30-reportflow-mvp-implementation.md`
- Create: `Interface/ReportFlowContract.h`
- Create: `Utils/Reportflow/ReportContextBuilder.h`
- Create: `Utils/Reportflow/ReportContextBuilder.cpp`
- Create: `Utils/Reportflow/ReportJobExporter.h`
- Create: `Utils/Reportflow/ReportJobExporter.cpp`
- Create: `Tests/ReportflowTests.cpp`
- Create: `Reportflow/python/pyproject.toml`
- Create: `Reportflow/python/reportflow/__init__.py`
- Create: `Reportflow/python/reportflow/cli.py`
- Create: `Reportflow/python/reportflow/contracts/__init__.py`
- Create: `Reportflow/python/reportflow/loaders/__init__.py`
- Create: `Reportflow/python/reportflow/loaders/bundle_loader.py`
- Create: `Reportflow/python/reportflow/renderers/__init__.py`
- Create: `Reportflow/python/reportflow/renderers/markdown_renderer.py`
- Create: `Reportflow/python/reportflow/renderers/html_renderer.py`
- Create: `Reportflow/python/reportflow/templates/report.md.j2`
- Create: `Reportflow/python/reportflow/templates/report.html.j2`
- Create: `Reportflow/python/reportflow/utils/__init__.py`
- Create: `Reportflow/python/reportflow/utils/fs.py`
- Create: `Reportflow/python/reportflow/utils/errors.py`
- Create: `Reportflow/fixtures/sample-job/README.md`
- Modify: `.gitignore`
- Modify: `CMakeLists.txt`

### Task 1: 固定 C++ 兼容层协议

**Files:**
- Create: `Interface/ReportFlowContract.h`
- Test: `Tests/ReportflowTests.cpp`

- [ ] **Step 1: 先写失败测试，固定 bundle 关键文件名和状态枚举**

```cpp
TEST_CASE("ReportFlowContract exposes stable bundle file names", "[reportflow][contract]") {
    REQUIRE(QString::fromLatin1(ReportFlow::kRequestFileName) == "request.json");
    REQUIRE(QString::fromLatin1(ReportFlow::kSimulationResultFileName) == "simulation-result.json");
    REQUIRE(QString::fromLatin1(ReportFlow::kReportContextFileName) == "report-context.json");
    REQUIRE(QString::fromLatin1(ReportFlow::kStatusFileName) == "status.json");
}
```

- [ ] **Step 2: 在本地测试目标中编译，确认当前缺少 `ReportFlowContract.h` 而失败**

Run: `cmake --build <build-dir> --target SchemaDtoValidationTests`
Expected: 编译失败，报找不到 `Interface/ReportFlowContract.h` 或未定义常量

- [ ] **Step 3: 实现协议头文件，固定版本、目录名、文件名、任务状态和资产键**

```cpp
namespace ReportFlow {
inline constexpr char kBundleVersion[] = "1.0.0";
inline constexpr char kRequestFileName[] = "request.json";
inline constexpr char kSimulationResultFileName[] = "simulation-result.json";
inline constexpr char kReportContextFileName[] = "report-context.json";
inline constexpr char kStatusFileName[] = "status.json";
inline constexpr char kAssetsDirName[] = "assets";
inline constexpr char kOutputsDirName[] = "outputs";
inline constexpr char kLogsDirName[] = "logs";
}
```

- [ ] **Step 4: 重新编译测试目标，确认协议常量可用**

Run: `cmake --build <build-dir> --target SchemaDtoValidationTests`
Expected: `ReportFlowContract` 相关编译错误消失

### Task 2: 实现 C++ 侧 report-context 与 bundle 导出

**Files:**
- Create: `Utils/Reportflow/ReportContextBuilder.h`
- Create: `Utils/Reportflow/ReportContextBuilder.cpp`
- Create: `Utils/Reportflow/ReportJobExporter.h`
- Create: `Utils/Reportflow/ReportJobExporter.cpp`
- Test: `Tests/ReportflowTests.cpp`

- [ ] **Step 1: 先写失败测试，固定 `report-context.json`、`request.json` 和初始 `status.json` 结构**

```cpp
TEST_CASE("ReportJobExporter writes a complete template-only bundle", "[reportflow][export]") {
    const SimulationTaskResult result = makeSuccessfulResultForReportflow();
    const QString jobDir = ReportJobExporter::exportBundle(result, tempRoot.path()).jobDirectory;

    REQUIRE(QFileInfo::exists(jobDir + "/request.json"));
    REQUIRE(QFileInfo::exists(jobDir + "/simulation-result.json"));
    REQUIRE(QFileInfo::exists(jobDir + "/report-context.json"));
    REQUIRE(QFileInfo::exists(jobDir + "/status.json"));
}
```

- [ ] **Step 2: 编译测试目标，确认导出器尚不存在而失败**

Run: `cmake --build <build-dir> --target SchemaDtoValidationTests`
Expected: 编译失败，报 `ReportContextBuilder` / `ReportJobExporter` 未定义

- [ ] **Step 3: 最小实现 `ReportContextBuilder`，只收敛报告模板真正需要的摘要字段**

```cpp
QJsonObject ReportContextBuilder::build(const SimulationTaskResult& result) {
    QJsonObject root;
    root.insert("reportContextVersion", "1.0.0");
    root.insert("taskId", result.taskId);
    root.insert("summaryText", result.summaryText);
    root.insert("referenceTransmitterId", result.inputSnapshot.emcAnalysisConfig.referenceTransmitterId);
    root.insert("referenceReceiverId", result.inputSnapshot.emcAnalysisConfig.referenceReceiverId);
    return root;
}
```

- [ ] **Step 4: 最小实现 `ReportJobExporter`，创建目录、写四个 JSON 文件、预留 `assets/outputs/logs` 目录**

```cpp
ReportJobExportResult ReportJobExporter::exportBundle(
    const SimulationTaskResult& result,
    const QString& workRootDir);
```

- [ ] **Step 5: 在导出测试里校验 `request.json.mode == template-only`、`outputFormats == [md, html]`、`status.state == pending`**

```cpp
REQUIRE(requestObject.value("mode").toString() == "template-only");
REQUIRE(statusObject.value("state").toString() == "pending");
```

- [ ] **Step 6: 重新编译测试目标，确认 C++ 兼容层编译通过**

Run: `cmake --build <build-dir> --target SchemaDtoValidationTests`
Expected: `ReportflowTests.cpp` 编译通过

### Task 3: 搭建 Python Reportflow MVP 骨架

**Files:**
- Create: `Reportflow/python/pyproject.toml`
- Create: `Reportflow/python/reportflow/...`
- Create: `Reportflow/fixtures/sample-job/README.md`

- [ ] **Step 1: 先写最小 CLI 契约，固定调用方式**

```bash
python -m reportflow.cli --job-dir Reportflow/workdir/task-001
```

- [ ] **Step 2: 实现 bundle loader，只校验必要输入文件和图片存在**

```python
def load_bundle(job_dir: Path) -> Bundle:
    request = read_json(job_dir / "request.json")
    context = read_json(job_dir / "report-context.json")
    result = read_json(job_dir / "simulation-result.json")
    return Bundle(job_dir=job_dir, request=request, context=context, result=result)
```

- [ ] **Step 3: 实现 Markdown/HTML 渲染器和模板，占位展示任务摘要、四指标摘要和图片路径**

```python
markdown = render_markdown(bundle)
html = render_html(bundle)
```

- [ ] **Step 4: 实现 `status.json` 状态推进：`pending -> running -> succeeded/failed`**

```python
write_status(job_dir, state="running", stage="render_markdown")
```

- [ ] **Step 5: 增加 `Reportflow/fixtures/sample-job/README.md`，说明样例 bundle 结构和运行方式**

```text
Reportflow/fixtures/sample-job/
  request.json
  simulation-result.json
  report-context.json
  status.json
  assets/
```

### Task 4: 接线测试与目录配置

**Files:**
- Modify: `.gitignore`
- Modify: `CMakeLists.txt`
- Modify: `Tests/ReportflowTests.cpp`

- [ ] **Step 1: 增加工作目录和 Python 构建产物忽略规则**

```gitignore
Reportflow/workdir/
Reportflow/python/.pytest_cache/
Reportflow/python/dist/
Reportflow/python/build/
Reportflow/python/*.egg-info/
__pycache__/
```

- [ ] **Step 2: 将 `Tests/ReportflowTests.cpp` 和新增 `Utils/Reportflow/*.cpp` 接入测试目标**

```cmake
    Tests/ReportflowTests.cpp
    Utils/Reportflow/ReportContextBuilder.cpp
    Utils/Reportflow/ReportJobExporter.cpp
```

- [ ] **Step 3: 由用户手动执行构建与测试，返回关键结果**

Run: `cmake --build <build-dir> --target SchemaDtoValidationTests`
Expected: 新增 Reportflow 代码全部编译通过

Run: `ctest --output-on-failure -R SchemaDtoValidationTests`
Expected: `ReportflowTests` 与既有结果对象测试一起通过

## 执行备注

- 本计划按用户已确认的 `docs/Reportflow/specs/2026-04-30-reportflow-mvp-design.md` 执行。
- 本轮不接 LLM、API key、PDF、主程序内嵌报告浏览器。
- 本轮不处理验证侧兼容。
- 由于仓库约定，`cmake/ctest` 需由用户手动执行并回传结果。
