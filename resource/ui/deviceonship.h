#pragma once
#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class DeviceonShip; }
QT_END_NAMESPACE

class DeviceonShip : public QWidget {
Q_OBJECT

public:
    explicit DeviceonShip(QWidget *parent = nullptr);
    ~DeviceonShip() override;

// Ui::DeviceonShip *getUI();
signals:
    void removalRequested();
private:
    Ui::DeviceonShip *ui;
private slots:
    void on_deleteDeviceonShip_clicked();
};
