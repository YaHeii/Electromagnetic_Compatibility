#ifndef TREEVIEWMANAGER_H
#define TREEVIEWMANAGER_H

#include <QObject>
#include <QStandardItemModel>
#include "../include/models/DataModel.h"


// 前置声明
class QTreeView;
class QStandardItem;

class TreeViewManager : public QObject
{
    Q_OBJECT
public:
    explicit TreeViewManager(QTreeView *treeView, QObject *parent = nullptr);
    void syncViewWithModel();

private:
    void populateShips(QStandardItem *rootItem);
    void populateDevices(QStandardItem *rootItem);

    QTreeView *m_treeView;
    QStandardItemModel *m_model;
};

#endif //TREEVIEWMANAGER_H
