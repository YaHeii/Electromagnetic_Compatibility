#include "shipwidget.h"
#include <QLayout>
#include "deviceonship.h"

#include "ElaText.h"
#include "ElaPushButton.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaListView.h"
#include "ElaScrollArea.h"
#include "ElaTheme.h"
#include "ElaScrollPageArea.h"
#include "spdlog/spdlog.h"

ShipWidget::ShipWidget(QWidget *parent) 
    : BasePage(parent)
{
    createCustomWidget("此页面可动态添加和管理多个舰船");

    _AddShipBtn = new ElaPushButton("添加新船只", this);
    _AddShipBtn->setFixedSize(120, 36);
    connect(_AddShipBtn, &ElaPushButton::clicked, 
        this, &ShipWidget::on_AddShipBtn_clicked);
    _SaveShipBtn = new ElaPushButton("保存所有船只", this);
    _SaveShipBtn->setFixedSize(120, 36);
    connect(_SaveShipBtn, &ElaPushButton::clicked, 
        this, &ShipWidget::on_SaveShipBtn_clicked);
    
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
    centralWidget->setWindowTitle("舰船属性管理");
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(_ShipListLayout);
	mainLayout->addStretch(); 
    mainLayout->addLayout(btnLayout, 1);
	mainLayout->setStretch(0, 1); // 舰船列表占满剩余空间
    addCentralWidget(centralWidget);

    on_AddShipBtn_clicked();
}

ShipWidget::~ShipWidget()
{
}

void ShipWidget::on_AddShipBtn_clicked() {
    ShipItemWidget* newItem = new ShipItemWidget(this);   
    _ShipListLayout->insertWidget(_ShipListLayout->count() - 1, newItem);
    
    connect(newItem, &ShipItemWidget::deleteMe, this, &ShipWidget::on_RemoveShipItemRequested);
    spdlog::info("已添加新的舰船 UI 条目");
}

void ShipWidget::on_RemoveShipItemRequested(ShipItemWidget* item)
{
    if (!item) return;
    
    // 从布局中移除并销毁
    _ShipListLayout->removeWidget(item);
    item->deleteLater();
    
    spdlog::info("已移除舰船 UI 条目");
}


void ShipWidget::on_SaveShipBtn_clicked()
{
     auto* model = DataModel::instance();
    // 1. 清空当前模型中的舰船列表，以 UI 上的实际条目为准
    model->allShips.clear();

    // 2. 遍历布局，收集每个条目中的数据
    for (int i = 0; i < _ShipListLayout->count(); ++i) {
        QLayoutItem* layoutItem = _ShipListLayout->itemAt(i);
        if (auto* widget = qobject_cast<ShipItemWidget*>(layoutItem->widget())) {
            // 获取 UI 当前的数据并验证
            ShipData data = widget->getData();

            if(data.validate_Ship().first == false) {
                spdlog::error("舰船 {} 的数据校验未通过，{}", data.shipID, data.validate_Ship().second.toStdString());
                continue;
            }   
            
            model->allShips.push_back(data);
        }
    }

    spdlog::info("舰船保存成功，当前 DataModel 中共有 {} 个舰船", model->allShips.size());
}
 
ShipItemWidget::ShipItemWidget(QWidget *parent) : QWidget(parent)
{
     // X coordinate
    ElaText* xText = new ElaText("X坐标", this);
    xText->setTextPixelSize(15);
    _X_offset = new ElaLineEdit(this);
    
    // Y coordinate
    ElaText* yText = new ElaText("Y坐标", this);
    yText->setTextPixelSize(15);
    _Y_offset = new ElaLineEdit(this);

    // Z coordinate
    ElaText* zText = new ElaText("Z坐标", this);
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

    //ShipID
    ElaText* IDText = new ElaText("名称", this);
    IDText->setTextPixelSize(15);
    _ship_ID = new ElaLineEdit(this);

    // Speed
    ElaText* speedText = new ElaText("船速", this);
    speedText->setTextPixelSize(15);
    _ship_Speed = new ElaLineEdit(this);
    
    // Orientation
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
    deviceScrollArea->setWidgetResizable(true); // 【关键】允许内部容器宽度自适应画框
    deviceScrollArea->setWidget(deviceContainer);
    deviceScrollArea->setFixedHeight(200);
    deviceScrollArea->setMinimumWidth(250);
    deviceScrollArea->setFrameShape(QFrame::NoFrame);
    deviceScrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");
    deviceScrollArea->viewport()->setStyleSheet("background-color: transparent;");
    
    _AddDeviceOnShipBtn = new ElaPushButton("添加设备", this);
    // _AddDeviceOnShipBtn->setFixedSize(100, 32);
    connect(_AddDeviceOnShipBtn, &ElaPushButton::clicked, this, 
        &ShipItemWidget::on_AddDeviceOnShipBtn_clicked);


    _rightPannel = new QVBoxLayout();
    _rightPannel->addWidget(deviceScrollArea);
    _rightPannel->addSpacing(10);
    _rightPannel->addWidget(_AddDeviceOnShipBtn);
    
    QWidget* mainWidget = new QWidget(this);
    // mainWidget->setWindowTitle("船");
    QHBoxLayout* mainHLayout = new QHBoxLayout(mainWidget);
    mainHLayout->setContentsMargins(0, 0, 0, 0);
    mainHLayout->addLayout(_leftPannel,5);
    mainHLayout->addLayout(_rightPannel,3);

    // Delete button
    _ReductionShipBtn = new ElaPushButton("删除该船", this);
    connect(_ReductionShipBtn, &ElaPushButton::clicked,
        this, &ShipItemWidget::on_SelfReductionBtn_clicked);
    QVBoxLayout* centerVLayout = new QVBoxLayout(this);
    centerVLayout->addWidget(mainWidget);
    centerVLayout->addWidget(_ReductionShipBtn);
    
}
// Model -> View
void ShipItemWidget::setData(const ShipData &data)
{
    _ship_ID->setText(QString::fromStdString(data.shipID));
    _X_offset->setText(QString::number(data.X_offset));
    _Y_offset->setText(QString::number(data.Y_offset));
    _Z_offset->setText(QString::number(data.Z_offset));
    _ship_Speed->setText(QString::number(data.ship_Speed));
    _ship_Orienteation->setText(QString::number(data.ship_Orienteation));

    QLayoutItem* child;
    while ((child = _deviceOnShipLayout->takeAt(0)) != nullptr) {
        if (child->spacerItem()) {
            _deviceOnShipLayout->addItem(child); // 把弹簧放回去并退出循环
            break;
        }
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    // 2. 根据数据生成新的设备 UI
    for (const auto& deviceData : data.Equipments) {
        DeviceonShip* deviceWidget = new DeviceonShip(this);
        deviceWidget->setData(deviceData); 
        _deviceOnShipLayout->insertWidget(_deviceOnShipLayout->count() - 1, deviceWidget);
        
        // 记得连上删除信号
        connect(deviceWidget, &DeviceonShip::removalRequested, this, [this, deviceWidget]() {
            _deviceOnShipLayout->removeWidget(deviceWidget);
            deviceWidget->deleteLater();
        });
    }
}   
// View -> Model
ShipData ShipItemWidget::getData() const
{
    ShipData data;
    data.shipID = _ship_ID->text().toStdString();
    data.X_offset = _X_offset->text().toDouble();
    data.Y_offset = _Y_offset->text().toDouble();
    data.Z_offset = _Z_offset->text().toDouble();
    data.ship_Speed = _ship_Speed->text().toDouble();
    data.ship_Orienteation = _ship_Orienteation->text().toDouble();
// 遍历布局，收集每一个 DeviceonShip 控件里的数据
    for (int i = 0; i < _deviceOnShipLayout->count(); ++i) {
        QLayoutItem* item = _deviceOnShipLayout->itemAt(i);
        if (auto* devWidget = qobject_cast<DeviceonShip*>(item->widget())) {
            data.Equipments.push_back(devWidget->getData());
        }
    }
    
    return data;
}

void ShipItemWidget::on_AddDeviceOnShipBtn_clicked()
{
    DeviceonShip* newDevice = new DeviceonShip(this);
    
    // 插入到布局中，count() - 1 是为了插在底部的 addStretch() 弹簧上方
    _deviceOnShipLayout->insertWidget(_deviceOnShipLayout->count() - 1, newDevice);

    // 监听内部设备组件发出的删除信号
    connect(newDevice, &DeviceonShip::removalRequested, this, [this, newDevice]() {
        _deviceOnShipLayout->removeWidget(newDevice);
        newDevice->deleteLater();
    });
}

void ShipItemWidget::on_SelfReductionBtn_clicked()
{
    emit deleteMe(this);
}