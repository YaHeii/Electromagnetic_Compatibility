#ifndef T_TREEITEM_H
#define T_TREEITEM_H

#include <QModelIndex>
#include <QObject>

#include "ElaProperty.h"
class T_TreeItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY_CREATE(QList<T_TreeItem*>, ChildrenItems)
    Q_PROPERTY_CREATE(bool, IsChecked)
    Q_PRIVATE_CREATE(T_TreeItem*, ParentItem)
public:
    enum ItemType {
        Root,
        Fleet,
        Ship,
        Tranmitter,
        Receiver,
        Transceiver
    };
public:
    explicit T_TreeItem(QString itemTitle, ItemType itemType, T_TreeItem* parent = nullptr);
    ~T_TreeItem();
    QString getItemKey() const;
    QString getItemTitle() const;

    void setChildChecked(bool isChecked);
    Qt::CheckState getChildCheckState();
    ItemType getItemType() const { return _pItemType; }
    void appendChildItem(T_TreeItem* childItem);
    bool getIsHasChild() const;
    int getRow() const;

private:
    ItemType _pItemType;
    QString _itemKey = "";
    QString _itemTitle = "";
    bool _isExpanded{false};
};

#endif // T_TREEITEM_H
