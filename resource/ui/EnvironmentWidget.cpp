#include "EnvironmentWidget.h"

#include <array>
#include <QFormLayout>

#include "ElaLineEdit.h"
#include "ElaMessageBar.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "spdlog/spdlog.h"

namespace {

bool readRequiredDouble(const QLineEdit* lineEdit, const QString& fieldName, double& value, QString& errorMessage) {
    bool ok = false;
    value = lineEdit->text().trimmed().toDouble(&ok);
    if (!ok) {
        errorMessage = QStringLiteral("%1 必须是数字").arg(fieldName);
        return false;
    }
    return true;
}

bool readRequiredInt(const QLineEdit* lineEdit, const QString& fieldName, int& value, QString& errorMessage) {
    bool ok = false;
    value = lineEdit->text().trimmed().toInt(&ok);
    if (!ok) {
        errorMessage = QStringLiteral("%1 必须是整数").arg(fieldName);
        return false;
    }
    return true;
}

void addRow(QFormLayout* formLayout, const QString& labelText, ElaLineEdit* lineEdit) {
    auto* label = new ElaText(labelText);
    label->setTextPixelSize(14);
    formLayout->addRow(label, lineEdit);
}

}  // namespace

EnvironmentWidget::EnvironmentWidget(QWidget* parent)
    : BasePage(parent) {
    
    createCustomWidget("在此页维护环境参数，并统一写入 DataModel 环境快照");

    _maxRangeEdit = new ElaLineEdit(this);
    _ductHeightEdit = new ElaLineEdit(this);
    _windSpeedEdit = new ElaLineEdit(this);
    _dxEdit = new ElaLineEdit(this);
    _dzEdit = new ElaLineEdit(this);
    _nzEdit = new ElaLineEdit(this);
    _angleStepEdit = new ElaLineEdit(this);

    _maxRangeEdit->setPlaceholderText("m");
    _ductHeightEdit->setPlaceholderText("m");
    _windSpeedEdit->setPlaceholderText("m/s");
    _dxEdit->setPlaceholderText("m");
    _dzEdit->setPlaceholderText("m");
    _nzEdit->setPlaceholderText("整数");
    _angleStepEdit->setPlaceholderText("1-360");

    auto* formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(16);
    addRow(formLayout, QStringLiteral("最大传播距离"), _maxRangeEdit);
    addRow(formLayout, QStringLiteral("蒸发波导高度"), _ductHeightEdit);
    addRow(formLayout, QStringLiteral("风速"), _windSpeedEdit);
    addRow(formLayout, QStringLiteral("水平步进 dx"), _dxEdit);
    addRow(formLayout, QStringLiteral("垂直分辨率 dz"), _dzEdit);
    addRow(formLayout, QStringLiteral("垂直网格数 nz"), _nzEdit);
    addRow(formLayout, QStringLiteral("角度步进"), _angleStepEdit);

    SaveEnvironmentConfigBtn = new ElaPushButton(QStringLiteral("保存环境参数"), this);
    SaveEnvironmentConfigBtn->setFixedSize(120, 36);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(SaveEnvironmentConfigBtn);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    centralWidget->setWindowTitle("环境属性管理");
    
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    addCentralWidget(centralWidget);

    const std::array<ElaLineEdit*, 7> edits = {
        _maxRangeEdit,
        _ductHeightEdit,
        _windSpeedEdit,
        _dxEdit,
        _dzEdit,
        _nzEdit,
        _angleStepEdit,
    };
    for (ElaLineEdit* edit : edits) {
        connect(edit, &QLineEdit::textChanged, this, &EnvironmentWidget::markDirty);
    }
    connect(SaveEnvironmentConfigBtn, &ElaPushButton::clicked, this, &EnvironmentWidget::on_SaveEnvironmentBtn_clicked);

    loadFromModel();
}

EnvironmentWidget::~EnvironmentWidget() = default;

void EnvironmentWidget::setData(const EnvironmentData& data) {
    _isLoading = true;
    _maxRangeEdit->setText(QString::number(data.maxRange));
    _ductHeightEdit->setText(QString::number(data.ductHeight));
    _windSpeedEdit->setText(QString::number(data.windSpeed));
    _dxEdit->setText(QString::number(data.dx));
    _dzEdit->setText(QString::number(data.dz));
    _nzEdit->setText(QString::number(data.nz));
    _angleStepEdit->setText(QString::number(data.angleStepDeg));
    _isLoading = false;
    setDirty(false);
}

EnvironmentData EnvironmentWidget::getData() const {
    EnvironmentData data;
    QString errorMessage;
    if (!tryBuildData(data, errorMessage)) {
        spdlog::warn("环境参数 DTO 组装失败: {}", errorMessage.toStdString());
    }
    return data;
}

bool EnvironmentWidget::tryBuildData(EnvironmentData& data, QString& errorMessage) const {
    data = EnvironmentData{};
    if (!readRequiredDouble(_maxRangeEdit, QStringLiteral("最大传播距离"), data.maxRange, errorMessage) ||
        !readRequiredDouble(_ductHeightEdit, QStringLiteral("蒸发波导高度"), data.ductHeight, errorMessage) ||
        !readRequiredDouble(_windSpeedEdit, QStringLiteral("风速"), data.windSpeed, errorMessage) ||
        !readRequiredDouble(_dxEdit, QStringLiteral("水平步进 dx"), data.dx, errorMessage) ||
        !readRequiredDouble(_dzEdit, QStringLiteral("垂直分辨率 dz"), data.dz, errorMessage) ||
        !readRequiredInt(_nzEdit, QStringLiteral("垂直网格数 nz"), data.nz, errorMessage) ||
        !readRequiredInt(_angleStepEdit, QStringLiteral("角度步进"), data.angleStepDeg, errorMessage)) {
        return false;
    }
    return true;
}

void EnvironmentWidget::loadFromModel() {
    setData(DataModel::instance()->environmentConfig);
}

bool EnvironmentWidget::saveToModel(QString* errorMessage) {
    EnvironmentData environmentData;
    QString localError;
    if (!tryBuildData(environmentData, localError)) {
        if (errorMessage) {
            *errorMessage = localError;
        }
        return false;
    }

    DataModel* model = DataModel::instance();
    auto snapshot = model->createSnapshot();
    snapshot.environmentConfig = environmentData;

    const auto validationResult = DataModel::validateSnapshot(snapshot);
    if (!validationResult.first) {
        if (errorMessage) {
            *errorMessage = validationResult.second;
        }
        return false;
    }

    model->environmentConfig = snapshot.environmentConfig;
    setDirty(false);
    emit modelCommitted();
    return true;
}

void EnvironmentWidget::setReadOnly(bool readOnly) {
    _isReadOnly = readOnly;
    const std::array<ElaLineEdit*, 7> edits = {
        _maxRangeEdit,
        _ductHeightEdit,
        _windSpeedEdit,
        _dxEdit,
        _dzEdit,
        _nzEdit,
        _angleStepEdit,
    };
    for (ElaLineEdit* edit : edits) {
        edit->setReadOnly(readOnly);
    }
    SaveEnvironmentConfigBtn->setEnabled(!readOnly);
}

void EnvironmentWidget::on_SaveEnvironmentBtn_clicked() {
    QString errorMessage;
    if (!saveToModel(&errorMessage)) {
        spdlog::error("环境参数保存失败: {}", errorMessage.toStdString());
        ElaMessageBar::error(ElaMessageBarType::BottomRight, QStringLiteral("保存失败"), errorMessage, 2000, this);
        return;
    }

    spdlog::info("环境参数保存成功");
    ElaMessageBar::success(ElaMessageBarType::BottomRight, QStringLiteral("保存成功"), QStringLiteral("环境参数已写入当前模型"), 1500, this);
}

void EnvironmentWidget::markDirty() {
    if (_isLoading || _isReadOnly) {
        return;
    }
    setDirty(true);
}

void EnvironmentWidget::setDirty(bool dirty) {
    if (_isDirty == dirty) {
        return;
    }
    _isDirty = dirty;
    emit dirtyStateChanged(_isDirty);
}
