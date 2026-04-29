#ifndef T_TREEVIEW_H
#define T_TREEVIEW_H

#include "BasePage.h"

class ElaLineEdit;
class ElaText;
class ElaTreeView;
class T_TreeViewModel;

class TreeView : public BasePage
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit TreeView(QWidget* parent = nullptr);
    ~TreeView() override;

    void syncViewWithModel();
    void expandAll();
    void collapseAll();
    bool findAndSelect(const QString& keyword);

private slots:
    void onSearchRequested();

private:
    void updateSummaryText();

    ElaTreeView* _treeView{nullptr};
    T_TreeViewModel* _treeModel{nullptr};
    ElaLineEdit* _searchEdit{nullptr};
    ElaText* _dataText{nullptr};
    ElaText* _searchResultText{nullptr};
};

#endif // T_TREEVIEW_H
