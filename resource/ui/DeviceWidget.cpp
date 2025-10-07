#include "DeviceWidget.h"
#include <QDebug>

DeviceWidget::DeviceWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceWidget)
{
    ui->setupUi(this);
}
DeviceWidget::~DeviceWidget()
{
    delete ui;
}

void DeviceWidget::setData(const DeviceData &data)
{
    m_currentId = data.equipmentID;
    ui->equipmentID->setText(data.equipmentID);
    ui->equipmentType->setCurrentText(data.equipmentType);
    ui->antennaType->setCurrentText(data.antennaType);
    ui->filterType->setCurrentText(data.filterType);
    ui->singelAntennaType->setCurrentText(data.singelAntennaType);
    ui->reciever_TransmiterID->setText(data.reciever_TransmiterID);
    ui->Gain->setText(QString::number(data.Gain));
    ui->X_offset->setText(QString::number(data.X_offset));
    ui->Y_offset->setText(QString::number(data.Y_offset));
    ui->antennaTheta->setText(QString::number(data.antennaTheta));
    ui->antennaPhi->setText(QString::number(data.antennaPhi));
    ui->exportPattern->setText(QString::number(data.pattern));
    ui->transmitterPower->setText(QString::number(data.transmitterPower));
    ui->transmitterBandwidth->setText(QString::number(data.transmitterBandwidth));
    ui->WIP->setText(QString::number(data.WIP));
    ui->recieverSensitive->setText(QString::number(data.recieverSensitive));
    ui->recieverBandwidth->setText(QString::number(data.recieverBandwidth));
    ui->noiseFigure->setText(QString::number(data.noiseFigure));
    ui->SNRMargin->setText(QString::number(data.SNRMargin));
    ui->interferenceMargin->setText(QString::number(data.interferenceMargin));

}

void DeviceWidget::updateModelData() {
    bool ok;
    for (DeviceData &data : DataModel::instance()->allDevices) {
        if(data.equipmentID == m_currentId){
            data.equipmentID = ui->equipmentID->text();
            if (data.equipmentID.isEmpty()) {
                qWarning() << "Validation Failed: 'equipmentID' cannot be empty.";
            }

            data.equipmentType = ui->equipmentType->currentText();
            data.antennaType = ui->antennaType->currentText();
            data.filterType = ui->filterType->currentText();
            data.singelAntennaType = ui->singelAntennaType->currentText();
            data.reciever_TransmiterID = ui->reciever_TransmiterID->text();

            // --- Antenna Parameters ---
            data.Gain = ui->Gain->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'Gain' is not a valid number. Value:" << ui->Gain->text();
            }

            data.X_offset = ui->X_offset->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'X_offset' is not a valid number. Value:" << ui->X_offset->text();
            }

            data.Y_offset = ui->Y_offset->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'Y_offset' is not a valid number. Value:" << ui->Y_offset->text();
            }

            data.antennaTheta = ui->antennaTheta->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'antennaTheta' is not a valid number. Value:" << ui->antennaTheta->text();
            }

            data.antennaPhi = ui->antennaPhi->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'antennaPhi' is not a valid number. Value:" << ui->antennaPhi->text();
            }

            data.pattern = ui->exportPattern->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'pattern' is not a valid number. Value:" << ui->exportPattern->text();
            }

            // --- Transmitter Parameters ---
            data.transmitterPower = ui->transmitterPower->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'transmitterPower' is not a valid number. Value:" << ui->transmitterPower
                        ->
                        text();
            }

            data.transmitterBandwidth = ui->transmitterBandwidth->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'transmitterBandwidth' is not a valid number. Value:" << ui->
                        transmitterBandwidth->text();
            }

            data.WIP = ui->WIP->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'WIP' is not a valid number. Value:" << ui->WIP->text();
            }

            // --- Receiver Parameters ---
            data.recieverSensitive = ui->recieverSensitive->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'recieverSensitive' is not a valid number. Value:" << ui->
                        recieverSensitive->
                        text();
            }

            data.recieverBandwidth = ui->recieverBandwidth->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'recieverBandwidth' is not a valid number. Value:" << ui->
                        recieverBandwidth->
                        text();
            }

            data.noiseFigure = ui->noiseFigure->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'noiseFigure' is not a valid number. Value:" << ui->noiseFigure->text();
            }

            data.SNRMargin = ui->SNRMargin->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'SNRMargin' is not a valid number. Value:" << ui->SNRMargin->text();
            }

            data.interferenceMargin = ui->interferenceMargin->text().toDouble(&ok);
            if (!ok) {
                qWarning() << "Validation Failed: 'interferenceMargin' is not a valid number. Value:" << ui->
                        interferenceMargin
                        ->text();
            }

            qDebug() << "--- Device Data Validated and Collected ---";
            qDebug().noquote() << "equipmentID:" << data.equipmentID;
            qDebug().noquote() << "equipmentType:" << data.equipmentType;
            qDebug() << "Gain:" << data.Gain;
            qDebug() << "X_offset:" << data.X_offset;
            qDebug() << "Y_offset:" << data.Y_offset;
        }
    }
}

void DeviceWidget::on_equipmentReduction_clicked()
{
    delete this;
    qDebug() << "DeviceWidget destroyed";
    // qDebug().noquote() << "equipmentID:" << this->m_id;
}
