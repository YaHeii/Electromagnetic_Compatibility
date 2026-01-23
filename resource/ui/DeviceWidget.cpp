#include "DeviceWidget.h"
#include <QDebug>
#include "spdlog/spdlog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "ElaText.h"
#include <QGridLayout>
#include <ElaPushButton.h>
#include <ElaComboBox.h>
#include <ElaLineEdit.h>
#include <ElaScrollPageArea.h>

DeviceWidget::DeviceWidget(QWidget *parent) :
    QWidget(parent)
{
    setupUI();
    resetTransmitterUI();
    resetReceiverUI();
    
    // 当设备类型改变时，自动填充默认参数
    connect(equipmentType, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &DeviceWidget::onEquipmentTypeChanged);
}
void DeviceWidget::setupUI()
{
    // 设置窗口属性
    setMinimumSize(300, 450);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setWindowTitle("DeviceWidget");
    
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建各个区域
    setupBaseWidget();
    setupReceiverWidget();
    setupTransmitterWidget();
    
    // 创建删除按钮
    equipmentReduction = new ElaPushButton("删除设备", this);
    
    // 添加到主布局
    mainLayout->addWidget(BaseWidget);
    mainLayout->addWidget(RecieverWidget);
    mainLayout->addWidget(TransmitterWidget);
    
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setContentsMargins(10, 10, 10, 10);
    buttonLayout->addWidget(equipmentReduction);
    mainLayout->addLayout(buttonLayout);
    
    // 连接删除按钮信号
    connect(equipmentReduction, &ElaPushButton::clicked,
            this, &DeviceWidget::on_equipmentReduction_clicked);
}

void DeviceWidget::setupBaseWidget()
{
    BaseWidget = new ElaScrollPageArea(this);
    BaseWidget->setMinimumSize(0, 120);
    BaseWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    QVBoxLayout *baseLayout = new QVBoxLayout(BaseWidget);
    baseLayout->setContentsMargins(2, 2, 2, 2);
    
    // 设备类型选择
    QHBoxLayout *typeLayout = new QHBoxLayout();
    ElaText *typeLabel = new ElaText("设备类型", 13, this);
    equipmentType = new ElaComboBox(this);
    equipmentType->addItem("天线");
    equipmentType->addItem("发射机");
    equipmentType->addItem("接收机");
    equipmentType->addItem("收发一体机");
    
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(equipmentType);
    baseLayout->addLayout(typeLayout);
    
    // 增益和设备ID
    QHBoxLayout *gainIdLayout = new QHBoxLayout();
    ElaText *gainLabel = new ElaText("发射/接收增益", 13, this);
    Gain = new ElaLineEdit(this);
    Gain->setPlaceholderText("dBm");
    
    ElaText *idLabel = new ElaText("设备ID", 13, this);
    equipmentID = new ElaLineEdit(this);
    
    gainIdLayout->addWidget(gainLabel);
    gainIdLayout->addWidget(Gain);
    gainIdLayout->addWidget(idLabel);
    gainIdLayout->addWidget(equipmentID);
    baseLayout->addLayout(gainIdLayout);
    
    // 坐标输入
    QHBoxLayout *coordLayout = new QHBoxLayout();
    ElaText *xLabel = new ElaText("X坐标", 13, this);
    X_offset = new ElaLineEdit(this);
    X_offset->setPlaceholderText("X坐标");
    
    ElaText *yLabel = new ElaText("Y坐标", 13, this);
    Y_offset = new ElaLineEdit(this);
    Y_offset->setPlaceholderText("Y坐标");
    
    ElaText *zLabel = new ElaText("Z坐标", 13, this);
    Z_offset = new ElaLineEdit(this);
    Z_offset->setPlaceholderText("Z坐标");
    
    coordLayout->addWidget(xLabel);
    coordLayout->addWidget(X_offset);
    coordLayout->addWidget(yLabel);
    coordLayout->addWidget(Y_offset);
    coordLayout->addWidget(zLabel);
    coordLayout->addWidget(Z_offset);
    baseLayout->addLayout(coordLayout);
}

void DeviceWidget::setupReceiverWidget()
{
    RecieverWidget = new ElaScrollPageArea(this);
    RecieverWidget->setMinimumSize(0, 120);
    RecieverWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    
    QGridLayout *receiverLayout = new QGridLayout(RecieverWidget);
    receiverLayout->setContentsMargins(2, 2, 2, 2);
    
    // 第一行：中心频率、接收带宽
    ElaText *centralFLabel = new ElaText("中心频率",13, this);
    CentralF_Reciever = new ElaLineEdit(this);
    CentralF_Reciever->setPlaceholderText("Hz");
    
    ElaText *bandwidthLabel = new ElaText("接收带宽", 13, this);
    Bandwidth_Reciever = new ElaLineEdit(this);
    Bandwidth_Reciever->setPlaceholderText("Hz");
    
    receiverLayout->addWidget(centralFLabel, 0, 0);
    receiverLayout->addWidget(CentralF_Reciever, 0, 1);
    receiverLayout->addWidget(bandwidthLabel, 0, 2);
    receiverLayout->addWidget(Bandwidth_Reciever, 0, 3);
    
    // 第二行：接收机灵敏度、干扰阈值
    ElaText *sensitiveLabel = new ElaText("接收机灵敏度", 13, this);
    Sensitive_reciever = new ElaLineEdit(this);
    Sensitive_reciever->setPlaceholderText("dBm");
    
    ElaText *interferenceLabel = new ElaText("干扰阈值", 13, this);
    interferenceMargin = new ElaLineEdit(this);
    interferenceMargin->setPlaceholderText("dBm");
    
    receiverLayout->addWidget(sensitiveLabel, 1, 0);
    receiverLayout->addWidget(Sensitive_reciever, 1, 1);
    receiverLayout->addWidget(interferenceLabel, 1, 2);
    receiverLayout->addWidget(interferenceMargin, 1, 3);
    
    // 第三行：信噪比阈值、噪声系数
    ElaText *sinrLabel = new ElaText("信噪比阈值", 13, this);
    SINRMargin = new ElaLineEdit(this);
    SINRMargin->setPlaceholderText("dBm");
    
    ElaText *noiseLabel = new ElaText("噪声系数", 13, this);
    noiseFigure = new ElaLineEdit(this);
    noiseFigure->setPlaceholderText("dBm");
    
    receiverLayout->addWidget(sinrLabel, 2, 0);
    receiverLayout->addWidget(SINRMargin, 2, 1);
    receiverLayout->addWidget(noiseLabel, 2, 2);
    receiverLayout->addWidget(noiseFigure, 2, 3);
}

void DeviceWidget::setupTransmitterWidget()
{
    TransmitterWidget = new ElaScrollPageArea(this);
    TransmitterWidget->setMinimumSize(0, 120);
    TransmitterWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    
    QVBoxLayout *transmitterLayout = new QVBoxLayout(TransmitterWidget);
    transmitterLayout->setSpacing(0);
    transmitterLayout->setContentsMargins(2, 2, 2, 5);
    
    // 第一行：中心频率、发射带宽、发射功率
    QHBoxLayout *freqPowerLayout = new QHBoxLayout();
    ElaText *centralFLabel = new ElaText("中心频率", 13, this);
    CentralF_Transmitter = new ElaLineEdit(this);
    CentralF_Transmitter->setPlaceholderText("dBm");
    
    ElaText *bandwidthLabel = new ElaText("发射带宽", 13, this);
    Bandwidth_Transmitter = new ElaLineEdit(this);
    Bandwidth_Transmitter->setPlaceholderText("dBm");
    
    ElaText *powerLabel = new ElaText("发射功率", 13, this);
    Power_Transmitter = new ElaLineEdit(this);
    Power_Transmitter->setPlaceholderText("dBm");
    
    freqPowerLayout->addWidget(centralFLabel);
    freqPowerLayout->addWidget(CentralF_Transmitter);
    freqPowerLayout->addWidget(bandwidthLabel);
    freqPowerLayout->addWidget(Bandwidth_Transmitter);
    freqPowerLayout->addWidget(powerLabel);
    freqPowerLayout->addWidget(Power_Transmitter);
    transmitterLayout->addLayout(freqPowerLayout);
    
    // 第二行：天线参数网格
    QGridLayout *antennaLayout = new QGridLayout();
    
    // 天线指向角、波束宽度
    ElaText *phiLabel = new ElaText("天线指向角", 13, this);
    antennaPhi_Transmitter = new ElaLineEdit(this);
    antennaPhi_Transmitter->setPlaceholderText("dBm");
    
    ElaText *beamwidthLabel = new ElaText("波束宽度", 13, this);
    Beamwidth_Transmitter = new ElaLineEdit(this);
    Beamwidth_Transmitter->setPlaceholderText("dBm");
    
    antennaLayout->addWidget(phiLabel, 0, 0);
    antennaLayout->addWidget(antennaPhi_Transmitter, 0, 1);
    antennaLayout->addWidget(beamwidthLabel, 0, 2);
    antennaLayout->addWidget(Beamwidth_Transmitter, 0, 3);
    
    // 极化方式、天线类型
    ElaText *polarizationLabel = new ElaText("极化方式", 13, this);
    PolarizationMethod_Transmitter = new ElaComboBox(this);
    PolarizationMethod_Transmitter->addItem("垂直极化");
    PolarizationMethod_Transmitter->addItem("水平极化");
    
    ElaText *antennaTypeLabel = new ElaText("天线类型", 13, this);
    antennaType_Transmitter = new ElaComboBox(this);
    antennaType_Transmitter->addItem("喇叭天线");
    antennaType_Transmitter->addItem("赋型波束天线");
    antennaType_Transmitter->addItem("抛物面天线");
    
    antennaLayout->addWidget(polarizationLabel, 1, 0);
    antennaLayout->addWidget(PolarizationMethod_Transmitter, 1, 1);
    antennaLayout->addWidget(antennaTypeLabel, 1, 2);
    antennaLayout->addWidget(antennaType_Transmitter, 1, 3);
    
    transmitterLayout->addLayout(antennaLayout);
}

DeviceWidget::~DeviceWidget()
{
    // Qt会自动删除子控件，无需手动删除
}

void DeviceWidget::setData(const EquipmentData &data)
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

void DeviceWidget::updateModelData() {
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

void DeviceWidget::onEquipmentTypeChanged()
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



void DeviceWidget::on_equipmentReduction_clicked()
{
    emit removalRequested(m_currentId);
}

void DeviceWidget::resetTransmitterUI() {
    CentralF_Transmitter->setText("0");
    Bandwidth_Transmitter->setText("0");
    Power_Transmitter->setText("0");
    antennaPhi_Transmitter->setText("0");
    Beamwidth_Transmitter->setText("0");
    // 下拉框可以重置到默认索引0
    PolarizationMethod_Transmitter->setCurrentIndex(0);
    antennaType_Transmitter->setCurrentIndex(0);
}

void DeviceWidget::resetReceiverUI() {
    CentralF_Reciever->setText("0");
    Bandwidth_Reciever->setText("0");
    Sensitive_reciever->setText("0");
    interferenceMargin->setText("0");
    SINRMargin->setText("0");
    noiseFigure->setText("0");
}
