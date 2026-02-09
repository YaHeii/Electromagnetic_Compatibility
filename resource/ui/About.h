#pragma once

#include <ElaDialog.h>

class About : public ElaDialog
{
    Q_OBJECT
public:
    explicit About(QWidget* parent = nullptr);
    ~About();
};


