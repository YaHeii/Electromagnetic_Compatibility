#include "DeviceWidget.h"

#include <QComboBox>
#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QLineEdit>
#include <QVariant>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaMessageBar.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "Interface/SchemaConstants.h"
#include "spdlog/spdlog.h"

namespace {

QString comboValue(QComboBox* comboBox) {
    const QVariant data = comboBox->currentData();
    return data.isValid() ? data.toString() : comboBox->currentText();
}

void setComboValue(QComboBox* comboBox, const QString& value) {
    const int dataIndex = comboBox->findData(value);
    if (dataIndex >= 0) {
        comboBox->setCurrentIndex(dataIndex);
        return;
    }

    const int textIndex = comboBox->findText(value);
    if (textIndex >= 0) {
        comboBox->setCurrentIndex(textIndex);
    }
}

void addComboItem(QComboBox* comboBox, const QString& label, const QString& value) {
    comboBox->addItem(label, value);
}

bool readRequiredText(const QLineEdit* lineEdit, const QString& fieldName, QString& value, QString& errorMessage) {
    value = lineEdit->text().trimmed();
    if (value.isEmpty()) {
        errorMessage = QStringLiteral("%1 不能为空").arg(fieldName);
        return false;
    }
    return true;
}

bool readRequiredNumber(const QLineEdit* lineEdit, const QString& fieldName, double& value, QString& errorMessage) {
    bool ok = false;
    value = lineEdit->text().trimmed().toDouble(&ok);
    if (!ok) {
        errorMessage = QStringLiteral("%1 必须是数字").arg(fieldName);
        return false;
    }
    return true;
}

bool isSupportedEquipmentType(const QString& type) {
    return type == QString::fromLatin1(SchemaValues::Transmitter) ||
           type == QString::fromLatin1(SchemaValues::Receiver) ||
           type == QString::fromLatin1(SchemaValues::Transceiver);
}

bool typeSupportsTransmitterFields(const QString& type) {
    return type == QString::fromLatin1(SchemaValues::Transmitter) ||
           type == QString::fromLatin1(SchemaValues::Transceiver);
}

bool typeSupportsReceiverFields(const QString& type) {
    return type == QString::fromLatin1(SchemaValues::Receiver) ||
           type == QString::fromLatin1(SchemaValues::Transceiver);
}

void clearLayoutWidgets(QLayout* layout) {
    while (QLayoutItem* child = layout->takeAt(0)) {
        if (QWidget* widget = child->widget()) {
            delete widget;
        }
        delete child;
    }
}

}  // namespace

DeviceWidget::DeviceWidget(QWidget* parent)
    : BasePage(parent) {
    createCustomWidget("此页用于维护可挂载设备库，保存时会统一校验完整快照");

    AddDeviceBtn = new ElaPushButton("添加新设备", this);
    AddDeviceBtn->setFixedSize(120, 36);
    SaveEquipmentBtn = new ElaPushButton("保存所有设备", this);
    SaveEquipmentBtn->setFixedSize(120, 36);

    _btnLayout = new QHBoxLayout();
    _btnLayout->addStretch();
    _btnLayout->addWidget(AddDeviceBtn);
    _btnLayout->addSpacing(20);
    _btnLayout->addWidget(SaveEquipmentBtn);

    _deviceListLayout = new QVBoxLayout();
    _deviceListLayout->setContentsMargins(10, 10, 10, 10);
    _deviceListLayout->setSpacing(20);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    centralWidget->setWindowTitle("设备属性管理");
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(_deviceListLayout, 10);
    mainLayout->addStretch();
    mainLayout->addLayout(_btnLayout, 1);
    addCentralWidget(centralWidget);

    connect(AddDeviceBtn, &ElaPushButton::clicked, this, &DeviceWidget::on_AddDeviceBtn_clicked);
    connect(SaveEquipmentBtn, &ElaPushButton::clicked, this, &DeviceWidget::on_SaveEquipmentBtn_clicked);

    loadFromModel();
}

DeviceWidget::~DeviceWidget() = default;

void DeviceWidget::loadFromModel() {
    _isLoading = true;
    clearItems();

    const auto& equipments = DataModel::instance()->allEquipments;
    if (equipments.empty()) {
        on_AddDeviceBtn_clicked();
    } else {
        for (const auto& equipment : equipments) {
            auto* newItem = new DeviceItemWidget(this);
            newItem->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            newItem->setData(equipment);
            newItem->setReadOnly(_isReadOnly);
            _deviceListLayout->addWidget(newItem);
            connect(newItem, &DeviceItemWidget::deleteMe, this, &DeviceWidget::on_RemoveItemRequested);
            connect(newItem, &DeviceItemWidget::dataEdited, this, &DeviceWidget::on_ItemEdited);
        }
    }

    _isLoading = false;
    setDirty(false);
}

bool DeviceWidget::saveToModel(QString* errorMessage) {
    auto* model = DataModel::instance();
    std::vector<EquipmentData> equipments;

    for (int i = 0; i < _deviceListLayout->count(); ++i) {
        QLayoutItem* layoutItem = _deviceListLayout->itemAt(i);
        if (auto* widget = qobject_cast<DeviceItemWidget*>(layoutItem->widget())) {
            EquipmentData data;
            QString localError;
            if (!widget->tryBuildData(data, localError)) {
                if (errorMessage) {
                    *errorMessage = localError;
                }
                return false;
            }
            equipments.push_back(std::move(data));
        }
    }

    auto snapshot = model->createSnapshot();
    snapshot.allEquipments = equipments;

    const auto validationResult = DataModel::validateSnapshot(snapshot);
    if (!validationResult.first) {
        if (errorMessage) {
            *errorMessage = validationResult.second;
        }
        return false;
    }

    model->allEquipments = std::move(snapshot.allEquipments);
    setDirty(false);
    emit modelCommitted();
    emit equipmentsCommitted();
    return true;
}

void DeviceWidget::setReadOnly(bool readOnly) {
    _isReadOnly = readOnly;
    AddDeviceBtn->setEnabled(!readOnly);
    SaveEquipmentBtn->setEnabled(!readOnly);
    for (int i = 0; i < _deviceListLayout->count(); ++i) {
        if (auto* widget = qobject_cast<DeviceItemWidget*>(_deviceListLayout->itemAt(i)->widget())) {
            widget->setReadOnly(readOnly);
        }
    }
}

void DeviceWidget::on_AddDeviceBtn_clicked() {
    if (_isReadOnly && !_isLoading) {
        return;
    }

    auto* newItem = new DeviceItemWidget(this);
    newItem->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    newItem->setReadOnly(_isReadOnly);
    _deviceListLayout->addWidget(newItem);

    connect(newItem, &DeviceItemWidget::deleteMe, this, &DeviceWidget::on_RemoveItemRequested);
    connect(newItem, &DeviceItemWidget::dataEdited, this, &DeviceWidget::on_ItemEdited);
    spdlog::info("已添加新的设备 UI 条目");
    if (!_isLoading) {
        setDirty(true);
    }
}

void DeviceWidget::on_RemoveItemRequested(DeviceItemWidget* item) {
    if (!item) {
        return;
    }

    _deviceListLayout->removeWidget(item);
    item->deleteLater();
    spdlog::info("已移除设备 UI 条目");
    if (!_isLoading) {
        setDirty(true);
    }
}

void DeviceWidget::on_SaveEquipmentBtn_clicked() {
    QString errorMessage;
    if (!saveToModel(&errorMessage)) {
        spdlog::error("设备保存失败: {}", errorMessage.toStdString());
        ElaMessageBar::error(ElaMessageBarType::BottomRight, QStringLiteral("保存失败"), errorMessage, 2000, this);
        return;
    }

    spdlog::info("设备保存成功，当前 DataModel 中共有 {} 个设备", DataModel::instance()->allEquipments.size());
    ElaMessageBar::success(ElaMessageBarType::BottomRight, QStringLiteral("保存成功"), QStringLiteral("设备库已同步到当前模型"), 1500, this);
}

void DeviceWidget::on_ItemEdited() {
    if (_isLoading || _isReadOnly) {
        return;
    }
    setDirty(true);
}

void DeviceWidget::setDirty(bool dirty) {
    if (_isDirty == dirty) {
        return;
    }
    _isDirty = dirty;
    emit dirtyStateChanged(_isDirty);
}

void DeviceWidget::clearItems() {
    clearLayoutWidgets(_deviceListLayout);
}

DeviceItemWidget::DeviceItemWidget(QWidget* parent)
    : QWidget(parent) {
    ElaText* typeText = new ElaText("设备类型", this);
    typeText->setTextPixelSize(15);
    _equipmentType = new ElaComboBox(this);
    addComboItem(_equipmentType, QStringLiteral("发射机"), QString::fromLatin1(SchemaValues::Transmitter));
    addComboItem(_equipmentType, QStringLiteral("接收机"), QString::fromLatin1(SchemaValues::Receiver));
    addComboItem(_equipmentType, QStringLiteral("收发一体机"), QString::fromLatin1(SchemaValues::Transceiver));

    connect(_equipmentType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DeviceItemWidget::onEquipmentTypeChanged);

    ElaText* gainText = new ElaText("设备增益", this);
    gainText->setTextPixelSize(15);
    _gain = new ElaLineEdit(this);
    _gain->setPlaceholderText("dBi");

    ElaText* idText = new ElaText("设备 ID", this);
    idText->setTextPixelSize(15);
    _equipmentID = new ElaLineEdit(this);
    _equipmentID->setPlaceholderText("建议使用可追踪的唯一名称");

    QHBoxLayout* firstLine = new QHBoxLayout();
    firstLine->addWidget(typeText);
    firstLine->addSpacing(10);
    firstLine->addWidget(_equipmentType);
    firstLine->addWidget(gainText);
    firstLine->addSpacing(10);
    firstLine->addWidget(_gain);
    firstLine->addSpacing(10);
    firstLine->addWidget(idText);
    firstLine->addSpacing(10);
    firstLine->addWidget(_equipmentID);

    ElaText* xText = new ElaText("X 坐标", this);
    xText->setTextPixelSize(15);
    _X_offset = new ElaLineEdit(this);
    _X_offset->setPlaceholderText("X 坐标");

    ElaText* yText = new ElaText("Y 坐标", this);
    yText->setTextPixelSize(15);
    _Y_offset = new ElaLineEdit(this);
    _Y_offset->setPlaceholderText("Y 坐标");

    ElaText* zText = new ElaText("Z 坐标", this);
    zText->setTextPixelSize(15);
    _Z_offset = new ElaLineEdit(this);
    _Z_offset->setPlaceholderText("Z 坐标");

    QHBoxLayout* secondLine = new QHBoxLayout();
    secondLine->addWidget(xText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_X_offset);
    secondLine->addSpacing(15);
    secondLine->addWidget(yText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_Y_offset);
    secondLine->addSpacing(15);
    secondLine->addWidget(zText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_Z_offset);

    _RecieverWidget = new QWidget(this);
    _TransmitterWidget = new QWidget(this);
    setupReceiverUI(_RecieverWidget);
    setupTransmitterUI(_TransmitterWidget);

    ReductionEquipmentBtn = new ElaPushButton("删除此设备", this);
    connect(ReductionEquipmentBtn, &ElaPushButton::clicked, this, &DeviceItemWidget::on_ReductionBtn_clicked);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addLayout(firstLine);
    mainLayout->addLayout(secondLine);
    mainLayout->addWidget(_TransmitterWidget);
    mainLayout->addWidget(_RecieverWidget);
    mainLayout->addWidget(ReductionEquipmentBtn);

    const auto lineEdits = findChildren<QLineEdit*>();
    for (QLineEdit* lineEdit : lineEdits) {
        connect(lineEdit, &QLineEdit::textChanged, this, [this]() { emit dataEdited(); });
    }
    const auto comboBoxes = findChildren<QComboBox*>();
    for (QComboBox* comboBox : comboBoxes) {
        connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { emit dataEdited(); });
    }

    onEquipmentTypeChanged();
}

EquipmentData DeviceItemWidget::getData() const {
    EquipmentData data;
    QString errorMessage;
    if (!tryBuildData(data, errorMessage)) {
        spdlog::warn("设备 DTO 组装失败: {}", errorMessage.toStdString());
    }
    return data;
}

bool DeviceItemWidget::tryBuildData(EquipmentData& data, QString& errorMessage) const {
    data = EquipmentData{};

    if (!readRequiredText(_equipmentID, QStringLiteral("设备 ID"), data.equipmentId, errorMessage)) {
        return false;
    }

    data.equipmentType = comboValue(_equipmentType);
    if (!isSupportedEquipmentType(data.equipmentType)) {
        errorMessage = QStringLiteral("设备类型不是受支持的 schema 枚举值");
        return false;
    }

    if (!readRequiredNumber(_gain, QStringLiteral("设备增益"), data.gainDbi, errorMessage) ||
        !readRequiredNumber(_X_offset, QStringLiteral("X 坐标"), data.offsetX, errorMessage) ||
        !readRequiredNumber(_Y_offset, QStringLiteral("Y 坐标"), data.offsetY, errorMessage) ||
        !readRequiredNumber(_Z_offset, QStringLiteral("Z 坐标"), data.offsetZ, errorMessage)) {
        return false;
    }

    if (typeSupportsTransmitterFields(data.equipmentType)) {
        if (!readRequiredNumber(_CentralF_Transmitter, QStringLiteral("发射中心频率"), data.transmitterCenterFrequencyGHz, errorMessage) ||
            !readRequiredNumber(_Bandwidth_Transmitter, QStringLiteral("发射带宽"), data.transmitterBandwidthMHz, errorMessage) ||
            !readRequiredNumber(_Power_Transmitter, QStringLiteral("发射功率"), data.transmitterPowerDbm, errorMessage) ||
            !readRequiredNumber(_antennaPhi_Transmitter, QStringLiteral("天线下倾角"), data.transmitterAntennaPhiDeg, errorMessage) ||
            !readRequiredNumber(_Beamwidth_Transmitter, QStringLiteral("波束宽度"), data.transmitterBeamWidthDeg, errorMessage)) {
            return false;
        }

        data.transmitterPolarization = comboValue(_PolarizationMethod_Transmitter);
        data.transmitterAntennaType = comboValue(_antennaType_Transmitter);
        if (data.transmitterPolarization.isEmpty() || data.transmitterAntennaType.isEmpty()) {
            errorMessage = QStringLiteral("发射机枚举选择不能为空");
            return false;
        }
    }

    if (typeSupportsReceiverFields(data.equipmentType)) {
        if (!readRequiredNumber(_CentralF_Receiver, QStringLiteral("接收中心频率"), data.receiverCenterFrequencyGHz, errorMessage) ||
            !readRequiredNumber(_Bandwidth_Receiver, QStringLiteral("接收带宽"), data.receiverBandwidthMHz, errorMessage) ||
            !readRequiredNumber(_Sensitive_Receiver, QStringLiteral("接收灵敏度"), data.receiverSensitivityDbm, errorMessage) ||
            !readRequiredNumber(_InterferenceMargin_Receiver, QStringLiteral("干扰门限"), data.receiverInterferenceMarginDb, errorMessage) ||
            !readRequiredNumber(_SINRMargin_Receiver, QStringLiteral("SINR 裕量"), data.receiverSinrMarginDb, errorMessage) ||
            !readRequiredNumber(_NoiseFigure_Receiver, QStringLiteral("噪声系数"), data.receiverNoiseFigureDb, errorMessage)) {
            return false;
        }
    }

    return true;
}

void DeviceItemWidget::setData(const EquipmentData& data) {
    _equipmentID->setText(data.equipmentId);
    setComboValue(_equipmentType, data.equipmentType);
    onEquipmentTypeChanged();

    _gain->setText(QString::number(data.gainDbi));
    _X_offset->setText(QString::number(data.offsetX));
    _Y_offset->setText(QString::number(data.offsetY));
    _Z_offset->setText(QString::number(data.offsetZ));

    if (typeSupportsReceiverFields(data.equipmentType)) {
        _CentralF_Receiver->setText(QString::number(data.receiverCenterFrequencyGHz));
        _Bandwidth_Receiver->setText(QString::number(data.receiverBandwidthMHz));
        _Sensitive_Receiver->setText(QString::number(data.receiverSensitivityDbm));
        _InterferenceMargin_Receiver->setText(QString::number(data.receiverInterferenceMarginDb));
        _SINRMargin_Receiver->setText(QString::number(data.receiverSinrMarginDb));
        _NoiseFigure_Receiver->setText(QString::number(data.receiverNoiseFigureDb));
    } else {
        resetReceiverUI();
    }

    if (typeSupportsTransmitterFields(data.equipmentType)) {
        _CentralF_Transmitter->setText(QString::number(data.transmitterCenterFrequencyGHz));
        _Bandwidth_Transmitter->setText(QString::number(data.transmitterBandwidthMHz));
        _Power_Transmitter->setText(QString::number(data.transmitterPowerDbm));
        _antennaPhi_Transmitter->setText(QString::number(data.transmitterAntennaPhiDeg));
        _Beamwidth_Transmitter->setText(QString::number(data.transmitterBeamWidthDeg));
        setComboValue(_PolarizationMethod_Transmitter, data.transmitterPolarization);
        setComboValue(_antennaType_Transmitter, data.transmitterAntennaType);
    } else {
        resetTransmitterUI();
    }
}

void DeviceItemWidget::onEquipmentTypeChanged() {
    const QString type = comboValue(_equipmentType);
    const bool isTrans = typeSupportsTransmitterFields(type);
    const bool isRecv = typeSupportsReceiverFields(type);

    _TransmitterWidget->setVisible(isTrans);
    _RecieverWidget->setVisible(isRecv);
}

void DeviceItemWidget::on_ReductionBtn_clicked() {
    emit deleteMe(this);
}

void DeviceItemWidget::setReadOnly(bool readOnly) {
    const auto lineEdits = findChildren<QLineEdit*>();
    for (QLineEdit* lineEdit : lineEdits) {
        lineEdit->setReadOnly(readOnly);
    }
    const auto comboBoxes = findChildren<QComboBox*>();
    for (QComboBox* comboBox : comboBoxes) {
        comboBox->setEnabled(!readOnly);
    }
    ReductionEquipmentBtn->setEnabled(!readOnly);
}

void DeviceItemWidget::resetTransmitterUI() {
    _CentralF_Transmitter->setText("0");
    _Bandwidth_Transmitter->setText("0");
    _Power_Transmitter->setText("0");
    _antennaPhi_Transmitter->setText("0");
    _Beamwidth_Transmitter->setText("0");
    _PolarizationMethod_Transmitter->setCurrentIndex(0);
    _antennaType_Transmitter->setCurrentIndex(0);
}

void DeviceItemWidget::resetReceiverUI() {
    _CentralF_Receiver->setText("0");
    _Bandwidth_Receiver->setText("0");
    _Sensitive_Receiver->setText("-75");
    _InterferenceMargin_Receiver->setText("0");
    _SINRMargin_Receiver->setText("0");
    _NoiseFigure_Receiver->setText("3");
}

void DeviceItemWidget::setupReceiverUI(QWidget* container) {
    ElaText* centralFText = new ElaText("中心频率", this);
    centralFText->setTextPixelSize(13);
    _CentralF_Receiver = new ElaLineEdit(this);
    _CentralF_Receiver->setPlaceholderText("GHz");

    ElaText* bandwidthText = new ElaText("接收带宽", this);
    bandwidthText->setTextPixelSize(13);
    _Bandwidth_Receiver = new ElaLineEdit(this);
    _Bandwidth_Receiver->setPlaceholderText("MHz");

    QHBoxLayout* firstLine = new QHBoxLayout();
    firstLine->addWidget(centralFText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_CentralF_Receiver);
    firstLine->addSpacing(15);
    firstLine->addWidget(bandwidthText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_Bandwidth_Receiver);

    ElaText* sensitiveText = new ElaText("接收机灵敏度", this);
    sensitiveText->setTextPixelSize(13);
    _Sensitive_Receiver = new ElaLineEdit(this);
    _Sensitive_Receiver->setPlaceholderText("dBm");

    ElaText* interferenceText = new ElaText("干扰门限", this);
    interferenceText->setTextPixelSize(13);
    _InterferenceMargin_Receiver = new ElaLineEdit(this);
    _InterferenceMargin_Receiver->setPlaceholderText("dB");

    QHBoxLayout* secondLine = new QHBoxLayout();
    secondLine->addWidget(sensitiveText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_Sensitive_Receiver);
    secondLine->addSpacing(15);
    secondLine->addWidget(interferenceText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_InterferenceMargin_Receiver);

    ElaText* sinrText = new ElaText("SINR 裕量", this);
    sinrText->setTextPixelSize(13);
    _SINRMargin_Receiver = new ElaLineEdit(this);
    _SINRMargin_Receiver->setPlaceholderText("dB");

    ElaText* noiseText = new ElaText("噪声系数", this);
    noiseText->setTextPixelSize(13);
    _NoiseFigure_Receiver = new ElaLineEdit(this);
    _NoiseFigure_Receiver->setPlaceholderText("dB");

    QHBoxLayout* thirdLine = new QHBoxLayout();
    thirdLine->addWidget(sinrText);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(_SINRMargin_Receiver);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(noiseText);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(_NoiseFigure_Receiver);

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("设备参数");
    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->addLayout(firstLine);
    centerVLayout->addSpacing(10);
    centerVLayout->addLayout(secondLine);
    centerVLayout->addSpacing(10);
    centerVLayout->addLayout(thirdLine);
    centerVLayout->addStretch();

    if (!container->layout()) {
        auto* v = new QVBoxLayout(container);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);
    }
    container->layout()->addWidget(centralWidget);
}

void DeviceItemWidget::setupTransmitterUI(QWidget* container) {
    ElaText* centralFText = new ElaText("中心频率", this);
    centralFText->setTextPixelSize(15);
    _CentralF_Transmitter = new ElaLineEdit(this);
    _CentralF_Transmitter->setPlaceholderText("GHz");

    ElaText* bandwidthText = new ElaText("发射带宽", this);
    bandwidthText->setTextPixelSize(15);
    _Bandwidth_Transmitter = new ElaLineEdit(this);
    _Bandwidth_Transmitter->setPlaceholderText("MHz");

    ElaText* powerText = new ElaText("发射功率", this);
    powerText->setTextPixelSize(15);
    _Power_Transmitter = new ElaLineEdit(this);
    _Power_Transmitter->setPlaceholderText("dBm");

    QHBoxLayout* firstLine = new QHBoxLayout();
    firstLine->addWidget(centralFText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_CentralF_Transmitter);
    firstLine->addSpacing(15);
    firstLine->addWidget(bandwidthText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_Bandwidth_Transmitter);
    firstLine->addSpacing(15);
    firstLine->addWidget(powerText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_Power_Transmitter);

    ElaText* phiText = new ElaText("天线下倾角", this);
    phiText->setTextPixelSize(13);
    _antennaPhi_Transmitter = new ElaLineEdit(this);
    _antennaPhi_Transmitter->setPlaceholderText("deg");

    ElaText* beamwidthText = new ElaText("波束宽度", this);
    beamwidthText->setTextPixelSize(13);
    _Beamwidth_Transmitter = new ElaLineEdit(this);
    _Beamwidth_Transmitter->setPlaceholderText("deg");

    QHBoxLayout* secondLine = new QHBoxLayout();
    secondLine->addWidget(phiText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_antennaPhi_Transmitter);
    secondLine->addSpacing(15);
    secondLine->addWidget(beamwidthText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_Beamwidth_Transmitter);

    ElaText* polarizationText = new ElaText("极化方式", this);
    polarizationText->setTextPixelSize(13);
    _PolarizationMethod_Transmitter = new ElaComboBox(this);
    addComboItem(_PolarizationMethod_Transmitter, QStringLiteral("垂直极化"), QString::fromLatin1(SchemaValues::Vertical));
    addComboItem(_PolarizationMethod_Transmitter, QStringLiteral("水平极化"), QString::fromLatin1(SchemaValues::Horizontal));

    ElaText* antennaTypeText = new ElaText("天线类型", this);
    antennaTypeText->setTextPixelSize(13);
    _antennaType_Transmitter = new ElaComboBox(this);
    addComboItem(_antennaType_Transmitter, QStringLiteral("全向天线"), QString::fromLatin1(SchemaValues::Omni));
    addComboItem(_antennaType_Transmitter, QStringLiteral("定向天线"), QString::fromLatin1(SchemaValues::Directional));
    addComboItem(_antennaType_Transmitter, QStringLiteral("喇叭天线"), QString::fromLatin1(SchemaValues::Horn));
    addComboItem(_antennaType_Transmitter, QStringLiteral("赋形波束天线"), QString::fromLatin1(SchemaValues::ShapedBeam));
    addComboItem(_antennaType_Transmitter, QStringLiteral("抛物面天线"), QString::fromLatin1(SchemaValues::Reflector));

    QHBoxLayout* thirdLine = new QHBoxLayout();
    thirdLine->addWidget(polarizationText);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(_PolarizationMethod_Transmitter);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(antennaTypeText);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(_antennaType_Transmitter);

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("设备参数");
    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->addLayout(firstLine);
    centerVLayout->addSpacing(10);
    centerVLayout->addLayout(secondLine);
    centerVLayout->addSpacing(10);
    centerVLayout->addLayout(thirdLine);
    centerVLayout->addStretch();

    if (!container->layout()) {
        auto* v = new QVBoxLayout(container);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);
    }
    container->layout()->addWidget(centralWidget);
}
