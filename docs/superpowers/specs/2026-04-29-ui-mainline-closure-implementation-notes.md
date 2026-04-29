# UI 主链路闭环实施记录

日期：2026-04-29

## 当前已实现

- 主窗口工具栏新增 `JSON/JSONC` 导入入口，流程为 `QFileDialog -> JsonLoader::LoadFile() -> DataModel -> 页面回填`。
- `EnvironmentWidget` 已正式落地，支持：
  - 环境参数表单录入
  - 基础格式校验
  - `loadFromModel()`
  - `saveToModel()`
  - `setReadOnly(bool)`
  - dirty 状态管理
- `DeviceWidget`、`ShipWidget` 已改为可回填编辑页，保存时统一基于完整 `DataSnapshot` 调用 `DataModel::validateSnapshot()`。
- `DeviceonShip` 已修复重复/无效布局装配问题，并支持：
  - 页面回填后刷新
  - 设备保存后刷新
  - 下拉展开前懒刷新
  - 保留当前草稿选择，避免设备库刷新时静默改写船只草稿
- `Simulation` 页已改为显式状态机：
  - `Idle`
  - `Running`
  - `Cancelling`
  - `Succeeded`
  - `Failed`
  - `Cancelled`
- 仿真启动会冻结单次任务快照，`EMC_Engine` 只消费启动时传入的 `DataSnapshot`，不再二次读取全局 `DataModel`。
- 仿真运行中会禁用：
  - 环境页编辑
  - 设备页编辑
  - 船只页编辑
  - JSON 导入
  - 再次启动仿真
- 仿真失败或取消时不会覆盖上一次成功结果图；若当前输入已变化而结果来自旧快照，状态文案会明确提示“当前结果对应旧输入”。
- `LogWidget` 已修正：
  - 初始过滤状态与默认选项不一致
  - 打开日志文件路径与 `main.cpp` 的真实落盘路径不一致
- `Home` 已修复 `_homeMenu` 空指针右键路径，并接入 `TreeView` 只读总览。
- `TreeView` 已按 `DataModel` 提供只读树状展示，一级节点固定为 `环境参数`、`船只列表`、`设备库`，支持刷新、展开全部、折叠全部与关键字查找。
- `JsonToWidget.hpp` 已删除，不再保留空壳占位。

## 自动测试结果

- Catch2 测试代码已按职责拆分为：
  - `Tests/SchemaDtoValidationTests.cpp`：schema、`JsonLoader`、`DataModel` 与引擎快照校验
  - `Tests/UiMainlineTests.cpp`：`EnvironmentWidget`、`DeviceWidget`、`ShipWidget`、`DeviceonShip` 等 UI 主链路
  - `Tests/TreeViewModelTests.cpp`：`T_TreeViewModel` 的只读树构建与查找能力
- `CMakeLists.txt` 已接入上述测试源码，以及 `EnvironmentWidget`、`TreeView`、`T_TreeViewModel` 相关编译项。
- 按用户于 2026-04-29 回传的构建与 `ctest --output-on-failure` 结果：
  - 工程构建通过
  - 自动测试全部通过

## 手动验证结果

- 按用户回传，本轮规格中的手动验证已全部通过：
  - 空模型录入并保存
  - 导入 `Tests/Test.jsonc`
  - 修改设备后刷新船只挂载列表
  - 启动仿真、切页只读浏览、取消任务、再次启动
  - 检查日志过滤与日志文件打开路径
  - 首页 `TreeView` 的刷新、展开、折叠与关键字查找

## 文档同步说明

- 已同步更新 `docs/项目概览.md`、`docs/项目架构与API规范.md`、`docs/风险与改进建议.md`。
- 本轮文档统一反映以下结论：
  - UI 主链路闭环已落地并完成验证
  - `TreeView` 当前定位为只读总览，不进入编辑链路
  - 剩余工作重点转为结构化结果对象、任务体系深化与后续产品化能力
