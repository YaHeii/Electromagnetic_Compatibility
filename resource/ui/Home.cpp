#include "Home.h"

#include <QDebug>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

#include "ElaAcrylicUrlCard.h"
#include "ElaFlowLayout.h"
#include "ElaImageCard.h"
#include "ElaMenu.h"
#include "ElaMessageBar.h"
#include "ElaNavigationRouter.h"
#include "ElaPopularCard.h"
#include "ElaScrollArea.h"
#include "ElaText.h"
#include "ElaToolTip.h"
#include "TreeView.h"

Home::Home(QWidget* parent)
    : BasePage(parent)
{
    // 预览窗口标题
    //setWindowTitle("Home");
    createCustomWidget("展示软件数据");

    //ElaImageCard* backgroundCard = new ElaImageCard(this);
    //backgroundCard->setBorderRadius(10);
    //backgroundCard->setFixedHeight(340);
    //backgroundCard->setCardImage(QImage(":/Home_Background.png"));
   

    TreeView* MainTreeView = new TreeView(this);
    
    QVBoxLayout* treeViewLayout = new QVBoxLayout;
    treeViewLayout->addWidget(MainTreeView);

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle("主界面");
    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    //centerVLayout->addWidget(backgroundCard);
    centerVLayout->addLayout(treeViewLayout);
    // centerVLayout->addSpacing(10);
    centerVLayout->addStretch();
    addCentralWidget(centralWidget, true, false, 0);

    // 初始化提示
    ElaMessageBar::success(ElaMessageBarType::BottomRight, "Success", "初始化成功!", 2000);
}

Home::~Home()
{
}

void Home::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::RightButton:
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        _homeMenu->popup(event->globalPosition().toPoint());
#else
        _homeMenu->popup(event->globalPos());
#endif
        break;
    }
    default:
    {
        break;
    }
    }
    ElaScrollPage::mouseReleaseEvent(event);
}
