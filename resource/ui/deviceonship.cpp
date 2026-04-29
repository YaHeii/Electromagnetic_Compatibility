#include "deviceonship.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <Qevent>

#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "spdlog/spdlog.h"

DeviceonShip::DeviceonShip(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("DeviceonShip");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    ElaText* EquipmentNameText = new ElaText("设备名称", this);
    EquipmentNameText->setTextPixelSize(15);
    EquipmentNameText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    _EquipmentIDCombo = new ElaComboBox(this);
    refreshEquipmentList();
    _EquipmentIDCombo->installEventFilter(this);

    ElaPushButton* deleteDeviceonShip = new ElaPushButton("删除", this);
    deleteDeviceonShip->setFixedSize(60, 32);
    connect(deleteDeviceonShip, &ElaPushButton::clicked, this, &DeviceonShip::on_deleteDeviceonShip_clicked);

    layout->addWidget(EquipmentNameText);
    layout->addSpacing(10);
    layout->addWidget(_EquipmentIDCombo);
    layout->addSpacing(10);
    layout->addWidget(deleteDeviceonShip);
    layout->addStretch();

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->addLayout(layout);
    centerVLayout->addStretch();
}

DeviceonShip::~DeviceonShip() = default;

void DeviceonShip::on_deleteDeviceonShip_clicked() {
    emit removalRequested();
}

void DeviceonShip::refreshEquipmentList() {
    const QString currentSelection = _EquipmentIDCombo->currentText();
    _EquipmentIDCombo->clear();

    auto* model = DataModel::instance();
    for (const auto& eq : model->allEquipments) {
        _EquipmentIDCombo->addItem(eq.equipmentId);
    }

    const int index = _EquipmentIDCombo->findText(currentSelection);
    if (index >= 0) {
        _EquipmentIDCombo->setCurrentIndex(index);
    } else if (_EquipmentIDCombo->count() > 0) {
        _EquipmentIDCombo->setCurrentIndex(0);
    }
}

EquipmentOnShip DeviceonShip::getData() const {
    EquipmentOnShip data = _data;
    data.equipmentId = _EquipmentIDCombo->currentText();
    return data;
}

void DeviceonShip::setData(const EquipmentOnShip& data) {
    _data = data;

    const int index = _EquipmentIDCombo->findText(data.equipmentId);
    if (index >= 0) {
        _EquipmentIDCombo->setCurrentIndex(index);
    } else {
        spdlog::warn("Missing equipment in current library: {}", data.equipmentId.toStdString());
    }
}

bool DeviceonShip::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _EquipmentIDCombo && event->type() == QEvent::MouseButtonPress) {
        refreshEquipmentList();
        return false;
    }
    return QWidget::eventFilter(watched, event);
}
