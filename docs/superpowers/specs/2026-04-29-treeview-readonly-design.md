# TreeView 只读数据总览设计

日期：2026-04-29

## 目标

在 `resource/ui/TreeView.*` 与 `ModelView/T_TreeViewModel.*` 中实现一个只读树状总览页，把当前 `DataModel` 内容稳定展示出来，并提供：

- 刷新
- 展开全部
- 折叠全部
- 关键字查找并定位首个匹配项

本轮不进入主编辑链路，不支持树上编辑，不支持树节点驱动页面跳转。

## 展示结构

树的一级节点固定为三类：

1. `环境参数`
2. `船只列表`
3. `设备库`

展示规则如下：

- `环境参数`
  - `maxRange`
  - `ductHeight`
  - `windSpeed`
  - `dx`
  - `dz`
  - `nz`
  - `angleStepDeg`
- `船只列表`
  - 每艘船一个节点，节点标题直接显示 `shipId`
  - 船节点下展示：
    - 位置
    - 速度
    - 朝向
    - `挂载设备`
  - `挂载设备` 下展示设备引用 ID
- `设备库`
  - 每个设备一个节点，节点标题直接显示 `equipmentId`
  - 设备节点下至少展示：
    - 类型
    - 增益
    - 相对坐标
  - 再按设备类型补充关键参数：
    - 发射相关字段
    - 接收相关字段

## 接口约定

### `T_TreeViewModel`

新增或收敛为只读树模型接口：

- `void reloadFromDataModel()`
- `QModelIndex findItemIndex(const QString& keyword) const`
- `int getItemCount() const`
- `void clear()`

行为约束：

- `reloadFromDataModel()` 每次从当前 `DataModel::instance()` 全量重建树
- `findItemIndex()` 按节点标题做大小写不敏感包含匹配，返回首个命中项
- `flags()` 仅返回只读选择能力，不保留勾选和编辑语义

### `TreeView`

补齐页面内部接口：

- `void syncViewWithModel()`
- `void expandAll()`
- `void collapseAll()`
- `bool findAndSelect(const QString& keyword)`

行为约束：

- 构造完成后立即首次加载
- 后续通过刷新按钮手动同步当前 `DataModel`
- 查找命中后展开祖先节点、滚动到目标并选中
- 查找失败时仅提示，不修改当前模型

## 测试范围

新增 TreeView 专项测试文件，覆盖：

1. `reloadFromDataModel()` 能把 `DataModel` 重建为三类一级节点
2. `findItemIndex()` 能命中：
   - 环境参数名
   - 船只 ID
   - 设备 ID
3. 关键字查找为大小写不敏感包含匹配

## 非目标

- 不修改 `DataModel` 语义
- 不给树节点增加编辑、勾选、删除能力
- 不做首页导航联动
- 不引入新的树控件框架
