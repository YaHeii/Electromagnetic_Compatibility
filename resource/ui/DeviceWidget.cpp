#include "DeviceWidget.h"
#include <QDebug>

DeviceWidget::DeviceWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceWidget)
{
    ui->setupUi(this);
    // 当设备类型改变时，自动填充默认参数
    connect(ui->equipmentType, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &DeviceWidget::onEquipmentTypeChanged);
}
DeviceWidget::~DeviceWidget()
{
    delete ui;
}

void DeviceWidget::setData(const DeviceData &data)
{
    m_currentId = data.equipmentID;

    // --- 1. 基本参数 ---
    ui->equipmentID->setText(data.equipmentID);
    ui->equipmentType->setCurrentText(data.equipmentType);
    ui->Gain->setText(QString::number(data.Gain));
    
    ui->X_offset->setText(QString::number(data.X_offset));
    ui->Y_offset->setText(QString::number(data.Y_offset));
    ui->Z_offset->setText(QString::number(data.Z_offset));

    // --- 2. 接收机参数 ---
    ui->CentralF_Reciever->setText(QString::number(data.CentralF_Reciever));
    ui->Bandwidth_Reciever->setText(QString::number(data.Bandwidth_Reciever));
    ui->Sensitive_reciever->setText(QString::number(data.Sensitive_reciever));
    ui->interferenceMargin->setText(QString::number(data.interferenceMargin));
    ui->SNRMargin->setText(QString::number(data.SNRMargin));
    ui->noiseFigure->setText(QString::number(data.noiseFigure));

    // --- 3. 发射机参数 ---
    ui->CentralF_Transmitter->setText(QString::number(data.CentralF_Transmitter));
    ui->Bandwidth_Transmitter->setText(QString::number(data.Bandwidth_Transmitter));
    ui->Power_Transmitter->setText(QString::number(data.Power_Transmitter));
    ui->antennaPhi_Transmitter->setText(QString::number(data.antennaPhi_Transmitter));
    ui->Beamwidth_Transmitter->setText(QString::number(data.Beamwidth_Transmitter));
    ui->PolarizationMethod_Transmitter->setCurrentText(data.PolarizationMethod_Transmitter);
    ui->VerticalFieldDistribution_Transmitter->setCurrentText(data.VerticalFieldDistribution_Transmitter);

    // --- 4. 天线参数 ---
    ui->CentralF_Antenna->setText(QString::number(data.CentralF_Antenna));
    ui->Bandwidth_Antenna->setText(QString::number(data.Bandwidth_Antenna));
    ui->Power_Antenna->setText(QString::number(data.Power_Antenna));
    ui->antennaPhi_Antenna->setText(QString::number(data.antennaPhi_Antenna));
    ui->Beamwidth_Antenna->setText(QString::number(data.Beamwidth_Antenna));
    ui->PolarizationMethod_Antenna->setCurrentText(data.PolarizationMethod_Antenna);
    ui->VerticalFieldDistribution_Antenna->setCurrentText(data.VerticalFieldDistribution_Antenna);
}

void DeviceWidget::updateModelData() {
    bool ok;
    // 遍历全局数据列表找到当前设备
    for (DeviceData &data : DataModel::instance()->allDevices) {
if(data.equipmentID == m_currentId){
            
            // --- 公共参数总是保存 ---
            data.equipmentID = ui->equipmentID->text();
            data.equipmentType = ui->equipmentType->currentText();
            data.Gain = ui->Gain->text().toDouble(&ok);
            data.X_offset = ui->X_offset->text().toDouble(&ok);
            data.Y_offset = ui->Y_offset->text().toDouble(&ok);
            data.Z_offset = ui->Z_offset->text().toDouble(&ok);

            // --- 发射机参数处理 ---
            // 判断发射机容器是否可见
            if (ui->TransmitterWidget->isVisible()) {
                data.CentralF_Transmitter = ui->CentralF_Transmitter->text().toDouble(&ok);
                data.Bandwidth_Transmitter = ui->Bandwidth_Transmitter->text().toDouble(&ok);
                data.Power_Transmitter = ui->Power_Transmitter->text().toDouble(&ok);
                data.antennaPhi_Transmitter = ui->antennaPhi_Transmitter->text().toDouble(&ok);
                data.Beamwidth_Transmitter = ui->Beamwidth_Transmitter->text().toDouble(&ok);
                data.PolarizationMethod_Transmitter = ui->PolarizationMethod_Transmitter->currentText();
                data.VerticalFieldDistribution_Transmitter = ui->VerticalFieldDistribution_Transmitter->currentText();
            } else {
                // 不可见，强制写入无效值（0）
                data.CentralF_Transmitter = 0;
                data.Bandwidth_Transmitter = 0;
                data.Power_Transmitter = 0;
                data.antennaPhi_Transmitter = 0;
                data.Beamwidth_Transmitter = 0;
                data.PolarizationMethod_Transmitter = "";
                data.VerticalFieldDistribution_Transmitter = "";
            }

            // --- 接收机参数处理 ---
            if (ui->RecieverWidget->isVisible()) {
                data.CentralF_Reciever = ui->CentralF_Reciever->text().toDouble(&ok);
                data.Bandwidth_Reciever = ui->Bandwidth_Reciever->text().toDouble(&ok);
                data.Sensitive_reciever = ui->Sensitive_reciever->text().toDouble(&ok);
                data.interferenceMargin = ui->interferenceMargin->text().toDouble(&ok);
                data.SNRMargin = ui->SNRMargin->text().toDouble(&ok);
                data.noiseFigure = ui->noiseFigure->text().toDouble(&ok);
            } else {
                data.CentralF_Reciever = 0;
                data.Bandwidth_Reciever = 0;
                data.Sensitive_reciever = 0;
                data.interferenceMargin = 0;
                data.SNRMargin = 0;
                data.noiseFigure = 0;
            }
            // --- 天线参数处理 ---
            if (ui->AntennaWidget->isVisible()) {
                data.CentralF_Antenna = ui->CentralF_Antenna->text().toDouble(&ok);
                data.Bandwidth_Antenna = ui->Bandwidth_Antenna->text().toDouble(&ok);
                data.Power_Antenna = ui->Power_Antenna->text().toDouble(&ok);
                data.antennaPhi_Antenna = ui->antennaPhi_Antenna->text().toDouble(&ok);
                data.Beamwidth_Antenna = ui->Beamwidth_Antenna->text().toDouble(&ok);
                data.PolarizationMethod_Antenna = ui->PolarizationMethod_Antenna->currentText();
                data.VerticalFieldDistribution_Antenna = ui->VerticalFieldDistribution_Antenna->currentText();
            } else {
                data.CentralF_Antenna = 0;
                data.Bandwidth_Antenna = 0;
                data.Power_Antenna = 0;
                data.antennaPhi_Antenna = 0;
                data.Beamwidth_Antenna = 0;
                data.PolarizationMethod_Antenna = "";
                data.VerticalFieldDistribution_Antenna = "";
            }

            qDebug() << "Data updated for ID:" << data.equipmentID;
            break;
        }
    }
}

void DeviceWidget::onEquipmentTypeChanged()
{
    QString equipmentType = ui->equipmentType->currentText();
    // 设置基础参数
    ui->Gain->setText("15");
    ui->Power_Transmitter->setText("20");
    ui->Bandwidth_Transmitter->setText("100");
    ui->X_offset = 0;
    ui->Y_offset = 0;
    ui->Z_offset = 0;

    if (equipmentType == "发射机") {
        // 设置发射机默认参数
        ui->CentralF_Transmitter->setText("1000");
        ui->Bandwidth_Transmitter->setText("100");
        ui->Power_Transmitter->setText("20");
        ui->antennaPhi_Transmitter->setText("30");
        ui->Beamwidth_Transmitter->setText("20");
        ui->PolarizationMethod_Transmitter->setCurrentIndex(0);
        // 清空其他参数
        resetReceiverUI();
        resetAntennaUI();
        ui->RecieverWidget->setVisible(false);
        ui->AntennaWidget->setVisible(false);
    }
    else if (equipmentType == "接收机") {
        // 设置接收机默认参数
        ui->CentralF_Reciever->setText("1000");     
        ui->Bandwidth_Reciever->setText("100");
        ui->Sensitive_reciever->setText("-90");
        ui->interferenceMargin->setText("6");
        ui->SNRMargin->setText("10");
        ui->noiseFigure->setText("3");

        // 清空其他参数
        resetTransmitterUI();
        resetAntennaUI();
        ui->TransmitterWidget->setVisible(false);
        ui->AntennaWidget->setVisible(false);

    }
    else if (equipmentType == "收发一体机") {

        ui->CentralF_Transmitter->setText("1000");
        ui->Bandwidth_Transmitter->setText("100");
        ui->Power_Transmitter->setText("20");
        ui->antennaPhi_Transmitter->setText("30");
        ui->Beamwidth_Transmitter->setText("20");
        ui->PolarizationMethod_Transmitter->setCurrentIndex(0);
        
        ui->CentralF_Reciever->setText("1000");     
        ui->Bandwidth_Reciever->setText("100");
        ui->Sensitive_reciever->setText("-90");
        ui->interferenceMargin->setText("6");
        ui->SNRMargin->setText("10");
        ui->noiseFigure->setText("3");

        resetAntennaUI();
        ui->AntennaWidget->setVisible(false);

    }
    else if (equipmentType == "天线") {
        ui->CentralF_Antenna->setText("1000");
        ui->Bandwidth_Antenna->setText("100");
        ui->Power_Antenna->setText("20");
        ui->antennaPhi_Antenna->setText("30");
        ui->Beamwidth_Antenna->setText("20");
        ui->PolarizationMethod_Antenna->setCurrentIndex(0);
        ui->VerticalFieldDistribution_Antenna->setCurrentIndex(0);
    }
    
    qDebug() << "Equipment type changed to:" << equipmentType;
}



void DeviceWidget::on_equipmentReduction_clicked()
{
    delete this;
    qDebug() << "DeviceWidget destroyed";
    // qDebug().noquote() << "equipmentID:" << this->m_id;
}

void DeviceWidget::resetTransmitterUI() {
    ui->CentralF_Transmitter->setText("0");
    ui->Bandwidth_Transmitter->setText("0");
    ui->Power_Transmitter->setText("0");
    ui->antennaPhi_Transmitter->setText("0");
    ui->Beamwidth_Transmitter->setText("0");
    // 下拉框可以重置到默认索引0
    ui->PolarizationMethod_Transmitter->setCurrentIndex(0);
    ui->VerticalFieldDistribution_Transmitter->setCurrentIndex(0);
}

void DeviceWidget::resetReceiverUI() {
    ui->CentralF_Reciever->setText("0");
    ui->Bandwidth_Reciever->setText("0");
    ui->Sensitive_reciever->setText("0");
    ui->interferenceMargin->setText("0");
    ui->SNRMargin->setText("0");
    ui->noiseFigure->setText("0");
}

void DeviceWidget::resetAntennaUI() {
    ui->CentralF_Antenna->setText("0");
    ui->Bandwidth_Antenna->setText("0");
    ui->Power_Antenna->setText("0");
    ui->antennaPhi_Antenna->setText("0");
    ui->Beamwidth_Antenna->setText("0");
    ui->PolarizationMethod_Antenna->setCurrentIndex(0);
    ui->VerticalFieldDistribution_Antenna->setCurrentIndex(0);
}