#ifndef T_GRAPHICS_H
#define T_GRAPHICS_H

#include "BasePage.h"

class T_Graphics : public BasePage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit T_Graphics(QWidget* parent = nullptr);
    ~T_Graphics();
};

#endif // T_GRAPHICS_H
