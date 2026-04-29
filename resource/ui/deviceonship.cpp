#include "deviceonship.h"

#include <QHBoxLayout>

#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "spdlog/spdlog.h"

DeviceonShip::DeviceonShip(QWidget* parent)
    : QWidget(parent) {
    setWindowTitle("DeviceOnShip");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* equipmentNameText = new ElaText("设备名称", this);
    equipmentNameText->setTextPixelSize(15);
    equipmentNameText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    _EquipmentIDCombo = new ElaComboBox(this);
    _EquipmentIDCombo->installEventFilter(this);

    _deleteButton = new ElaPushButton("删除", this);
    _deleteButton->setFixedSize(60, 32);
    connect(_deleteButton, &ElaPushButton::clicked, this, &DeviceonShip::on_deleteDeviceonShip_clicked);

    layout->addWidget(equipmentNameText);
    layout->addSpacing(10);
    layout->addWidget(_EquipmentIDCombo);
    layout->addSpacing(10);
    layout->addWidget(_deleteButton);
    layout->addStretch();

    connect(_EquipmentIDCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        _data.equipmentId = _EquipmentIDCombo->currentText();
        emit dataEdited();
    });

    refreshEquipmentList();
}

DeviceonShip::~DeviceonShip() = default;

void DeviceonShip::on_deleteDeviceonShip_clicked() {
    emit removalRequested();
}

void DeviceonShip::refreshEquipmentList() {
    const QString currentSelection = !_data.equipmentId.trimmed().isEmpty() ? _data.equipmentId : _EquipmentIDCombo->currentText();

    _EquipmentIDCombo->blockSignals(true);
    _EquipmentIDCombo->clear();

    auto* model = DataModel::instance();
    for (const auto& eq : model->allEquipments) {
        _EquipmentIDCombo->addItem(eq.equipmentId);
    }

    if (!currentSelection.trimmed().isEmpty() && _EquipmentIDCombo->findText(currentSelection) < 0) {
        _EquipmentIDCombo->addItem(currentSelection);
    }

    const int index = _EquipmentIDCombo->findText(currentSelection);
    if (index >= 0) {
        _EquipmentIDCombo->setCurrentIndex(index);
    } else if (_EquipmentIDCombo->count() > 0) {
        _EquipmentIDCombo->setCurrentIndex(0);
    }

    _EquipmentIDCombo->blockSignals(false);
    _data.equipmentId = _EquipmentIDCombo->currentText();
}

EquipmentOnShip DeviceonShip::getData() const {
    EquipmentOnShip data = _data;
    data.equipmentId = _EquipmentIDCombo->currentText();
    return data;
}

void DeviceonShip::setData(const EquipmentOnShip& data) {
    _data = data;
    refreshEquipmentList();

    const int index = _EquipmentIDCombo->findText(data.equipmentId);
    if (index >= 0) {
        _EquipmentIDCombo->setCurrentIndex(index);
    } else {
        spdlog::warn("Missing equipment in current library: {}", data.equipmentId.toStdString());
    }
}

void DeviceonShip::setReadOnly(bool readOnly) {
    _EquipmentIDCombo->setEnabled(!readOnly);
    _deleteButton->setEnabled(!readOnly);
}

bool DeviceonShip::eventFilter(QObject* watched, QEvent* event) {
    if (watched == _EquipmentIDCombo && event->type() == QEvent::MouseButtonPress) {
        refreshEquipmentList();
        return false;
    }
    return QWidget::eventFilter(watched, event);
}
