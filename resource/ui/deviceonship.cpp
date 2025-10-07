//
// Created by lenovo on 25-10-6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_DeviceonShip.h" resolved

#include "deviceonship.h"
#include "ui_DeviceonShip.h"


DeviceonShip::DeviceonShip(QWidget *parent) :
    QWidget(parent), ui(new Ui::DeviceonShip) {
    ui->setupUi(this);
}

DeviceonShip::~DeviceonShip() {
    delete ui;
}
//
// Ui::DeviceonShip *DeviceonShip::getUI() {
//     return ui;
// }