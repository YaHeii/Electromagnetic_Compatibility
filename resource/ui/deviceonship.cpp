#include "deviceonship.h"
#include "ElaComboBox.h"
#include "ElaText.h"
#include "ElaPushButton.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

DeviceonShip::DeviceonShip(QWidget *parent)
    : QWidget(parent)
{
    // 设置窗口标题
    setWindowTitle("DeviceonShip");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // 创建设备名称标签
    ElaText* EquipmentNameText = new ElaText("设备名称", this);
    EquipmentNameText->setTextPixelSize(15);
    EquipmentNameText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    // 创建设备ID下拉框
    ElaComboBox *EquipmentIDCombo = new ElaComboBox(this);
    //     QStringList comboList{
    //     "我愿投身前途未卜的群星",
    //     "潜行 步伐小心翼翼",
    //     "不留游走痕迹",
    //     "如同一簇幽灵",
    //     "所谓 道德加上伦理",
    //     "抱歉只能律己"};
    // _comboBox->addItems(comboList);
    // 创建删除按钮
    ElaPushButton* deleteDeviceonShip = new ElaPushButton("删除", this);
    deleteDeviceonShip->setFixedSize(60, 32);
    connect(deleteDeviceonShip, &ElaPushButton::clicked, this, &DeviceonShip::on_deleteDeviceonShip_clicked);


    layout->addWidget(EquipmentNameText);
    layout->addSpacing(10);
    layout->addWidget(EquipmentIDCombo);
    layout->addSpacing(10);
    layout->addWidget(deleteDeviceonShip);
    layout->addStretch();
    //deviceonShipLayout->addStrech();

    QWidget* centralWidget = new QWidget(this);
    // centralWidget->setWindowTitle("");
    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->addLayout(layout);
    centerVLayout->addStretch();
    //addCentralWidget(centralWidget);
}

DeviceonShip::~DeviceonShip() = default;

void DeviceonShip::on_deleteDeviceonShip_clicked() {
    emit removalRequested();
}
