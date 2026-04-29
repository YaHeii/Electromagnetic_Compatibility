#pragma once

#include <QAbstractItemModel>

#include "Interface/DataModel.h"
#include "T_TreeItem.h"

class T_TreeViewModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit T_TreeViewModel(QObject* parent = nullptr);
    ~T_TreeViewModel() override;

    QModelIndex parent(const QModelIndex& child) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int getItemCount() const;
    void clear();
    void reloadFromDataModel();
    QModelIndex findItemIndex(const QString& keyword) const;
    T_TreeItem* getItemFromIndex(const QModelIndex& index) const;
    T_TreeItem* appendChild(T_TreeItem* parent, const QString& title, T_TreeItem::ItemType type);
private:
    void populateEnvironment(T_TreeItem* parent, const EnvironmentData& environment);
    void populateAnalysisConfig(T_TreeItem* parent, const EMCAnalysisConfig& analysisConfig);
    void populateShips(T_TreeItem* parent, const std::vector<ShipData>& ships);
    void populateEquipments(T_TreeItem* parent, const std::vector<EquipmentData>& equipments);
    int countNodes(const T_TreeItem* item) const;
    T_TreeItem* findFirstMatch(T_TreeItem* item, const QString& keyword) const;

    T_TreeItem* _rootItem{nullptr};
};
