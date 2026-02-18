#include "deviceonship.h"
#include "ElaComboBox.h"
#include "ElaText.h"
#include "ElaPushButton.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <Qevent>

#include "spdlog/spdlog.h"

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
    _EquipmentIDCombo = new ElaComboBox(this);
    refreshEquipmentList();
    _EquipmentIDCombo->installEventFilter(this);
    // 创建删除按钮
    ElaPushButton* deleteDeviceonShip = new ElaPushButton("删除", this);
    deleteDeviceonShip->setFixedSize(60, 32);
    connect(deleteDeviceonShip, &ElaPushButton::clicked, this, &DeviceonShip::on_deleteDeviceonShip_clicked);


    layout->addWidget(EquipmentNameText);
    layout->addSpacing(10);
    layout->addWidget(_EquipmentIDCombo);
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

void DeviceonShip::refreshEquipmentList() {
    // 1. 记录当前选中的文本，防止刷新后用户的选择丢失
    QString currentSelection = _EquipmentIDCombo->currentText();

    // 2. 清空现有选项
    _EquipmentIDCombo->clear();

    // 3. 从单例中获取最新设备列表并填充
    auto* model = DataModel::instance();
    for (const auto& eq : model->allEquipments) {
        _EquipmentIDCombo->addItem(eq.equipmentID);
    }

    // 4. 尝试恢复之前的选择
    int index = _EquipmentIDCombo->findText(currentSelection);
    if (index >= 0) {
        _EquipmentIDCombo->setCurrentIndex(index);
    }
    else if (_EquipmentIDCombo->count() > 0) {
        // 如果之前的选择不存在了（比如设备被删了），默认选中第一个
        _EquipmentIDCombo->setCurrentIndex(0);
    }
}

EquipmentOnShip DeviceonShip::getData() const {
    EquipmentOnShip data;
    data.equipmentID = _EquipmentIDCombo->currentText();
    data.isEnabled = true; // 默认启用，后续可根据需要添加 CheckBox 控制
    return data;
}

// 【新增】数据结构 -> UI
void DeviceonShip::setData(const EquipmentOnShip& data) {
    _data = data;

    // 在下拉框中查找对应的设备ID并选中
    int index = _EquipmentIDCombo->findText(data.equipmentID);
    if (index >= 0) {
        _EquipmentIDCombo->setCurrentIndex(index);
    }
    else {
        // 容错：如果加载的配置中包含了当前模型中没有的设备
        spdlog::warn("加载设备配置失败: 在当前设备库中找不到 ID 为 '{}' 的设备", data.equipmentID.toStdString());
        // 可选：强行添加进去以防丢失数据
        // _EquipmentIDCombo->addItem(data.equipmentID);
        // _EquipmentIDCombo->setCurrentText(data.equipmentID);
    }
}

bool DeviceonShip::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _EquipmentIDCombo) {
        // 当鼠标在下拉框上按下时触发
        if (event->type() == QEvent::MouseButtonPress) {
            // 在下拉框真正展开前，刷新数据
            refreshEquipmentList();

            // 注意：不要 return true，否则会拦截点击事件，导致下拉框无法展开
            return false;
        }
    }
    // 其他事件交给父类处理
    return QWidget::eventFilter(watched, event);
}