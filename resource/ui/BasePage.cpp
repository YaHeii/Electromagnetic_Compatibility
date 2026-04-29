#include "BasePage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ElaText.h"
#include "ElaTheme.h"
#include "ElaToolButton.h"
#include "ElaWindow.h"
BasePage::BasePage(QWidget* parent)
    : ElaScrollPage(parent)
{
    connect(eTheme, &ElaTheme::themeModeChanged, this, [=]() {
        if (!parent)
        {
            update();
        }
    });
    setContentsMargins(20, 5, 0, 0);
}

BasePage::~BasePage()
{
}

void BasePage::createCustomWidget(QString desText)
{
    // 顶部元素
    QWidget* customWidget = new QWidget(this);

    //回退
    ElaToolButton* backtrackButton = new ElaToolButton(this);
    backtrackButton->setFixedSize(35, 35);
    backtrackButton->setIsTransparent(false);
    backtrackButton->setElaIcon(ElaIconType::Timer);
    connect(backtrackButton, &ElaToolButton::clicked, this, [=]() {
        ElaWindow* window = dynamic_cast<ElaWindow*>(this->window());
        if (window)
        {
            window->backtrackNavigationNode(property("ElaPageKey").toString());
        }
    });

    ElaText* descText = new ElaText(this);
    descText->setText(desText);
    descText->setTextPixelSize(13);

    QVBoxLayout* topLayout = new QVBoxLayout(customWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* subLayout = new QHBoxLayout();
    subLayout->addWidget(descText);
    subLayout->addStretch();
    subLayout->addWidget(backtrackButton);
    subLayout->addSpacing(15);
    topLayout->addLayout(subLayout);
    setCustomWidget(customWidget);
}
