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
    
    _ShipListLayout = new QVBoxLayout();
    //_shipItemWidget = new ShipItemWidget(this);
    //_shipItemWidget->setMinimumHeight(300);

    _AddShipBtn = new ElaPushButton("添加设备", this);
    _AddShipBtn->setFixedSize(60, 32);
    connect(_AddShipBtn, &ElaPushButton::clicked, 
        this, &ShipWidget::on_AddShipBtn_clicked);
    _SaveShipBtn = new ElaPushButton("保存", this);
    _SaveShipBtn->setFixedSize(60, 32);
    connect(_SaveShipBtn, &ElaPushButton::clicked, 
        this, &ShipWidget::on_SaveShipBtn_clicked);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(_AddShipBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(_SaveShipBtn);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    centralWidget->setWindowTitle("舰船属性管理");
    mainLayout->setContentsMargins(0, 0, 0, 0);
    _ShipListLayout->setSpacing(20); // 卡片之间的间距
    mainLayout->addLayout(_ShipListLayout);
	mainLayout->addStretch(); 
    mainLayout->addLayout(btnLayout);
	mainLayout->setStretch(0, 1); // 舰船列表占满剩余空间
    addCentralWidget(centralWidget);
    on_AddShipBtn_clicked();
}

ShipWidget::~ShipWidget()
{
}

void ShipWidget::on_AddShipBtn_clicked() {
    ShipItemWidget* newItem = new ShipItemWidget(this);   
    newItem->setMinimumHeight(300);
    // 2. 将其插入到滚动布局中 (假设 _scrollLayout 是你放置设备的布局)
    // 建议在布局最后保留一个 addStretch()，这样新条目会往上排
    _ShipListLayout->insertWidget(_ShipListLayout->count() - 1, newItem);
    
    // 3. 处理删除信号
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
 
ShipItemWidget::ShipItemWidget(QWidget *parent) : ElaScrollPageArea(parent)
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

    _deviceOnShipListView = new ElaListView(this);
    _AddDeviceOnShipBtn = new ElaPushButton("添加设备", this);
    _AddDeviceOnShipBtn->setFixedSize(60, 32);
    connect(_AddDeviceOnShipBtn, &ElaPushButton::clicked, this, 
        &ShipItemWidget::on_AddDeviceOnShipBtn_clicked);
    _rightPannel = new QVBoxLayout();
    _rightPannel->addWidget(_deviceOnShipListView);
    _rightPannel->addStretch();
    _rightPannel->addWidget(_AddDeviceOnShipBtn);

    QWidget* mainWidget = new QWidget(this);
    // mainWidget->setWindowTitle("船");
    QHBoxLayout* mainHLayout = new QHBoxLayout(mainWidget);
    mainHLayout->setContentsMargins(0, 0, 0, 0);
    mainHLayout->addLayout(_leftPannel);
    mainHLayout->addLayout(_rightPannel);

    // Delete button
    _ReductionShipBtn = new ElaPushButton("删除该船", this);
    _ReductionShipBtn->setFixedSize(60, 32);
    connect(_ReductionShipBtn, &ElaPushButton::clicked,
        this, &ShipItemWidget::on_SelfReductionBtn_clicked);
    QVBoxLayout* centerVLayout = new QVBoxLayout(this);
    centerVLayout->addWidget(mainWidget);
    centerVLayout->addStretch();
    centerVLayout->addWidget(_ReductionShipBtn);
    
}

void ShipItemWidget::setData(const ShipData &data)
{
    _ship_ID->setText(QString::fromStdString(data.shipID));
    _X_offset->setText(QString::number(data.X_offset));
    _Y_offset->setText(QString::number(data.Y_offset));
    _Z_offset->setText(QString::number(data.Z_offset));
    _ship_Speed->setText(QString::number(data.ship_Speed));
    _ship_Orienteation->setText(QString::number(data.ship_Orienteation));

    // 这里添加代码来填充设备列表
}   

ShipData ShipItemWidget::getData() const
{
    ShipData data;
    data.shipID = _ship_ID->text().toStdString();
    data.X_offset = _X_offset->text().toDouble();
    data.Y_offset = _Y_offset->text().toDouble();
    data.Z_offset = _Z_offset->text().toDouble();
    data.ship_Speed = _ship_Speed->text().toDouble();
    data.ship_Orienteation = _ship_Orienteation->text().toDouble();

    // 这里添加代码来从设备列表中提取设备配置
    QLayoutItem* child;
    while ((child = _deviceListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    // 重新添加弹簧
    _deviceListLayout->addStretch();

    // 3. 根据数据模型恢复设备列表
    // 假设 ShipData 中有一个 std::vector<DeviceData> devices
    for (const auto& deviceData : data.Equipments) {
        DeviceonShip* deviceWidget = new DeviceonShip();
         deviceWidget->setData(deviceData); 

        _deviceListLayout->insertWidget(_deviceListLayout->count() - 1, deviceWidget);
    }
    return data;
}

void ShipItemWidget::on_AddDeviceOnShipBtn_clicked()
{
    _deviceOnShipListView->addScrollBarWidget(new DeviceonShip(this),Qt::AlignTop | Qt::AlignLeft );
}

void ShipItemWidget::on_SelfReductionBtn_clicked()
{
    emit deleteMe(this);
}