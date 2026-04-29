#pragma once

#include <QHBoxLayout>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "BasePage.h"
#include "Interface/DataModel.h"

class ElaLineEdit;
class ElaPushButton;

class EnvironmentWidget : public BasePage {
    Q_OBJECT

public:
    explicit EnvironmentWidget(QWidget* parent = nullptr);
    ~EnvironmentWidget() override;

    void setData(const EnvironmentData& data);
    EnvironmentData getData() const;
    bool tryBuildData(EnvironmentData& data, QString& errorMessage) const;
    void loadFromModel();
    bool saveToModel(QString* errorMessage = nullptr);
    void setReadOnly(bool readOnly);
    bool isDirty() const { return _isDirty; }

signals:
    void dirtyStateChanged(bool isDirty);
    void modelCommitted();

private slots:
    void on_SaveEnvironmentBtn_clicked();
    void markDirty();

private:
    void setDirty(bool dirty);

    ElaLineEdit* _maxRangeEdit{nullptr};
    ElaLineEdit* _ductHeightEdit{nullptr};
    ElaLineEdit* _windSpeedEdit{nullptr};
    ElaLineEdit* _dxEdit{nullptr};
    ElaLineEdit* _dzEdit{nullptr};
    ElaLineEdit* _nzEdit{nullptr};
    ElaLineEdit* _angleStepEdit{nullptr};
    ElaPushButton* SaveEnvironmentConfigBtn{nullptr};
    bool _isDirty{false};
    bool _isLoading{false};
    bool _isReadOnly{false};
};
