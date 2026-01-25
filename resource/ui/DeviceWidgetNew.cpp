#include "DeviceWidgetNew.h"
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
#include "ElaMessageBar.h"

DeviceWidgetNew::DeviceWidgetNew(QWidget *parent) :
    QWidget(parent)
{
    setupUI();
    resetTransmitterUI();
    resetReceiverUI();
    
    // 当设备类型改变时，自动填充默认参数
    connect(equipmentType, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &DeviceWidgetNew::onEquipmentTypeChanged);
        
    // 主题切换支持
    connect(eTheme, &ElaTheme::themeModeChanged, this, [=]() {
        update();
    });
}

void DeviceWidgetNew::setupUI()
{
    // 设置窗口属性 - 采用ElaWidget的设计模式
    setMinimumSize(320, 480);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setWindowTitle("设备配置");
    
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);
    
    // 创建标题区域 - 仿照T_BasePage的createCustomWidget模式
    setupTitleWidget();
    
    // 创建各个区域
    setupBaseWidget();
    setupReceiverWidget();
    setupTransmitterWidget();
    
    // 创建操作按钮区域
    setupActionButtons();
    
    // 添加到主布局
    mainLayout->addWidget(titleWidget);
    mainLayout->addWidget(BaseWidget);
    mainLayout->addWidget(RecieverWidget);
    mainLayout->addWidget(TransmitterWidget);
    mainLayout->addWidget(actionWidget);
    mainLayout->addStretch();
}

void DeviceWidgetNew::setupTitleWidget()
{
    titleWidget = new QWidget(this);
    titleWidget->setFixedHeight(60);
    
    QVBoxLayout *titleLayout = new QVBoxLayout(titleWidget);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(5);
    
    // 标题文本
    ElaText *titleText = new ElaText("设备配置", this);
    titleText->setTextPixelSize(18);
    // titleText->setTextBold(true);
    
    // 描述文本
    ElaText *descText = new ElaText("配置设备参数和属性", this);
    descText->setTextPixelSize(12);
    // descText->setTextColor(QColor(128, 128, 128));
    
    titleLayout->addWidget(titleText);
    titleLayout->addWidget(descText);
    titleLayout->addStretch();
}

void DeviceWidgetNew::setupActionButtons()
{
    actionWidget = new QWidget(this);
    actionWidget->setFixedHeight(50);
    
    QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
    actionLayout->setContentsMargins(0, 10, 0, 0);
    
    // 保存按钮
    ElaPushButton *saveButton = new ElaPushButton("保存配置", this);
    saveButton->setFixedSize(100, 35);
    // saveButton->setElaIcon(ElaIconType::Sailboat);
    connect(saveButton, &ElaPushButton::clicked, this, [=]() {
        updateModelData();
        ElaMessageBar::success(ElaMessageBarType::BottomRight, "成功", "配置已保存", 2000);
    });
    
    // 删除按钮
    equipmentReduction = new ElaPushButton("删除设备", this);
    equipmentReduction->setFixedSize(100, 35);
    // equipmentReduction->setElaIcon(ElaIconType::Sailboat);
    // equipmentReduction->setIsTransparent(false);
    
    actionLayout->addStretch();
    actionLayout->addWidget(saveButton);
    actionLayout->addSpacing(10);
    actionLayout->addWidget(equipmentReduction);
    
    // 连接删除按钮信号
    connect(equipmentReduction, &ElaPushButton::clicked,
            this, &DeviceWidgetNew::on_equipmentReduction_clicked);
}

void DeviceWidgetNew::setupBaseWidget()
{
    BaseWidget = new ElaScrollPageArea(this);
    BaseWidget->setFixedHeight(140);
    BaseWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    QVBoxLayout *baseLayout = new QVBoxLayout(BaseWidget);
    baseLayout->setContentsMargins(10, 10, 10, 10);
    baseLayout->setSpacing(8);
    
    // 区域标题
    ElaText *baseTitle = new ElaText("基础信息", this);
    baseTitle->setTextPixelSize(14);
    // baseTitle->setTextBold(true);
    baseLayout->addWidget(baseTitle);
    
    // 设备类型选择
    QHBoxLayout *typeLayout = new QHBoxLayout();
    ElaText *typeLabel = new ElaText("设备类型", this);
    typeLabel->setTextPixelSize(12);
    typeLabel->setFixedWidth(80);
    
    equipmentType = new ElaComboBox(this);
    equipmentType->addItem("天线");
    equipmentType->addItem("发射机");
    equipmentType->addItem("接收机");
    equipmentType->addItem("收发一体机");
    equipmentType->setFixedHeight(30);
    
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(equipmentType);
    baseLayout->addLayout(typeLayout);
    
    // 增益和设备ID
    QHBoxLayout *gainIdLayout = new QHBoxLayout();
    ElaText *gainLabel = new ElaText("发射/接收增益", this);
    gainLabel->setTextPixelSize(12);
    gainLabel->setFixedWidth(80);
    Gain = new ElaLineEdit(this);
    Gain->setPlaceholderText("dBm");
    Gain->setFixedHeight(30);
    
    ElaText *idLabel = new ElaText("设备ID", this);
    idLabel->setTextPixelSize(12);
    idLabel->setFixedWidth(60);
    equipmentID = new ElaLineEdit(this);
    equipmentID->setFixedHeight(30);
    
    gainIdLayout->addWidget(gainLabel);
    gainIdLayout->addWidget(Gain);
    gainIdLayout->addSpacing(10);
    gainIdLayout->addWidget(idLabel);
    gainIdLayout->addWidget(equipmentID);
    baseLayout->addLayout(gainIdLayout);
    
    // 坐标输入
    QHBoxLayout *coordLayout = new QHBoxLayout();
    ElaText *coordLabel = new ElaText("坐标位置", this);
    coordLabel->setTextPixelSize(12);
    coordLabel->setFixedWidth(80);
    
    X_offset = new ElaLineEdit(this);
    X_offset->setPlaceholderText("X");
    X_offset->setFixedHeight(30);
    X_offset->setFixedWidth(60);
    
    Y_offset = new ElaLineEdit(this);
    Y_offset->setPlaceholderText("Y");
    Y_offset->setFixedHeight(30);
    Y_offset->setFixedWidth(60);
    
    Z_offset = new ElaLineEdit(this);
    Z_offset->setPlaceholderText("Z");
    Z_offset->setFixedHeight(30);
    Z_offset->setFixedWidth(60);
    
    coordLayout->addWidget(coordLabel);
    coordLayout->addWidget(X_offset);
    coordLayout->addWidget(Y_offset);
    coordLayout->addWidget(Z_offset);
    coordLayout->addStretch();
    baseLayout->addLayout(coordLayout);
}

void DeviceWidgetNew::setupReceiverWidget()
{
    RecieverWidget = new ElaScrollPageArea(this);
    RecieverWidget->setFixedHeight(120);
    RecieverWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    
    QVBoxLayout *receiverLayout = new QVBoxLayout(RecieverWidget);
    receiverLayout->setContentsMargins(10, 10, 10, 10);
    receiverLayout->setSpacing(8);
    
    // 区域标题
    ElaText *receiverTitle = new ElaText("接收机参数", this);
    receiverTitle->setTextPixelSize(14);
    // receiverTitle->setTextBold(true);
    receiverLayout->addWidget(receiverTitle);
    
    // 参数网格
    QGridLayout *paramGrid = new QGridLayout();
    paramGrid->setSpacing(8);
    
    // 第一行：中心频率、接收带宽
    ElaText *centralFLabel = new ElaText("中心频率", this);
    centralFLabel->setTextPixelSize(12);
    CentralF_Reciever = new ElaLineEdit(this);
    CentralF_Reciever->setPlaceholderText("Hz");
    CentralF_Reciever->setFixedHeight(30);
    
    ElaText *bandwidthLabel = new ElaText("接收带宽", this);
    bandwidthLabel->setTextPixelSize(12);
    Bandwidth_Reciever = new ElaLineEdit(this);
    Bandwidth_Reciever->setPlaceholderText("Hz");
    Bandwidth_Reciever->setFixedHeight(30);
    
    paramGrid->addWidget(centralFLabel, 0, 0);
    paramGrid->addWidget(CentralF_Reciever, 0, 1);
    paramGrid->addWidget(bandwidthLabel, 0, 2);
    paramGrid->addWidget(Bandwidth_Reciever, 0, 3);
    
    // 第二行：接收机灵敏度、干扰阈值
    ElaText *sensitiveLabel = new ElaText("接收机灵敏度", this);
    sensitiveLabel->setTextPixelSize(12);
    Sensitive_reciever = new ElaLineEdit(this);
    Sensitive_reciever->setPlaceholderText("dBm");
    Sensitive_reciever->setFixedHeight(30);
    
    ElaText *interferenceLabel = new ElaText("干扰阈值", this);
    interferenceLabel->setTextPixelSize(12);
    interferenceMargin = new ElaLineEdit(this);
    interferenceMargin->setPlaceholderText("dBm");
    interferenceMargin->setFixedHeight(30);
    
    paramGrid->addWidget(sensitiveLabel, 1, 0);
    paramGrid->addWidget(Sensitive_reciever, 1, 1);
    paramGrid->addWidget(interferenceLabel, 1, 2);
    paramGrid->addWidget(interferenceMargin, 1, 3);
    
    receiverLayout->addLayout(paramGrid);
}

void DeviceWidgetNew::setupTransmitterWidget()
{
    TransmitterWidget = new ElaScrollPageArea(this);
    TransmitterWidget->setFixedHeight(140);
    TransmitterWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    
    QVBoxLayout *transmitterLayout = new QVBoxLayout(TransmitterWidget);
    transmitterLayout->setContentsMargins(10, 10, 10, 10);
    transmitterLayout->setSpacing(8);
    
    // 区域标题
    ElaText *transmitterTitle = new ElaText("发射机参数", this);
    transmitterTitle->setTextPixelSize(14);
    // transmitterTitle->setTextBold(true);
    transmitterLayout->addWidget(transmitterTitle);
    
    // 第一行：中心频率、发射带宽、发射功率
    QHBoxLayout *freqPowerLayout = new QHBoxLayout();
    freqPowerLayout->setSpacing(8);
    
    ElaText *centralFLabel = new ElaText("中心频率", this);
    centralFLabel->setTextPixelSize(12);
    centralFLabel->setFixedWidth(80);
    CentralF_Transmitter = new ElaLineEdit(this);
    CentralF_Transmitter->setPlaceholderText("Hz");
    CentralF_Transmitter->setFixedHeight(30);
    
    ElaText *bandwidthLabel = new ElaText("发射带宽", this);
    bandwidthLabel->setTextPixelSize(12);
    bandwidthLabel->setFixedWidth(80);
    Bandwidth_Transmitter = new ElaLineEdit(this);
    Bandwidth_Transmitter->setPlaceholderText("Hz");
    Bandwidth_Transmitter->setFixedHeight(30);
    
    ElaText *powerLabel = new ElaText("发射功率", this);
    powerLabel->setTextPixelSize(12);
    powerLabel->setFixedWidth(80);
    Power_Transmitter = new ElaLineEdit(this);
    Power_Transmitter->setPlaceholderText("dBm");
    Power_Transmitter->setFixedHeight(30);
    
    freqPowerLayout->addWidget(centralFLabel);
    freqPowerLayout->addWidget(CentralF_Transmitter);
    freqPowerLayout->addWidget(bandwidthLabel);
    freqPowerLayout->addWidget(Bandwidth_Transmitter);
    freqPowerLayout->addWidget(powerLabel);
    freqPowerLayout->addWidget(Power_Transmitter);
    
    transmitterLayout->addLayout(freqPowerLayout);
    
    // 第二行：天线参数
    QHBoxLayout *antennaLayout = new QHBoxLayout();
    antennaLayout->setSpacing(8);
    
    ElaText *phiLabel = new ElaText("天线指向角", this);
    phiLabel->setTextPixelSize(12);
    phiLabel->setFixedWidth(80);
    antennaPhi_Transmitter = new ElaLineEdit(this);
    antennaPhi_Transmitter->setPlaceholderText("度");
    antennaPhi_Transmitter->setFixedHeight(30);
    
    ElaText *beamwidthLabel = new ElaText("波束宽度", this);
    beamwidthLabel->setTextPixelSize(12);
    beamwidthLabel->setFixedWidth(80);
    Beamwidth_Transmitter = new ElaLineEdit(this);
    Beamwidth_Transmitter->setPlaceholderText("度");
    Beamwidth_Transmitter->setFixedHeight(30);
    
    ElaText *polarizationLabel = new ElaText("极化方式", this);
    polarizationLabel->setTextPixelSize(12);
    polarizationLabel->setFixedWidth(80);
    PolarizationMethod_Transmitter = new ElaComboBox(this);
    PolarizationMethod_Transmitter->addItem("垂直极化");
    PolarizationMethod_Transmitter->addItem("水平极化");
    PolarizationMethod_Transmitter->setFixedHeight(30);
    
    antennaLayout->addWidget(phiLabel);
    antennaLayout->addWidget(antennaPhi_Transmitter);
    antennaLayout->addWidget(beamwidthLabel);
    antennaLayout->addWidget(Beamwidth_Transmitter);
    antennaLayout->addWidget(polarizationLabel);
    antennaLayout->addWidget(PolarizationMethod_Transmitter);
    
    transmitterLayout->addLayout(antennaLayout);
}

DeviceWidgetNew::~DeviceWidgetNew()
{
    // Qt会自动删除子控件，无需手动删除
}

void DeviceWidgetNew::setData(const EquipmentData &data)
{
    m_currentId = data.equipmentID;

    // --- 1. 基本参数 ---
    equipmentID->setText(data.equipmentID);
    equipmentType->setCurrentText(data.equipmentType);
    Gain->setText(QString::number(data.Gain));
    
    X_offset->setText(QString::number(data.X_offset));
    Y_offset->setText(QString::number(data.Y_offset));
    Z_offset->setText(QString::number(data.Z_offset));

    // --- 2. 接收机参数 ---
    CentralF_Reciever->setText(QString::number(data.CentralF_Reciever));
    Bandwidth_Reciever->setText(QString::number(data.Bandwidth_Reciever));
    Sensitive_reciever->setText(QString::number(data.Sensitive_reciever));
    interferenceMargin->setText(QString::number(data.interferenceMargin));
    SINRMargin->setText(QString::number(data.SINRMargin));
    noiseFigure->setText(QString::number(data.noiseFigure));

    // --- 3. 发射机参数 ---
    CentralF_Transmitter->setText(QString::number(data.CentralF_Transmitter));
    Bandwidth_Transmitter->setText(QString::number(data.Bandwidth_Transmitter));
    Power_Transmitter->setText(QString::number(data.Power_Transmitter));
    antennaPhi_Transmitter->setText(QString::number(data.antennaPhi_Transmitter));
    Beamwidth_Transmitter->setText(QString::number(data.Beamwidth_Transmitter));
    PolarizationMethod_Transmitter->setCurrentText(data.PolarizationMethod_Transmitter);
    antennaType_Transmitter->setCurrentText(data.antennaType_Transmitter);
}

void DeviceWidgetNew::updateModelData() {
    bool ok;
    // 遍历全局数据列表找到当前设备
    for (EquipmentData &data : DataModel::instance()->allEquipments) {
        if(data.equipmentID == m_currentId){
            // --- 公共参数总是保存 ---
            data.equipmentID = equipmentID->text();
            data.equipmentType = equipmentType->currentText();
            data.Gain = Gain->text().toDouble(&ok);
            data.X_offset = X_offset->text().toDouble(&ok);
            data.Y_offset = Y_offset->text().toDouble(&ok);
            data.Z_offset = Z_offset->text().toDouble(&ok);

            // --- 发射机参数处理 ---
            // 判断发射机容器是否可见
            if (TransmitterWidget->isVisible()) {
                data.CentralF_Transmitter = CentralF_Transmitter->text().toDouble(&ok);
                data.Bandwidth_Transmitter = Bandwidth_Transmitter->text().toDouble(&ok);
                data.Power_Transmitter = Power_Transmitter->text().toDouble(&ok);
                data.antennaPhi_Transmitter = antennaPhi_Transmitter->text().toDouble(&ok);
                data.Beamwidth_Transmitter = Beamwidth_Transmitter->text().toDouble(&ok);
                data.PolarizationMethod_Transmitter = PolarizationMethod_Transmitter->currentText();
                data.antennaType_Transmitter = antennaType_Transmitter->currentText();
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
            if (RecieverWidget->isVisible()) {
                data.CentralF_Reciever = CentralF_Reciever->text().toDouble(&ok);
                data.Bandwidth_Reciever = Bandwidth_Reciever->text().toDouble(&ok);
                data.Sensitive_reciever = Sensitive_reciever->text().toDouble(&ok);
                data.interferenceMargin = interferenceMargin->text().toDouble(&ok);
                data.SINRMargin = SINRMargin->text().toDouble(&ok);
                data.noiseFigure = noiseFigure->text().toDouble(&ok);
            } else {
                data.CentralF_Reciever = 0;
                data.Bandwidth_Reciever = 0;
                data.Sensitive_reciever = 0;
                data.interferenceMargin = 0;
                data.SINRMargin = 0;
                data.noiseFigure = 0;
            }
            spdlog::debug("设备 {} 参数已经保存", data.equipmentID.toStdString());
            break;
        }
    }
}

void DeviceWidgetNew::onEquipmentTypeChanged()
{
    QString type = equipmentType->currentText();
    // 设置基础参数
    Gain->setText("15");
    X_offset->setText("0");
    Y_offset->setText("0");
    Z_offset->setText("0");

    if (type == "发射机") {
        TransmitterWidget->setVisible(true);
        RecieverWidget->setVisible(false);

        // 设置发射机默认参数
        CentralF_Transmitter->setText("1000");
        Bandwidth_Transmitter->setText("100");
        Power_Transmitter->setText("20");
        antennaPhi_Transmitter->setText("30");
        Beamwidth_Transmitter->setText("20");
        PolarizationMethod_Transmitter->setCurrentIndex(0);
        // 清空其他参数
        resetReceiverUI();
        spdlog::debug("正在设定{}参数", type.toStdString());
    }
    else if (type == "接收机") {
        TransmitterWidget->setVisible(false);
        RecieverWidget->setVisible(true);

        // 设置接收机默认参数
        CentralF_Reciever->setText("1000");
        Bandwidth_Reciever->setText("100");
        Sensitive_reciever->setText("-90");
        interferenceMargin->setText("6");
        SINRMargin->setText("10");
        noiseFigure->setText("3");

        // 清空其他参数
        resetTransmitterUI();
        spdlog::debug("正在设定{}参数", type.toStdString());
    }
    else if (type == "收发一体机") {
        TransmitterWidget->setVisible(true);
        RecieverWidget->setVisible(true);

        CentralF_Transmitter->setText("1000");
        Bandwidth_Transmitter->setText("100");
        Power_Transmitter->setText("20");
        antennaPhi_Transmitter->setText("30");
        Beamwidth_Transmitter->setText("20");
        PolarizationMethod_Transmitter->setCurrentIndex(0);
        
        CentralF_Reciever->setText("1000");
        Bandwidth_Reciever->setText("100");
        Sensitive_reciever->setText("-90");
        interferenceMargin->setText("6");
        SINRMargin->setText("10");
        noiseFigure->setText("3");

        spdlog::debug("正在设定{}参数", type.toStdString());
    }
}

void DeviceWidgetNew::on_equipmentReduction_clicked()
{
    emit removalRequested(m_currentId);
}

void DeviceWidgetNew::resetTransmitterUI() {
    CentralF_Transmitter->setText("0");
    Bandwidth_Transmitter->setText("0");
    Power_Transmitter->setText("0");
    antennaPhi_Transmitter->setText("0");
    Beamwidth_Transmitter->setText("0");
    // 下拉框可以重置到默认索引0
    PolarizationMethod_Transmitter->setCurrentIndex(0);
    antennaType_Transmitter->setCurrentIndex(0);
}

void DeviceWidgetNew::resetReceiverUI() {
    CentralF_Reciever->setText("0");
    Bandwidth_Reciever->setText("0");
    Sensitive_reciever->setText("0");
    interferenceMargin->setText("0");
    SINRMargin->setText("0");
    noiseFigure->setText("0");
}