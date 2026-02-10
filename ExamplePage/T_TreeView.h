#ifndef T_TREEVIEW_H
#define T_TREEVIEW_H

#include "BasePage.h"

class ElaTreeView;
class T_TreeView : public BasePage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit T_TreeView(QWidget* parent = nullptr);
    ~T_TreeView();

    //void syncViewWithModel();
    //void expandAll();
    //void collapseAll();
    //// 获取选中项目的ID
    //QString getSelectedItemId() const;
    //// 根据ID查找项目
    //QModelIndex findItemIndex(const QString& itemId) const;

public slots:
    //void onItemClicked(const QModelIndex& index);
    //void setupTreeView();
    //void populateShips(FleetTreeItem* rootItem);
    //void populateDevices(FleetTreeItem* rootItem);


private:
    ElaTreeView* _treeView{nullptr};
};

#endif // T_TREEVIEW_H
