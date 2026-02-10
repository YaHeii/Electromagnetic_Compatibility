#ifndef T_HOME_H
#define T_HOME_H

#include "BasePage.h"
class ElaMenu;
class Home : public BasePage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit Home(QWidget* parent = nullptr);
    ~Home();
Q_SIGNALS:
    Q_SIGNAL void elaScreenNavigation();
    Q_SIGNAL void elaBaseComponentNavigation();
    Q_SIGNAL void elaSceneNavigation();
    Q_SIGNAL void elaCardNavigation();
    Q_SIGNAL void elaIconNavigation();

protected:
    virtual void mouseReleaseEvent(QMouseEvent* event);

private:
    ElaMenu* _homeMenu{nullptr};
};

#endif // T_HOME_H
