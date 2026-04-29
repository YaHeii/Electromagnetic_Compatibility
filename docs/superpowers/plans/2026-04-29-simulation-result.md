# Simulation Result Boundary Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为仿真主链路建立稳定的结果对象边界，让 UI、派生指标和报告都从 `SimulationTaskResult` 读取结果，而不再直接依赖裸 `GridMap`。

**Architecture:** 新增 `Interface/SimulationResult.*` 承载结果根对象与校验规则；将当前 `Propagation_Engine` 拆为单发射机求解器 `PEPropagationSolver`；让 `EMC_Engine` 只负责计算与聚合；新增任务级 façade `simSchedulerCtx` 组装 `SimulationTaskResult`；最后让 `Simulation.cpp` 与绘图入口消费 `ScalarField2D`。

**Tech Stack:** C++17, Qt Widgets, Catch2, Eigen, FFTW3, spdlog

---

## File Map

- Create: `Interface/SimulationResult.h`
  责任：定义 `FormationSource`、`SimulationResultStatus`、`EmitterResultStatus`、`ScalarFieldQuantity`、`ScalarField2D`、`EmitterResult`、`DerivedMetrics`、`SimulationTaskResult`，并提供内部 `validate()`。
- Create: `Simulation/EMCComputationResult.h`
  责任：定义 `EMC_Engine` 输出给 `simSchedulerCtx` 的原始计算结果结构，避免 UI 直接依赖引擎内部状态。
- Create: `Simulation/PEPropagationSolver.h`
- Create: `Simulation/PEPropagationSolver.cpp`
  责任：承接当前 `Propagation_Engine` 的单发射机 1D/2D PE 求解逻辑。
- Create: `Simulation/simSchedulerCtx.h`
- Create: `Simulation/simSchedulerCtx.cpp`
  责任：任务级调度、时间/状态记录、编队来源字段承接、根结果对象组装。
- Create: `Tests/SimulationResultTests.cpp`
  责任：结果对象与校验规则测试。
- Create: `Tests/SimulationSchedulerTests.cpp`
  责任：`simSchedulerCtx` 组装任务结果测试。
- Create: `docs/schema/simulation-result.schema.json`
- Create: `docs/schema/simulation-result.template.jsonc`
  责任：固定结果 schema 字段、类型、枚举和值域口径。
- Modify: `Simulation/EMC_Engine.h`
- Modify: `Simulation/EMC_Engine.cpp`
  责任：改为使用 `PEPropagationSolver`，输出 `EMCComputationResult` 而不是仅暴露 `_LossGrid`。
- Modify: `Resource/ui/Simulation.h`
- Modify: `Resource/ui/Simulation.cpp`
  责任：改为消费 `SimulationTaskResult`，保留状态机逻辑与旧结果过期提示。
- Modify: `Utils/PaintImage.hpp`
  责任：新增接受 `ScalarField2D` 的绘图入口。
- Modify: `Tests/SchemaDtoValidationTests.cpp`
  责任：保留现有 `EMC_Engine` 输入快照校验，同时迁移 include 依赖。
- Modify: `CMakeLists.txt`
  责任：接入新增源文件与测试文件。
- Modify: `docs/项目概览.md`
- Modify: `docs/项目架构与API规范.md`
- Modify: `docs/风险与改进建议.md`
  责任：同步结果对象边界已落地后的文档口径。

### Task 1: Add Failing Result Boundary Tests

**Files:**
- Create: `Tests/SimulationResultTests.cpp`
- Create: `Tests/SimulationSchedulerTests.cpp`
- Modify: `CMakeLists.txt`
- Test: `Tests/SimulationResultTests.cpp`
- Test: `Tests/SimulationSchedulerTests.cpp`

- [ ] **Step 1: Write failing tests for result structs**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "Interface/SimulationResult.h"

TEST_CASE("ScalarField2D validates row-major dimensions", "[result][field]") {
    ScalarField2D field;
    field.fieldId = "agg-field";
    field.displayName = QStringLiteral("总接收功率场");
    field.quantity = ScalarFieldQuantity::AggregatedPowerDbm;
    field.valueUnit = "dBm";
    field.axisXUnit = "m";
    field.axisYUnit = "m";
    field.rows = 2;
    field.cols = 3;
    field.originX = 0.0;
    field.originY = 0.0;
    field.stepX = 5.0;
    field.stepY = 5.0;
    field.values = {1.0, 2.0, 3.0, 4.0, 5.0};

    const auto validation = field.validate();
    REQUIRE_FALSE(validation.first);
}

TEST_CASE("SimulationTaskResult validates formation source contract", "[result][task]") {
    SimulationTaskResult result;
    result.resultSchemaVersion = "1.0.0";
    result.taskId = "task-1";
    result.status = SimulationResultStatus::Failed;
    result.formationSource = FormationSource::ManualInput;
    result.presetFormationId = 7;
    result.startedAtUtcMs = 100;
    result.finishedAtUtcMs = 200;
    result.durationMs = 100;
    result.errorMessage = QStringLiteral("mock error");

    const auto validation = result.validate();
    REQUIRE_FALSE(validation.first);
}
```

- [ ] **Step 2: Write failing tests for `simSchedulerCtx` result assembly**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "Simulation/EMCComputationResult.h"
#include "Simulation/simSchedulerCtx.h"

TEST_CASE("simSchedulerCtx assembles successful task result with preset formation metadata", "[result][scheduler]") {
    DataModel::DataSnapshot snapshot;
    snapshot.allShips.push_back(ShipData{});
    snapshot.allShips.back().shipId = "USV1";

    EMCComputationResult computation;
    computation.status = SimulationResultStatus::Succeeded;
    computation.aggregatedField.fieldId = "agg-field";
    computation.aggregatedField.displayName = QStringLiteral("总接收功率场");
    computation.aggregatedField.quantity = ScalarFieldQuantity::AggregatedPowerDbm;
    computation.aggregatedField.valueUnit = "dBm";
    computation.aggregatedField.axisXUnit = "m";
    computation.aggregatedField.axisYUnit = "m";
    computation.aggregatedField.rows = 1;
    computation.aggregatedField.cols = 1;
    computation.aggregatedField.stepX = 5.0;
    computation.aggregatedField.stepY = 5.0;
    computation.aggregatedField.values = { -42.0 };

    const SimulationTaskResult result = simSchedulerCtx::assembleResult(
        snapshot,
        FormationSource::PresetFormation,
        3,
        ModelType::PE,
        1000,
        1600,
        computation);

    REQUIRE(result.formationSource == FormationSource::PresetFormation);
    REQUIRE(result.presetFormationId.has_value());
    REQUIRE(result.presetFormationId.value() == 3);
    REQUIRE(result.status == SimulationResultStatus::Succeeded);
    REQUIRE(result.aggregatedField.values.size() == 1);
}
```

- [ ] **Step 3: Add new test files and future sources to the test target**

```cmake
add_executable(SchemaDtoValidationTests
    Tests/SchemaDtoValidationTests.cpp
    Tests/UiMainlineTests.cpp
    Tests/TreeViewModelTests.cpp
    Tests/SimulationResultTests.cpp
    Tests/SimulationSchedulerTests.cpp
    Interface/SimulationResult.h
    Simulation/EMCComputationResult.h
    Simulation/PEPropagationSolver.cpp
    Simulation/PEPropagationSolver.h
    Simulation/simSchedulerCtx.cpp
    Simulation/simSchedulerCtx.h
    ...
)
```

- [ ] **Step 4: Ask the user to run the red build/tests**

Run:

```powershell
cmake --build d:\code\Electromagnetic_compatibility\build --config Debug --target SchemaDtoValidationTests
ctest --output-on-failure -R "result|scheduler"
```

Expected:

- build fails because `SimulationResult.h` / `simSchedulerCtx` / `EMCComputationResult` do not exist yet
- or tests compile but fail due to missing `validate()` / `assembleResult()`

### Task 2: Implement Result Boundary Types

**Files:**
- Create: `Interface/SimulationResult.h`
- Modify: `CMakeLists.txt`
- Test: `Tests/SimulationResultTests.cpp`

- [ ] **Step 1: Add enums and value objects in `Interface/SimulationResult.h`**

```cpp
enum class FormationSource {
    ManualInput,
    PresetFormation
};

enum class SimulationResultStatus {
    Succeeded,
    Failed,
    Cancelled
};

enum class EmitterResultStatus {
    Succeeded,
    Failed,
    Cancelled,
    Skipped
};

enum class ScalarFieldQuantity {
    AggregatedPowerDbm,
    PathLossDb
};
```

- [ ] **Step 2: Add `ScalarField2D`, `EmitterResult`, `DerivedMetrics`, and `SimulationTaskResult` with inline `validate()`**

```cpp
struct ScalarField2D {
    QString fieldId;
    QString displayName;
    ScalarFieldQuantity quantity{ScalarFieldQuantity::AggregatedPowerDbm};
    QString valueUnit;
    QString axisXUnit;
    QString axisYUnit;
    int rows{0};
    int cols{0};
    double originX{0.0};
    double originY{0.0};
    double stepX{0.0};
    double stepY{0.0};
    std::optional<double> noDataValue;
    std::vector<double> values;

    std::pair<bool, QString> validate() const;
};
```

```cpp
struct SimulationTaskResult {
    QString resultSchemaVersion;
    QString taskId;
    ModelType modelType{ModelType::PE};
    SimulationResultStatus status{SimulationResultStatus::Succeeded};
    FormationSource formationSource{FormationSource::ManualInput};
    std::optional<int> presetFormationId;
    qint64 startedAtUtcMs{0};
    qint64 finishedAtUtcMs{0};
    qint64 durationMs{0};
    QString errorMessage;
    QString summaryText;
    DataModel::DataSnapshot inputSnapshot;
    ScalarField2D aggregatedField;
    std::vector<EmitterResult> emitterResults;
    DerivedMetrics derivedMetrics;

    std::pair<bool, QString> validate() const;
};
```

- [ ] **Step 3: Implement the concrete validation rules from the spec**

```cpp
inline std::pair<bool, QString> SimulationTaskResult::validate() const {
    if (resultSchemaVersion.trimmed().isEmpty()) {
        return {false, QStringLiteral("resultSchemaVersion 不能为空")};
    }
    if (taskId.trimmed().isEmpty()) {
        return {false, QStringLiteral("taskId 不能为空")};
    }
    if (formationSource == FormationSource::ManualInput && presetFormationId.has_value()) {
        return {false, QStringLiteral("ManualInput 不应携带 presetFormationId")};
    }
    if (formationSource == FormationSource::PresetFormation && !presetFormationId.has_value()) {
        return {false, QStringLiteral("PresetFormation 必须携带 presetFormationId")};
    }
    return {true, QString()};
}
```

- [ ] **Step 4: Ask the user to rebuild and run the new result tests**

Run:

```powershell
cmake --build d:\code\Electromagnetic_compatibility\build --config Debug --target SchemaDtoValidationTests
ctest --output-on-failure -R "ScalarField2D|SimulationTaskResult"
```

Expected:

- `SimulationResultTests` passes

### Task 3: Split `Propagation_Engine` into `PEPropagationSolver` and Refactor `EMC_Engine`

**Files:**
- Create: `Simulation/EMCComputationResult.h`
- Create: `Simulation/PEPropagationSolver.h`
- Create: `Simulation/PEPropagationSolver.cpp`
- Modify: `Simulation/EMC_Engine.h`
- Modify: `Simulation/EMC_Engine.cpp`
- Modify: `Tests/SchemaDtoValidationTests.cpp`
- Test: `Tests/SchemaDtoValidationTests.cpp`

- [ ] **Step 1: Extract the single-emitter solver class into `PEPropagationSolver.*`**

```cpp
class PEPropagationSolver {
public:
    PEPropagationSolver(ModelType modelType, const Fleet* fleet);
    LineMap compute1D(Transmitter_PE_data peData, EnvironmentData env, double receiverAntennaHeight);
    GridMatrix compute2D(Transmitter_PE_data peData, EnvironmentData env, double receiverAntennaHeight);

private:
    const Fleet* _fleet{nullptr};
    ModelType _modelType{ModelType::PE};
};
```

- [ ] **Step 2: Add `EMCComputationResult` for engine-to-scheduler handoff**

```cpp
struct EMCComputationResult {
    SimulationResultStatus status{SimulationResultStatus::Failed};
    QString errorMessage;
    ScalarField2D aggregatedField;
    std::vector<EmitterResult> emitterResults;
};
```

- [ ] **Step 3: Refactor `EMC_Engine` to own a `PEPropagationSolver` and fill `EMCComputationResult`**

```cpp
class EMC_Engine : public QObject {
    Q_OBJECT
public:
    ...
    const EMCComputationResult& computationResult() const {
        return _computationResult;
    }

private:
    std::unique_ptr<PEPropagationSolver> _propagationSolver;
    EMCComputationResult _computationResult;
};
```

```cpp
void EMC_Engine::do_PE_computing() {
    _computationResult = {};
    _computationResult.status = SimulationResultStatus::Failed;
    ...
    _computationResult.aggregatedField = makeAggregatedField(final_total_dbm, _env.dx);
    _computationResult.emitterResults.push_back(makeEmitterResult(pe_data, current_loss, _env.dx));
    _computationResult.status = SimulationResultStatus::Succeeded;
}
```

- [ ] **Step 4: Keep and re-run the existing frozen snapshot regression**

Run:

```powershell
ctest --output-on-failure -R "launch snapshot"
```

Expected:

- existing `EMC_Engine keeps the launch snapshot instead of re-reading DataModel` still passes

### Task 4: Implement `simSchedulerCtx` Task-Level Result Assembly

**Files:**
- Create: `Simulation/simSchedulerCtx.h`
- Create: `Simulation/simSchedulerCtx.cpp`
- Modify: `CMakeLists.txt`
- Test: `Tests/SimulationSchedulerTests.cpp`

- [ ] **Step 1: Add the task-level façade interface**

```cpp
class simSchedulerCtx {
public:
    static SimulationTaskResult assembleResult(
        const DataModel::DataSnapshot& inputSnapshot,
        FormationSource formationSource,
        std::optional<int> presetFormationId,
        ModelType modelType,
        qint64 startedAtUtcMs,
        qint64 finishedAtUtcMs,
        const EMCComputationResult& computationResult);

    static SimulationTaskResult run(
        ModelType modelType,
        const DataModel::DataSnapshot& inputSnapshot,
        FormationSource formationSource,
        std::optional<int> presetFormationId,
        std::unique_ptr<Fleet> fleet);
};
```

- [ ] **Step 2: Implement `assembleResult()` as the deterministic assembly seam used by tests**

```cpp
SimulationTaskResult simSchedulerCtx::assembleResult(...) {
    SimulationTaskResult result;
    result.resultSchemaVersion = QStringLiteral("1.0.0");
    result.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    result.modelType = modelType;
    result.status = computationResult.status;
    result.formationSource = formationSource;
    result.presetFormationId = presetFormationId;
    result.startedAtUtcMs = startedAtUtcMs;
    result.finishedAtUtcMs = finishedAtUtcMs;
    result.durationMs = finishedAtUtcMs - startedAtUtcMs;
    result.errorMessage = computationResult.errorMessage;
    result.inputSnapshot = inputSnapshot;
    result.aggregatedField = computationResult.aggregatedField;
    result.emitterResults = computationResult.emitterResults;
    result.summaryText = result.status == SimulationResultStatus::Succeeded
        ? QStringLiteral("仿真完成")
        : QStringLiteral("仿真未成功完成");
    return result;
}
```

- [ ] **Step 3: Implement `run()` to call `EMC_Engine` synchronously and return `SimulationTaskResult`**

```cpp
SimulationTaskResult simSchedulerCtx::run(...) {
    const qint64 startedAt = QDateTime::currentMSecsSinceEpoch();
    EMC_Engine engine(modelType, std::move(fleet), inputSnapshot);
    engine.do_PE_computing();
    const qint64 finishedAt = QDateTime::currentMSecsSinceEpoch();
    return assembleResult(
        inputSnapshot,
        formationSource,
        presetFormationId,
        modelType,
        startedAt,
        finishedAt,
        engine.computationResult());
}
```

- [ ] **Step 4: Ask the user to rebuild and run the scheduler tests**

Run:

```powershell
cmake --build d:\code\Electromagnetic_compatibility\build --config Debug --target SchemaDtoValidationTests
ctest --output-on-failure -R "simSchedulerCtx"
```

Expected:

- `SimulationSchedulerTests` passes

### Task 5: Update UI and Painting to Consume `SimulationTaskResult`

**Files:**
- Modify: `Resource/ui/Simulation.h`
- Modify: `Resource/ui/Simulation.cpp`
- Modify: `Utils/PaintImage.hpp`
- Test: manual verification in UI

- [ ] **Step 1: Update painting helper to accept `ScalarField2D`**

```cpp
inline void PEmodel_Painting2D(const ScalarField2D& field, QCustomPlot* plot) {
    if (field.rows <= 0 || field.cols <= 0 || field.values.empty()) {
        return;
    }

    QCPColorMap* colorMap = new QCPColorMap(plot->xAxis, plot->yAxis);
    colorMap->data()->setSize(field.rows, field.cols);
    colorMap->data()->setRange(
        QCPRange(field.originX, field.originX + field.rows * field.stepX),
        QCPRange(field.originY, field.originY + field.cols * field.stepY));
    ...
}
```

- [ ] **Step 2: Replace `_emcEngine`-centric UI state with task result consumption**

```cpp
std::unique_ptr<simSchedulerCtx> _scheduler;
std::optional<SimulationTaskResult> _lastSuccessfulResult;
std::optional<SimulationTaskResult> _lastFinishedResult;
```

```cpp
if (taskResult.status == SimulationResultStatus::Succeeded) {
    PEmodel_Painting2D(taskResult.aggregatedField, _plot);
    _lastSuccessfulResult = taskResult;
}
```

- [ ] **Step 3: Preserve stale-result warning by comparing against `SimulationTaskResult::inputSnapshot`**

```cpp
bool Simulation::hasStaleSuccessfulResult() const {
    if (!_lastSuccessfulResult.has_value()) {
        return false;
    }
    return _hasDirtyInputs ||
           !(DataModel::instance()->createSnapshot() == _lastSuccessfulResult->inputSnapshot);
}
```

- [ ] **Step 4: Ask the user to manually verify the simulation page**

Manual verification:

- 启动一次成功仿真，确认主图正常绘制
- 修改环境/设备/船只任一输入，不保存，确认状态文案仍提示“当前结果对应旧输入”
- 取消仿真，确认不会覆盖上一张成功结果

### Task 6: Add Result Schema Documents and Sync Project Docs

**Files:**
- Create: `docs/schema/simulation-result.schema.json`
- Create: `docs/schema/simulation-result.template.jsonc`
- Modify: `docs/项目概览.md`
- Modify: `docs/项目架构与API规范.md`
- Modify: `docs/风险与改进建议.md`

- [ ] **Step 1: Add result schema with fixed field names and enum values**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Simulation Result Schema",
  "type": "object",
  "required": [
    "resultSchemaVersion",
    "taskId",
    "modelType",
    "status",
    "formationSource",
    "startedAtUtcMs",
    "finishedAtUtcMs",
    "durationMs",
    "inputSnapshot",
    "derivedMetrics"
  ]
}
```

- [ ] **Step 2: Add template JSONC documenting `formationSource + presetFormationId`**

```jsonc
{
  "resultSchemaVersion": "1.0.0",
  "taskId": "sample-task",
  "modelType": "PE",
  "status": "Succeeded",
  "formationSource": "PresetFormation",
  "presetFormationId": 3,
  "startedAtUtcMs": 1714377600000,
  "finishedAtUtcMs": 1714377603200,
  "durationMs": 3200,
  "aggregatedField": {
    "fieldId": "agg-field",
    "quantity": "AggregatedPowerDbm",
    "valueUnit": "dBm",
    "rows": 2,
    "cols": 2,
    "values": [-42.0, -41.5, -40.8, -39.9]
  }
}
```

- [ ] **Step 3: Update overview, architecture, and risk docs to reflect the new result boundary**

```markdown
- 主程序仿真结果已收敛为 `SimulationTaskResult`
- `EMC_Engine` 只承担计算与聚合
- `simSchedulerCtx` 负责任务级结果组装
- UI 与报告后续统一从结果对象读取
```

### Task 7: Verification Handoff

**Files:**
- None

- [ ] **Step 1: Ask the user to rebuild the project and tests**

Run:

```powershell
cmake --build d:\code\Electromagnetic_compatibility\build --config Debug --target all
```

Expected:

- build succeeds

- [ ] **Step 2: Ask the user to run targeted tests**

Run:

```powershell
ctest --output-on-failure -R "result|scheduler|snapshot"
```

Expected:

- all targeted tests pass

- [ ] **Step 3: Ask the user to run the full suite**

Run:

```powershell
ctest --output-on-failure
```

Expected:

- full suite passes

- [ ] **Step 4: Ask the user to manually validate the simulation page and result stale-state behavior**

Manual verification:

- 成功仿真后主图正确显示
- 取消仿真后旧结果仍保留
- 修改输入后状态提示结果已过期
- 预设编队字段当前仅保留在结果对象中，不影响现有手工输入链路
