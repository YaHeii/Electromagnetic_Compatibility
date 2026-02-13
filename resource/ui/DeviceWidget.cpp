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
#include "ElaScrollArea.h"
#include "ElaTheme.h"
#include "ElaScrollPageArea.h"
#include "spdlog/spdlog.h"

DeviceWidget::DeviceWidget(QWidget *parent)
    : BasePage(parent)
{
    setWindowTitle("设备属性管理");
    createCustomWidget("此页面可动态添加和管理多个可用设备");

    _deviceListLayout = new QVBoxLayout();

    AddDeviceBtn = new ElaPushButton("添加新设备", this);
    AddDeviceBtn->setFixedSize(120, 36);
    SaveEquipmentBtn = new ElaPushButton("保存所有设备", this);
    SaveEquipmentBtn->setFixedSize(120, 36);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(AddDeviceBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(SaveEquipmentBtn);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    centralWidget->setWindowTitle("设备属性管理");
    mainLayout->setContentsMargins(0, 0, 0, 0);
    _deviceListLayout->setSpacing(20); // 卡片之间的间距
    mainLayout->addLayout(_deviceListLayout);
	mainLayout->addStretch(); 
    mainLayout->addLayout(btnLayout);
	mainLayout->setStretch(0, 1); // 设备列表占满剩余空间
    addCentralWidget(centralWidget);
    connect(AddDeviceBtn, &ElaPushButton::clicked, this, &DeviceWidget::on_AddDeviceBtn_clicked);
    connect(SaveEquipmentBtn, &ElaPushButton::clicked, this, &DeviceWidget::on_SaveEquipmentBtn_clicked);
    // 连接删除按钮信号
    on_AddDeviceBtn_clicked();    
}

DeviceWidget::~DeviceWidget()
{
}



void DeviceWidget::on_AddDeviceBtn_clicked() {
    DeviceItemWidget* newItem = new DeviceItemWidget(this);   
    newItem->setMinimumHeight(300);
    // 2. 将其插入到滚动布局中 (假设 _scrollLayout 是你放置设备的布局)
    // 建议在布局最后保留一个 addStretch()，这样新条目会往上排
    _deviceListLayout->insertWidget(_deviceListLayout->count() - 1, newItem);
    
    // 3. 处理删除信号
    connect(newItem, &DeviceItemWidget::deleteMe, this, &DeviceWidget::on_RemoveItemRequested);
    spdlog::info("已添加新的设备 UI 条目");
}

void DeviceWidget::on_RemoveItemRequested(DeviceItemWidget* item)
{
    if (!item) return;
    
    // 从布局中移除并销毁
    _deviceListLayout->removeWidget(item);
    item->deleteLater();
    
    spdlog::info("已移除设备 UI 条目");
}

void DeviceWidget::on_SaveEquipmentBtn_clicked() {
    auto* model = DataModel::instance();
    // 1. 清空当前模型中的设备列表，以 UI 上的实际条目为准
    model->allEquipments.clear();

    // 2. 遍历布局，收集每个条目中的数据
    for (int i = 0; i < _deviceListLayout->count(); ++i) {
        QLayoutItem* layoutItem = _deviceListLayout->itemAt(i);
        if (auto* widget = qobject_cast<DeviceItemWidget*>(layoutItem->widget())) {
            // 获取 UI 当前的数据并验证
            EquipmentData data = widget->getData();
            
            // 基础校验（示例：ID不能为空）
            if (data.equipmentID.isEmpty()) {
                spdlog::warn("发现 ID 为空的设备，跳过保存");
                continue;
            }
            
            model->allEquipments.push_back(data);
        }
    }

    spdlog::info("设备保存成功，当前 DataModel 中共有 {} 个设备", model->allEquipments.size());
}

DeviceItemWidget::DeviceItemWidget(QWidget* parent) : ElaScrollPageArea(parent) {
 ElaText *typeText = new ElaText("设备类型", this);
    typeText->setTextPixelSize(15);
    _equipmentType = new ElaComboBox(this);
    QStringList comboList{
    "发射机",
    "接收机",
    "收发一体机"};
    _equipmentType->addItems(comboList);

    connect(_equipmentType, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &DeviceItemWidget::onEquipmentTypeChanged);
    // 增益和设备ID
    ElaText *gainText = new ElaText("发射/接收增益", this);
    gainText->setTextPixelSize(15);
    _gain = new ElaLineEdit(this);
    _gain->setPlaceholderText("dBm");
    ElaText *idText = new ElaText("设备ID", this);
    idText->setTextPixelSize(15);
    _equipmentID = new ElaLineEdit(this);
    _equipmentID->setPlaceholderText("建议设为纯数字或纯字母");

    QHBoxLayout *firstLine = new QHBoxLayout();
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
    
    _RecieverWidget = new ElaScrollPageArea(this);
    _TransmitterWidget = new ElaScrollPageArea(this);
    _RecieverWidget->setFixedHeight(360);
    _TransmitterWidget->setFixedHeight(360);
    setupReceiverUI(_RecieverWidget);
    setupTransmitterUI(_TransmitterWidget);

    ReductionEquipmentBtn = new ElaPushButton("删除此设备", this);
    connect(ReductionEquipmentBtn, &ElaPushButton::clicked, this, &DeviceItemWidget::on_ReductionBtn_clicked);
   
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 组合主布局
    mainLayout->addLayout(firstLine);
    mainLayout->addLayout(secondLine);
    mainLayout->addWidget(_TransmitterWidget);
    mainLayout->addWidget(_RecieverWidget);
    mainLayout->addWidget(ReductionEquipmentBtn);
  
    // 初始化显示状态
    onEquipmentTypeChanged(); 
}

// view->datamodel
EquipmentData DeviceItemWidget::getData() const {
    EquipmentData data;
    bool ok;
    data.equipmentID = _equipmentID->text();
    data.equipmentType = _equipmentType->currentText();
    data.Gain = _gain->text().toDouble(&ok);
    data.X_offset = _X_offset->text().toDouble(&ok);
    data.Y_offset = _Y_offset->text().toDouble(&ok);
    data.Z_offset = _Z_offset->text().toDouble(&ok);

    if (_TransmitterWidget->isVisible()) {
        data.CentralF_Transmitter = _CentralF_Transmitter->text().toDouble(&ok);
        data.Bandwidth_Transmitter = _Bandwidth_Transmitter->text().toDouble(&ok);
        data.Power_Transmitter = _Power_Transmitter->text().toDouble(&ok);
        data.antennaPhi_Transmitter = _antennaPhi_Transmitter->text().toDouble(&ok);
        data.Beamwidth_Transmitter = _Beamwidth_Transmitter->text().toDouble(&ok);
        data.PolarizationMethod_Transmitter = _PolarizationMethod_Transmitter->currentText();
        data.antennaType_Transmitter = _antennaType_Transmitter->currentText();
    }

    if (_RecieverWidget->isVisible()) {
        data.CentralF_Reciever = _CentralF_Receiver->text().toDouble(&ok);
        data.Bandwidth_Reciever = _Bandwidth_Receiver->text().toDouble(&ok);
        data.Sensitive_reciever = _Sensitive_Receiver->text().toDouble(&ok);
        data.interferenceMargin = _InterferenceMargin_Receiver->text().toDouble(&ok);
        data.SINRMargin = _SINRMargin_Receiver->text().toDouble(&ok);
        data.noiseFigure = _NoiseFigure_Receiver->text().toDouble(&ok);
    }
    spdlog::info("设备 {} 参数已经保存", data.equipmentID.toStdString());
    return data;
}

// datamodel->view
void DeviceItemWidget::setData(const EquipmentData &data)
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
    if(_RecieverWidget->isVisible() ){
    _CentralF_Receiver->setText(QString::number(data.CentralF_Reciever));
    _Bandwidth_Receiver->setText(QString::number(data.Bandwidth_Reciever));
    _Sensitive_Receiver->setText(QString::number(data.Sensitive_reciever));
    _InterferenceMargin_Receiver->setText(QString::number(data.interferenceMargin));
    _SINRMargin_Receiver->setText(QString::number(data.SINRMargin));
    _NoiseFigure_Receiver->setText(QString::number(data.noiseFigure));
    } else if(_TransmitterWidget->isVisible()){
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

void DeviceItemWidget::onEquipmentTypeChanged() {
    QString type = _equipmentType->currentText();
    bool isTrans = (type == "发射机" || type == "收发一体机");
    bool isRecv = (type == "接收机" || type == "收发一体机");
    
    _TransmitterWidget->setVisible(isTrans);
    _RecieverWidget->setVisible(isRecv);
}

void DeviceItemWidget::on_ReductionBtn_clicked() {
    emit deleteMe(this); // 发送删除信号给父窗口
}


void DeviceItemWidget::resetTransmitterUI() {
    _CentralF_Transmitter->setText("0");
    _Bandwidth_Transmitter->setText("0");
    _Power_Transmitter->setText("0");
    _antennaPhi_Transmitter->setText("0");
    _Beamwidth_Transmitter->setText("0");
    // 下拉框可以重置到默认索引0
    _PolarizationMethod_Transmitter->setCurrentIndex(0);
    _antennaType_Transmitter->setCurrentIndex(0);
}

void DeviceItemWidget::resetReceiverUI() {
    _CentralF_Receiver->setText("0");
    _Bandwidth_Receiver->setText("0");
    _Sensitive_Receiver->setText("0");
    _InterferenceMargin_Receiver->setText("0");
    _SINRMargin_Receiver->setText("0");
    _NoiseFigure_Receiver->setText("0");
}

void DeviceItemWidget::setupReceiverUI(ElaScrollPageArea* container)
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

void DeviceItemWidget::setupTransmitterUI(ElaScrollPageArea* container)
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