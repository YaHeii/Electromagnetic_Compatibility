#include "DeviceWidget.h"
#include <QDebug>
#include "spdlog/spdlog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include "ElaText.h"
#include "ElaPushButton.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaScrollPageArea.h"
#include "ElaTheme.h"

DeviceWidget::DeviceWidget(QWidget *parent)
    : BasePage(parent)
{
    setWindowTitle("设备属性");
    createCustomWidget("此页面添加所有可用设备");

    // 设备类型选择
    ElaText *typeText = new ElaText("设备类型", this);
    typeText->setTextPixelSize(15);
    _equipmentType = new ElaComboBox(this);
    QStringList comboList{
    "发射机",
    "接收机",
    "收发一体机"};
    _equipmentType->addItems(comboList);
    connect(_equipmentType, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &DeviceWidget::onEquipmentTypeChanged);

    QHBoxLayout *firstLine = new QHBoxLayout();
    firstLine->addWidget(typeText);
    firstLine->addSpacing(10);
    firstLine->addWidget(_equipmentType);
    
    // 增益和设备ID
    ElaText *gainText = new ElaText("发射/接收增益", this);
    gainText->setTextPixelSize(15);
    _gain = new ElaLineEdit(this);
    _gain->setPlaceholderText("dBm");
    ElaText *idText = new ElaText("设备ID", this);
    idText->setTextPixelSize(15);
    _equipmentID = new ElaLineEdit(this);
    _equipmentID->setPlaceholderText("建议设为纯数字或纯字母");

    QHBoxLayout* secondLine = new QHBoxLayout();
    secondLine->addWidget(gainText);
    secondLine->addSpacing(10);
    secondLine->addWidget(_gain);
    secondLine->addSpacing(10);
    secondLine->addWidget(idText);
    secondLine->addSpacing(10);
    secondLine->addWidget(_equipmentID);
    
    // 坐标输入
    ElaText *xText = new ElaText("X坐标", this);
    xText->setTextPixelSize(15);
    _X_offset = new ElaLineEdit(this);
    _X_offset->setPlaceholderText("X坐标");
    ElaText *yText = new ElaText("Y坐标",this);
    yText->setTextPixelSize(15);
    _Y_offset = new ElaLineEdit(this);
    _Y_offset->setPlaceholderText("Y坐标");
    ElaText *zText = new ElaText("Z坐标",this);
    zText->setTextPixelSize(15);
    _Z_offset = new ElaLineEdit(this);
    _Z_offset->setPlaceholderText("Z坐标");

    QHBoxLayout* thirdLine = new QHBoxLayout();
    thirdLine->addWidget(xText);
    thirdLine->addSpacing(15);      
    thirdLine->addWidget(_X_offset);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(yText);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(_Y_offset);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(zText);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(_Z_offset);

    QVBoxLayout* Input = new QVBoxLayout();
    Input->addLayout(firstLine);
    Input->addSpacing(10);
    Input->addLayout(secondLine);
    Input->addSpacing(10);
    Input->addLayout(thirdLine);
    Input->addSpacing(10);

	_RecieverWidget = new ElaScrollPageArea(this);
	_TransmitterWidget = new ElaScrollPageArea(this);
    _RecieverWidget->setFixedHeight(220); // 估算一个合适的高度
    _TransmitterWidget->setFixedHeight(220);
    setupReceiverWidget(_RecieverWidget);    // 接收机专有参数
    setupTransmitterWidget(_TransmitterWidget); // 发射机专有参数

    // 创建删除按钮
    _equipmentReduction = new ElaPushButton("删除设备", this);
    _equipmentReduction->setFixedSize(60, 32);
    connect(_equipmentReduction, &ElaPushButton::clicked,
        this, &DeviceWidget::on_equipmentReduction_clicked);

    // 添加到主布局
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("添加设备");
    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->addLayout(Input);           
    centerVLayout->addWidget(_TransmitterWidget); 
    centerVLayout->addWidget(_RecieverWidget);

    // 按钮布局
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(_equipmentReduction);
    centerVLayout->addLayout(btnLayout);      
    centerVLayout->addStretch();

    addCentralWidget(centralWidget);
    // 连接删除按钮信号
    onEquipmentTypeChanged();
    resetTransmitterUI();
    resetReceiverUI();
    
}

DeviceWidget::~DeviceWidget()
{
}

void DeviceWidget::setupReceiverWidget(ElaScrollPageArea* container)
{
    // 接收机参数区域
    // 中心频率、接收带宽
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

    // 第二行：接收机灵敏度、干扰阈值
    ElaText *sensitiveText = new ElaText("接收机灵敏度", this);
    sensitiveText->setTextPixelSize(13);
    _Sensitive_Receiver = new ElaLineEdit(this);
    _Sensitive_Receiver->setPlaceholderText("dBm");

    ElaText *interferenceText = new ElaText("干扰阈值", this);
    interferenceText->setTextPixelSize(13);
    _InterferenceMargin_Receiver = new ElaLineEdit(this);
    _InterferenceMargin_Receiver->setPlaceholderText("dBm");

    QHBoxLayout* secondLine = new QHBoxLayout();
	secondLine->addWidget(sensitiveText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_Sensitive_Receiver);
    secondLine->addSpacing(15);
    secondLine->addWidget(interferenceText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_InterferenceMargin_Receiver);

    // 第三行：信噪比阈值、噪声系数
    ElaText *sinrText = new ElaText("信噪比阈值", this);
    sinrText->setTextPixelSize(13);
    _SINRMargin_Receiver = new ElaLineEdit(this);
    _SINRMargin_Receiver->setPlaceholderText("dB");

    ElaText * noiseText = new ElaText("噪声系数", this);
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
    //addCentralWidget(centralWidget);
    if (!container->layout()) {
        auto* v = new QVBoxLayout(container);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);
    }
    container->layout()->addWidget(centralWidget);
}

void DeviceWidget::setupTransmitterWidget(ElaScrollPageArea* container)
{
        // 发射机参数区域
    // 第一行：中心频率、发射带宽、发射功率
    ElaText *centralFText = new ElaText("中心频率", this);
    centralFText->setTextPixelSize(15);
    _CentralF_Transmitter = new ElaLineEdit(this);
    _CentralF_Transmitter->setPlaceholderText("GHz");

    ElaText *bandwidthText = new ElaText("发射带宽", this);
    bandwidthText->setTextPixelSize(15);
    _Bandwidth_Transmitter = new ElaLineEdit(this);
    _Bandwidth_Transmitter->setPlaceholderText("MHz");

    ElaText *powerText = new ElaText("发射功率", this);
    powerText->setTextPixelSize(15);
    _Power_Transmitter = new ElaLineEdit(this);
    _Power_Transmitter->setPlaceholderText("dBm");

    QHBoxLayout *firstLine = new QHBoxLayout();
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

    // 第二行：天线指向角、波束宽度
    ElaText *phiText = new ElaText("天线指向角", this);
    phiText->setTextPixelSize(13);
    _antennaPhi_Transmitter = new ElaLineEdit(this);
    _antennaPhi_Transmitter->setPlaceholderText("°");

    ElaText *beamwidthText = new ElaText("波束宽度", this);
    beamwidthText->setTextPixelSize(13);
    _Beamwidth_Transmitter = new ElaLineEdit(this);
    _Beamwidth_Transmitter->setPlaceholderText("°");

    QHBoxLayout *secondLine = new QHBoxLayout();
    secondLine->addWidget(phiText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_antennaPhi_Transmitter);
    secondLine->addSpacing(15);
    secondLine->addWidget(beamwidthText);
    secondLine->addSpacing(15);
    secondLine->addWidget(_Beamwidth_Transmitter);

    // 第三行：极化方式、天线类型
    ElaText *polarizationText = new ElaText("极化方式", this);
    polarizationText->setTextPixelSize(13);
    _PolarizationMethod_Transmitter = new ElaComboBox(this);
    QStringList polarizationList{
        "垂直极化",
        "水平极化"};
    _PolarizationMethod_Transmitter->addItems(polarizationList);

    ElaText *antennaTypeText = new ElaText("天线类型", this);
    antennaTypeText->setTextPixelSize(13);
    _antennaType_Transmitter = new ElaComboBox(this);
    QStringList antennaTypeList{
        "喇叭天线",
        "赋型波束天线",
        "抛物面天线"};
    _antennaType_Transmitter->addItems(antennaTypeList);

    QHBoxLayout *thirdLine = new QHBoxLayout();
    thirdLine->addWidget(polarizationText);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(_PolarizationMethod_Transmitter);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(antennaTypeText);
    thirdLine->addSpacing(15);
    thirdLine->addWidget(_antennaType_Transmitter);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addLayout(firstLine);
    mainLayout->addLayout(secondLine);
    mainLayout->addLayout(thirdLine);


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

void DeviceWidget::setData(const EquipmentData &data)
{
    _currentId = data.equipmentID;

    // --- 1. 基本参数 ---
    _equipmentID->setText(data.equipmentID);
    _equipmentType->setCurrentText(data.equipmentType);
    _gain->setText(QString::number(data.Gain));
    
    _X_offset->setText(QString::number(data.X_offset));
    _Y_offset->setText(QString::number(data.Y_offset));
    _Z_offset->setText(QString::number(data.Z_offset));

    // --- 2. 接收机参数 ---
    if(data.equipmentType == "接收机" ){
    _CentralF_Receiver->setText(QString::number(data.CentralF_Reciever));
    _Bandwidth_Receiver->setText(QString::number(data.Bandwidth_Reciever));
    _Sensitive_Receiver->setText(QString::number(data.Sensitive_reciever));
    _InterferenceMargin_Receiver->setText(QString::number(data.interferenceMargin));
    _SINRMargin_Receiver->setText(QString::number(data.SINRMargin));
    _NoiseFigure_Receiver->setText(QString::number(data.noiseFigure));
    } else if(data.equipmentType == "发射机"){
    // --- 3. 发射机参数 ---
    _CentralF_Transmitter->setText(QString::number(data.CentralF_Transmitter));
    _Bandwidth_Transmitter->setText(QString::number(data.Bandwidth_Transmitter));
    _Power_Transmitter->setText(QString::number(data.Power_Transmitter));
    _antennaPhi_Transmitter->setText(QString::number(data.antennaPhi_Transmitter));
    _Beamwidth_Transmitter->setText(QString::number(data.Beamwidth_Transmitter));
    _PolarizationMethod_Transmitter->setCurrentText(data.PolarizationMethod_Transmitter);
    _antennaType_Transmitter->setCurrentText(data.antennaType_Transmitter);
    }
}

void DeviceWidget::updateModelData() {
    bool ok;
    // 遍历全局数据列表找到当前设备
    for (EquipmentData &data : DataModel::instance()->allEquipments) {
        if(data.equipmentID == _currentId){
            // --- 公共参数总是保存 ---
            data.equipmentID = _equipmentID->text();
            data.equipmentType = _equipmentType->currentText();
            data.Gain = _gain->text().toDouble(&ok);
            data.X_offset = _X_offset->text().toDouble(&ok);
            data.Y_offset = _Y_offset->text().toDouble(&ok);
            data.Z_offset = _Z_offset->text().toDouble(&ok);

            // --- 发射机参数处理 ---
            // 判断发射机容器是否可见
            if (_TransmitterWidget->isVisible()) {
                data.CentralF_Transmitter = _CentralF_Transmitter->text().toDouble(&ok);
                data.Bandwidth_Transmitter = _Bandwidth_Transmitter->text().toDouble(&ok);
                data.Power_Transmitter = _Power_Transmitter->text().toDouble(&ok);
                data.antennaPhi_Transmitter = _antennaPhi_Transmitter->text().toDouble(&ok);
                data.Beamwidth_Transmitter = _Beamwidth_Transmitter->text().toDouble(&ok);
                data.PolarizationMethod_Transmitter = _PolarizationMethod_Transmitter->currentText();
                data.antennaType_Transmitter = _antennaType_Transmitter->currentText();
            } else {
                // 不可见，强制写入无效值（0）
                data.CentralF_Transmitter = 0;
                data.Bandwidth_Transmitter = 0;
                data.Power_Transmitter = 0;
                data.antennaPhi_Transmitter = 0;
                data.Beamwidth_Transmitter = 0;
                data.PolarizationMethod_Transmitter = "";
                data.antennaType_Transmitter = "";
            }

            // --- 接收机参数处理 ---
            if (_RecieverWidget->isVisible()) {
                data.CentralF_Reciever = _CentralF_Receiver->text().toDouble(&ok);
                data.Bandwidth_Reciever = _Bandwidth_Receiver->text().toDouble(&ok);
                data.Sensitive_reciever = _Sensitive_Receiver->text().toDouble(&ok);
                data.interferenceMargin = _InterferenceMargin_Receiver->text().toDouble(&ok);
                data.SINRMargin = _SINRMargin_Receiver->text().toDouble(&ok);
                data.noiseFigure = _NoiseFigure_Receiver->text().toDouble(&ok);
            } else {
                data.CentralF_Reciever = 0;
                data.Bandwidth_Reciever = 0;
                data.Sensitive_reciever = 0;
                data.interferenceMargin = 0;
                data.SINRMargin = 0;
                data.noiseFigure = 0;
            }
            spdlog::info("设备 {} 参数已经保存", data.equipmentID.toStdString());
            break;
        }
    }
}

void DeviceWidget::onEquipmentTypeChanged()
{
    QString type = _equipmentType->currentText();
    if (type == "发射机") {
        _TransmitterWidget->setVisible(true);
        _RecieverWidget->setVisible(false);
        // 清空其他参数
        resetReceiverUI();
        spdlog::debug("正在设定{}参数", type.toStdString());
    }
    else if (type == "接收机") {
        _TransmitterWidget->setVisible(false);
        _RecieverWidget->setVisible(true);
        // 清空其他参数
        resetTransmitterUI();
        spdlog::debug("正在设定{}参数", type.toStdString());
    }
    else if (type == "收发一体机") {
        _TransmitterWidget->setVisible(true);
        _RecieverWidget->setVisible(true);
        spdlog::debug("正在设定{}参数", type.toStdString());
    }
    
}



void DeviceWidget::on_equipmentReduction_clicked()
{
    emit removalRequested(_currentId);
}

void DeviceWidget::resetTransmitterUI() {
    _CentralF_Transmitter->setText("0");
    _Bandwidth_Transmitter->setText("0");
    _Power_Transmitter->setText("0");
    _antennaPhi_Transmitter->setText("0");
    _Beamwidth_Transmitter->setText("0");
    // 下拉框可以重置到默认索引0
    _PolarizationMethod_Transmitter->setCurrentIndex(0);
    _antennaType_Transmitter->setCurrentIndex(0);
}

void DeviceWidget::resetReceiverUI() {
    _CentralF_Receiver->setText("0");
    _Bandwidth_Receiver->setText("0");
    _Sensitive_Receiver->setText("0");
    _InterferenceMargin_Receiver->setText("0");
    _SINRMargin_Receiver->setText("0");
    _NoiseFigure_Receiver->setText("0");
}


//void DeviceWidget::on_DeviceSave_clicked()
//{
//    if (updateDeviceModelFromView()) {
//        //QMessageBox::information(this, "成功", "设备信息已保存并校验通过。");
//        spdlog::info("Device data saved and validated.");
//    }
//}


//bool DeviceWidget::updateDeviceModelFromView()
//{
//    // 1. 从 View 同步到 Model
//    for (int i = 0; i < deviceLayout->count(); ++i) {
//        QLayoutItem* item = deviceLayout->itemAt(i);
//        if (item && item->widget()) {
//            DeviceWidget* widget = qobject_cast<DeviceWidget*>(item->widget());
//            if (widget) {
//                // 让每个Widget用自己UI上的当前值去更新数据模型
//                widget->updateModelData();
//            }
//        }
//    }
//
//    // 2. 执行校验逻辑
//    auto& equipments = DataModel::instance()->allEquipments;
//    for (int i = 0; i < equipments.size(); ++i) {
//        auto result = equipments[i].validate();
//        if (!result.first) {
//            QString errorMsg = QString("设备数据错误 (ID: %1):%2")
//                .arg(equipments[i].equipmentID)
//                .arg(result.second);
//            //QMessageBox::critical(this, "校验失败", errorMsg);
//            spdlog::error("Validation failed for equipment {}: {}",
//                equipments[i].equipmentID.toStdString(), result.second.toStdString());
//            return false;
//        }
//    }
//
//    //_treeView->syncViewWithModel();
//    return true;
//}
//
//
//
//void DeviceWidget::onDeviceWidgetRemovalRequested(const QString& id)
//{
//    // 1. 从DataModel中移除对应的数据
//    auto& equipments = DataModel::instance()->allEquipments;
//    auto it = std::remove_if(equipments.begin(), equipments.end(),
//        [&](const EquipmentData& ed) { return ed.equipmentID == id; });
//
//    if (it != equipments.end()) {
//        equipments.erase(it, equipments.end());
//        spdlog::info("设备 {} 的数据已从模型中删除。", id.toStdString());
//
//        // 2. 遍历布局，找到并删除对应的UI控件
//        for (int i = 0; i < deviceLayout->count(); ++i) {
//            QLayoutItem* item = deviceLayout->itemAt(i);
//            if (item && item->widget()) {
//                DeviceWidget* widget = qobject_cast<DeviceWidget*>(item->widget());
//                // 假设DeviceWidget有方法可以获取其ID
//                if (widget && widget->getID() == id) {
//                    deviceLayout->removeWidget(widget);
//                    widget->deleteLater();
//                    spdlog::info("设备 {} 的UI控件已删除。", id.toStdString());
//                    break; // 找到并删除后即可退出循环
//                }
//            }
//        }
//
//        // 3. 更新TreeView
//        //_treeView->syncViewWithModel();
//    }
//    else {
//        spdlog::warn("请求删除设备 {}，但在数据模型中未找到。", id.toStdString());
//    }
//}
//
//void DeviceWidget::on_addDeviceButton_clicked()
//{
//    //在添加新控件前，先将UI上所有未保存的修改更新到数据模型中
//    updateDeviceModelFromView();
//
//    EquipmentData newDevice;
//    newDevice.equipmentID = QString("NewDevice%1").arg(DataModel::instance()->allEquipments.size() + 1);
//    // 首先在DataModel中占位
//    DataModel::instance()->allEquipments.push_back(newDevice);
//    // 然后创建新的DeviceWidget并添加到UI
//    DeviceWidget* widget = new DeviceWidget();
//    widget->setData(newDevice); // 关联UI与数据
//    deviceLayout->addWidget(widget);
//    connect(widget, &DeviceWidget::removalRequested, this, &FleetInput::onDeviceWidgetRemovalRequested);
//    
//    //同步treeView
//    //_treeView->syncViewWithModel();
//}