//
// Created by lenovo on 25-10-6.
//

#ifndef DEVICEONSHIP_H
#define DEVICEONSHIP_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class DeviceonShip; }
QT_END_NAMESPACE

class DeviceonShip : public QWidget {
Q_OBJECT

public:
    explicit DeviceonShip(QWidget *parent = nullptr);
    ~DeviceonShip() override;
    void getData();
private:
    Ui::DeviceonShip *ui;
};


#endif //DEVICEONSHIP_H
