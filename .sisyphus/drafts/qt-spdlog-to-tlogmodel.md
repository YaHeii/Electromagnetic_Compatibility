# Draft: QtSpdlogSink → T_LogModel 设计讨论

## 背景
- 现有自定义 sink：`Utils/QtSpdlogSink.h`
  - `QtTextEditSink<Mutex>` 在 `sink_it_()` 中格式化 `spdlog::details::log_msg`，并通过 `LogEmitter::newLog(QString,int)` 发射。
  - 目前用 `int level` 跨线程传递（避免注册自定义 metatype）。
- 现有 UI 模型：`ModelView/T_LogModel.h`
  - `QAbstractListModel`，内部维护 `QList<LogEntry>`，`LogEntry` 含 `message / spdlog::level / color`。
  - 暴露 append/clear/filter 等接口。

## 目标（用户请求）
- 使用 `T_LogModel` 接收 `QtSpdlogSink` 发出的日志，并在 UI 中展示。

## 初步设计方向（待确认）
- 建议建立“日志管线”：`spdlog → Qt sink → LogEmitter(signal) → T_LogModel(slot)`。
- 关键约束：
  - **线程安全**：spdlog 可能在任意线程产生日志；Model 必须只在其线程（通常 GUI 线程）更新。
  - **生命周期**：sink 持有 `emitter_` 裸指针，必须避免 emitter 释放后仍被调用。
  - **性能**：高频日志时避免每条都刷新 UI；可能需要批量/限流/上限。

## 需要用户确认的问题
1. UI 技术栈：Qt Widgets（QListView/QTableView）还是 QML（ListView）？
2. 日志量级：每秒大概多少条？是否需要“最多保留 N 条 / 滚动丢弃”？
3. 过滤方式：仅 UI 过滤（不影响 spdlog 输出）还是需要从源头按 level 控制？
4. 日志格式：是否已在 spdlog formatter 中统一（时间/线程/文件行号），还是希望 Model 侧拆字段？
5. 期望展示字段：仅 message + level + color，还是还要 timestamp / logger name / thread id？

## 已确认偏好（来自用户回复）
- UI：**Qt Widgets**
- 管线：**队列 + 定时批量刷新（推荐方案）**
- 需要 UI 保留上限：**需要（推荐）**

## 已知风险点（后续要在计划里显式写 guardrails）
- 直接从非 GUI 线程改动 `_logEntries` 或调用 `beginInsertRows/endInsertRows` 会导致崩溃/未定义行为。
- `LogEmitter*` 裸指针跨线程使用的悬空风险（必须有 detach/析构顺序保证）。

## 仓库内已出现的相关线索（待进一步核实）
- `Resource/ui/LogWidget.cpp` 内已存在 `_logEmitter = new LogEmitter(this);` 并用 `Qt::QueuedConnection` 连接到 `onLogReceived`，且标注 TODO "处理spdlog输出到UI"。
- `ModelView/T_LogModel.cpp` 已实现按 `spdlog::level` 着色与 **最多 1000 条** 的丢弃策略（删除最旧）。
