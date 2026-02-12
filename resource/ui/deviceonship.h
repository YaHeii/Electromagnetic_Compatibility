#pragma once
#include <QWidget>

// XXX:考虑从elawidgettool中继承
class DeviceonShip : public QWidget {
Q_OBJECT

public:
    explicit DeviceonShip(QWidget *parent = nullptr);
    ~DeviceonShip() override;
signals:
    void removalRequested();

private slots:
    void on_deleteDeviceonShip_clicked();
};
