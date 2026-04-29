#pragma once

#include <QWidget>
#include <qevent.h>

#include "Interface/DataModel.h"

class ElaComboBox;
class ElaPushButton;

class DeviceonShip : public QWidget {
    Q_OBJECT

public:
    explicit DeviceonShip(QWidget* parent = nullptr);
    ~DeviceonShip() override;

    void setData(const EquipmentOnShip& data);
    EquipmentOnShip getData() const;
    void refreshEquipmentList();
    void setReadOnly(bool readOnly);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void removalRequested();
    void dataEdited();

private slots:
    void on_deleteDeviceonShip_clicked();

private:
    EquipmentOnShip _data;
    ElaComboBox* _EquipmentIDCombo{nullptr};
    ElaPushButton* _deleteButton{nullptr};
};
