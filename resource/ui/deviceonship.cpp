#include "deviceonship.h"
#include "ElaComboBox.h"
#include "ElaText.h"
#include "ElaPushButton.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

DeviceonShip::DeviceonShip(QWidget *parent) : QWidget(parent) {
    // 设置窗口标题
    setWindowTitle("DeviceonShip");
    // 设置窗口大小
    setGeometry(0, 0, 343, 120);

    // 创建主水平布局
    QHBoxLayout *horizontalLayout_4 = new QHBoxLayout(this);

    // 创建垂直布局
    QVBoxLayout *verticalLayout = new QVBoxLayout();

    // 创建水平布局，设置stretch比例
    QHBoxLayout *horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->setStretch(0, 1);
    horizontalLayout_2->setStretch(1, 2);
    horizontalLayout_2->setStretch(2, 2);

    // 创建设备名称标签
    ElaText *label = new ElaText(this);
    QFont font = label->font();
    font.setPointSize(9);
    label->setFont(font);
    label->setText("设备名称");

    // 创建设备ID下拉框
    ElaComboBox *EquipmentID = new ElaComboBox(this);

    // 创建删除按钮
    ElaPushButton *deleteDeviceonShip = new ElaPushButton(this);
    deleteDeviceonShip->setText("-");

    // 将控件添加到水平布局
    horizontalLayout_2->addWidget(label);
    horizontalLayout_2->addWidget(EquipmentID);
    horizontalLayout_2->addWidget(deleteDeviceonShip);

    // 将水平布局添加到垂直布局
    verticalLayout->addLayout(horizontalLayout_2);

    // 将垂直布局添加到主水平布局
    horizontalLayout_4->addLayout(verticalLayout);

    // 设置主布局
    setLayout(horizontalLayout_4);
}

DeviceonShip::~DeviceonShip() {
    delete ui;
}

void DeviceonShip::on_deleteDeviceonShip_clicked() {
    emit removalRequested();
}
