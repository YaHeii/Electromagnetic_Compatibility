#ifndef T_LISTVIEW_H
#define T_LISTVIEW_H

#include "Resource/ui/BasePage.h"

class ElaListView;
class T_ListView : public BasePage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit T_ListView(QWidget* parent = nullptr);
    ~T_ListView();

private:
    ElaListView* _listView{nullptr};
};

#endif // T_LISTVIEW_H
