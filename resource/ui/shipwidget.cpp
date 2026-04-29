#include "shipwidget.h"

#include <QFrame>
#include <QLayout>

#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaListView.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ElaTheme.h"
#include "deviceonship.h"
#include "spdlog/spdlog.h"

namespace {

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

}  // namespace

ShipWidget::ShipWidget(QWidget* parent)
    : BasePage(parent) {
    createCustomWidget("此页面可动态添加和管理多个船只");

    _AddShipBtn = new ElaPushButton("添加新船只", this);
    _AddShipBtn->setFixedSize(120, 36);
    connect(_AddShipBtn, &ElaPushButton::clicked, this, &ShipWidget::on_AddShipBtn_clicked);

    _SaveShipBtn = new ElaPushButton("保存所有船只", this);
    _SaveShipBtn->setFixedSize(120, 36);
    connect(_SaveShipBtn, &ElaPushButton::clicked, this, &ShipWidget::on_SaveShipBtn_clicked);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(_AddShipBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(_SaveShipBtn);

    _ShipListLayout = new QVBoxLayout();
    _ShipListLayout->setContentsMargins(10, 10, 10, 10);
    _ShipListLayout->setSpacing(20);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    centralWidget->setWindowTitle("船只属性管理");
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(_ShipListLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(btnLayout, 1);
    mainLayout->setStretch(0, 1);
    addCentralWidget(centralWidget);

    on_AddShipBtn_clicked();
}

ShipWidget::~ShipWidget() = default;

void ShipWidget::on_AddShipBtn_clicked() {
    ShipItemWidget* newItem = new ShipItemWidget(this);
    _ShipListLayout->insertWidget(_ShipListLayout->count() - 1, newItem);

    connect(newItem, &ShipItemWidget::deleteMe, this, &ShipWidget::on_RemoveShipItemRequested);
    spdlog::info("已添加新的船只 UI 条目");
}

void ShipWidget::on_RemoveShipItemRequested(ShipItemWidget* item) {
    if (!item) {
        return;
    }

    _ShipListLayout->removeWidget(item);
    item->deleteLater();
    spdlog::info("已移除船只 UI 条目");
}

void ShipWidget::on_SaveShipBtn_clicked() {
    auto* model = DataModel::instance();
    std::vector<ShipData> ships;

    for (int i = 0; i < _ShipListLayout->count(); ++i) {
        QLayoutItem* layoutItem = _ShipListLayout->itemAt(i);
        if (auto* widget = qobject_cast<ShipItemWidget*>(layoutItem->widget())) {
            ShipData data;
            QString errorMessage;
            if (!widget->tryBuildData(data, errorMessage)) {
                spdlog::error("船只 UI 基础校验失败，第 {} 项: {}", i + 1, errorMessage.toStdString());
                return;
            }

            ships.push_back(std::move(data));
        }
    }

    auto snapshot = model->createSnapshot();
    snapshot.allShips = ships;

    const auto validationResult = DataModel::validateSnapshot(snapshot);
    if (!validationResult.first) {
        spdlog::error("船只快照核心校验失败: {}", validationResult.second.toStdString());
        return;
    }

    model->allShips = std::move(snapshot.allShips);
    spdlog::info("船只保存成功，当前 DataModel 中共有 {} 艘船", model->allShips.size());
}

ShipItemWidget::ShipItemWidget(QWidget* parent)
    : QWidget(parent) {
    ElaText* xText = new ElaText("X 坐标", this);
    xText->setTextPixelSize(15);
    _X_offset = new ElaLineEdit(this);

    ElaText* yText = new ElaText("Y 坐标", this);
    yText->setTextPixelSize(15);
    _Y_offset = new ElaLineEdit(this);

    ElaText* zText = new ElaText("Z 坐标", this);
    zText->setTextPixelSize(15);
    _Z_offset = new ElaLineEdit(this);

    QHBoxLayout* firstLine = new QHBoxLayout();
    firstLine->addWidget(xText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_X_offset);
    firstLine->addSpacing(15);
    firstLine->addWidget(yText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_Y_offset);
    firstLine->addSpacing(15);
    firstLine->addWidget(zText);
    firstLine->addSpacing(15);
    firstLine->addWidget(_Z_offset);
    firstLine->addSpacing(15);

    ElaText* IDText = new ElaText("名称", this);
    IDText->setTextPixelSize(15);
    _ship_ID = new ElaLineEdit(this);

    ElaText* speedText = new ElaText("船速", this);
    speedText->setTextPixelSize(15);
    _ship_Speed = new ElaLineEdit(this);

    ElaText* orientationText = new ElaText("朝向", this);
    orientationText->setTextPixelSize(15);
    _ship_Orienteation = new ElaLineEdit(this);

    QHBoxLayout* secondLine = new QHBoxLayout();
    secondLine->addWidget(IDText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_ship_ID);
    secondLine->addSpacing(15);
    secondLine->addWidget(speedText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_ship_Speed);
    secondLine->addSpacing(15);
    secondLine->addWidget(orientationText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_ship_Orienteation);
    secondLine->addSpacing(15);

    _leftPannel = new QVBoxLayout();
    _leftPannel->addLayout(firstLine);
    _leftPannel->addLayout(secondLine);

    QWidget* deviceContainer = new QWidget(this);
    deviceContainer->setStyleSheet("background-color: transparent;");
    _deviceOnShipLayout = new QVBoxLayout(deviceContainer);
    _deviceOnShipLayout->setContentsMargins(0, 0, 0, 0);
    _deviceOnShipLayout->setSpacing(5);
    _deviceOnShipLayout->addStretch();

    ElaScrollArea* deviceScrollArea = new ElaScrollArea(this);
    deviceScrollArea->setWidgetResizable(true);
    deviceScrollArea->setWidget(deviceContainer);
    deviceScrollArea->setFixedHeight(200);
    deviceScrollArea->setMinimumWidth(250);
    deviceScrollArea->setFrameShape(QFrame::NoFrame);
    deviceScrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");
    deviceScrollArea->viewport()->setStyleSheet("background-color: transparent;");

    _AddDeviceOnShipBtn = new ElaPushButton("添加设备", this);
    connect(_AddDeviceOnShipBtn, &ElaPushButton::clicked, this, &ShipItemWidget::on_AddDeviceOnShipBtn_clicked);

    _rightPannel = new QVBoxLayout();
    _rightPannel->addWidget(deviceScrollArea);
    _rightPannel->addSpacing(10);
    _rightPannel->addWidget(_AddDeviceOnShipBtn);

    QWidget* mainWidget = new QWidget(this);
    QHBoxLayout* mainHLayout = new QHBoxLayout(mainWidget);
    mainHLayout->setContentsMargins(0, 0, 0, 0);
    mainHLayout->addLayout(_leftPannel, 5);
    mainHLayout->addLayout(_rightPannel, 3);

    _ReductionShipBtn = new ElaPushButton("删除该船", this);
    connect(_ReductionShipBtn, &ElaPushButton::clicked, this, &ShipItemWidget::on_SelfReductionBtn_clicked);

    QVBoxLayout* centerVLayout = new QVBoxLayout(this);
    centerVLayout->addWidget(mainWidget);
    centerVLayout->addWidget(_ReductionShipBtn);
}

void ShipItemWidget::setData(const ShipData& data) {
    _ship_ID->setText(QString::fromStdString(data.shipId));
    _X_offset->setText(QString::number(data.worldX));
    _Y_offset->setText(QString::number(data.worldY));
    _Z_offset->setText(QString::number(data.worldZ));
    _ship_Speed->setText(QString::number(data.shipSpeedMps));
    _ship_Orienteation->setText(QString::number(data.shipOrientationDeg));

    QLayoutItem* child = nullptr;
    while ((child = _deviceOnShipLayout->takeAt(0)) != nullptr) {
        if (child->spacerItem()) {
            _deviceOnShipLayout->addItem(child);
            break;
        }
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    for (const auto& deviceData : data.equipmentRefs) {
        DeviceonShip* deviceWidget = new DeviceonShip(this);
        deviceWidget->setData(deviceData);
        _deviceOnShipLayout->insertWidget(_deviceOnShipLayout->count() - 1, deviceWidget);

        connect(deviceWidget, &DeviceonShip::removalRequested, this, [this, deviceWidget]() {
            _deviceOnShipLayout->removeWidget(deviceWidget);
            deviceWidget->deleteLater();
        });
    }
}

ShipData ShipItemWidget::getData() const {
    ShipData data;
    QString errorMessage;
    if (!tryBuildData(data, errorMessage)) {
        spdlog::warn("船只 DTO 组装失败: {}", errorMessage.toStdString());
    }
    return data;
}

bool ShipItemWidget::tryBuildData(ShipData& data, QString& errorMessage) const {
    data = ShipData{};

    QString shipId;
    if (!readRequiredText(_ship_ID, QStringLiteral("船只 ID"), shipId, errorMessage)) {
        return false;
    }
    data.shipId = shipId.toStdString();

    if (!readRequiredNumber(_X_offset, QStringLiteral("X 坐标"), data.worldX, errorMessage) ||
        !readRequiredNumber(_Y_offset, QStringLiteral("Y 坐标"), data.worldY, errorMessage) ||
        !readRequiredNumber(_Z_offset, QStringLiteral("Z 坐标"), data.worldZ, errorMessage) ||
        !readRequiredNumber(_ship_Speed, QStringLiteral("船速"), data.shipSpeedMps, errorMessage) ||
        !readRequiredNumber(_ship_Orienteation, QStringLiteral("朝向"), data.shipOrientationDeg, errorMessage)) {
        return false;
    }

    for (int i = 0; i < _deviceOnShipLayout->count(); ++i) {
        QLayoutItem* item = _deviceOnShipLayout->itemAt(i);
        if (auto* devWidget = qobject_cast<DeviceonShip*>(item->widget())) {
            EquipmentOnShip equipmentRef = devWidget->getData();
            if (equipmentRef.equipmentId.trimmed().isEmpty()) {
                errorMessage = QStringLiteral("挂载设备引用不能为空");
                return false;
            }
            data.equipmentRefs.push_back(std::move(equipmentRef));
        }
    }

    return true;
}

void ShipItemWidget::on_AddDeviceOnShipBtn_clicked() {
    DeviceonShip* newDevice = new DeviceonShip(this);
    _deviceOnShipLayout->insertWidget(_deviceOnShipLayout->count() - 1, newDevice);

    connect(newDevice, &DeviceonShip::removalRequested, this, [this, newDevice]() {
        _deviceOnShipLayout->removeWidget(newDevice);
        newDevice->deleteLater();
    });
}

void ShipItemWidget::on_SelfReductionBtn_clicked() {
    emit deleteMe(this);
}
