#pragma once

#include "Resource/ui/BasePage.h"
class ElaRadioButton;
class ElaToggleSwitch;
class ElaComboBox;
class Setting : public BasePage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit Setting(QWidget* parent = nullptr);
    ~Setting() override;

private:
    ElaComboBox* _themeComboBox{nullptr};
    ElaRadioButton* _normalButton{nullptr};

    ElaRadioButton* _windowNormalButton{nullptr};
    ElaRadioButton* _windowPixmapButton{nullptr};
    ElaRadioButton* _windowMovieButton{nullptr};

    ElaRadioButton* _elaMicaButton{nullptr};
#ifdef Q_OS_WIN
    ElaRadioButton* _micaButton{nullptr};
    ElaRadioButton* _micaAltButton{nullptr};
    ElaRadioButton* _acrylicButton{nullptr};
    ElaRadioButton* _dwmBlurnormalButton{nullptr};
#endif
    ElaToggleSwitch* _logSwitchButton{nullptr};
    ElaToggleSwitch* _userCardSwitchButton{nullptr};
    ElaRadioButton* _minimumButton{nullptr};
    ElaRadioButton* _compactButton{nullptr};
    ElaRadioButton* _maximumButton{nullptr};
    ElaRadioButton* _autoButton{nullptr};

    ElaRadioButton* _noneButton{nullptr};
    ElaRadioButton* _popupButton{nullptr};
    ElaRadioButton* _scaleButton{nullptr};
    ElaRadioButton* _flipButton{nullptr};
    ElaRadioButton* _blurButton{nullptr};
};
