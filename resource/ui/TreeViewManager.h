#pragma once
#include <QObject>
#include <QAbstractItemModel>
#include "Models/DataModel.h"
#include "ModelView/T_TreeItem.h"

// 前置声明
class QTreeView;
class QStandardItemModel;
class FleetTreeItem;
class FleetTreeViewModel;

class TreeViewManager : public QObject
{
    Q_OBJECT
public:
    explicit TreeViewManager(QTreeView *treeView, QObject *parent = nullptr);
    ~TreeViewManager();
    
    void syncViewWithModel();
    void expandAll();
    void collapseAll();
    
    // 获取选中项目的ID
    QString getSelectedItemId() const;
    // 根据ID查找项目
    QModelIndex findItemIndex(const QString& itemId) const;

public slots:
    void onItemClicked(const QModelIndex& index);
    void setupTreeView();
    void populateShips(FleetTreeItem *rootItem);
    void populateDevices(FleetTreeItem *rootItem);
    //void updateTreeStructure();
private:
    QTreeView *_treeView;
    FleetTreeViewModel *_model;
};

// 专用树项目类，继承自T_TreeItem
class FleetTreeItem : public T_TreeItem
{
    Q_OBJECT
public:
    enum ItemType {
        Root,
        Fleet,
        Ship,
        Device
    };

public:
    explicit FleetTreeItem(const QString& title, ItemType type, const QString& dataId = "", FleetTreeItem* parent = nullptr);
    
    ItemType getItemType() const { return _pItemType; }
    QString getDataId() const { return _pDataId; }
private:
    ItemType _pItemType;
    QString _pDataId;
    
};

// 专用树模型类，继承自QAbstractItemModel
class FleetTreeViewModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit FleetTreeViewModel(QObject* parent = nullptr);
    ~FleetTreeViewModel();
    
    // QAbstractItemModel interface
    QModelIndex parent(const QModelIndex& child) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    // 自定义方法
    void clear();
    void addShip(const ShipData& ship);
    void addDevice(const EquipmentData& device);
    void removeItem(const QString& itemId);
    QModelIndex findItemIndex(const QString& itemId) const;
    FleetTreeItem* getItemFromIndex(const QModelIndex& index) const;
    

    FleetTreeItem* findItemById(FleetTreeItem* item, const QString& itemId) const;
    
    FleetTreeItem* _rootItem;
};
