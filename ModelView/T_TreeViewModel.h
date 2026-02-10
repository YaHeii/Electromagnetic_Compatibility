#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include "Interface/DataModel.h"

class T_TreeItem;
class T_TreeViewModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit T_TreeViewModel(QObject* parent = nullptr);
    ~T_TreeViewModel();
    QModelIndex parent(const QModelIndex& child) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int getItemCount() const;

    // 自定义方法
    void clear();
    void addShip(const ShipData& ship);
    void addDevice(const EquipmentData& device);
    void removeItem(const QString& itemId);
    QModelIndex findItemIndex(const QString& itemId) const;
    T_TreeItem* getItemFromIndex(const QModelIndex& index) const;


    T_TreeItem* findItemById(T_TreeItem* item, const QString& itemId) const;

    T_TreeItem* _rootItem;

private:
    QMap<QString, T_TreeItem*> _itemsMap;
    T_TreeItem* _rootItem{nullptr};
};
