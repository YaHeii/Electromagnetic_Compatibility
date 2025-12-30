#include "deviceonship.h"
#include "ui_DeviceonShip.h"


DeviceonShip::DeviceonShip(QWidget *parent) :
    QWidget(parent), ui(new Ui::DeviceonShip) {
    ui->setupUi(this);
}

DeviceonShip::~DeviceonShip() {
    delete ui;
}

void DeviceonShip::on_deleteDeviceonShip_clicked() {
    emit removalRequested();
}
