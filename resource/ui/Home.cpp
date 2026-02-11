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

    setTitleVisible(true);
    setContentsMargins(2, 2, 0, 0);
    //// 标题卡片区域
    //ElaText* desText = new ElaText("主界面", this);
    setWindowTitle("主界面");
    //desText->setTextPixelSize(18);
    //ElaText* titleText = new ElaText("HomeView");
    //titleText->setTextPixelSize(35);

    //QVBoxLayout* titleLayout = new QVBoxLayout();
    //titleLayout->setContentsMargins(30, 10, 0, 0);
    //titleLayout->addWidget(desText);
    //titleLayout->addWidget(titleText);
    createCustomWidget("树型视图被放置于此，可在此界面观察目前数据模型");

    //ElaImageCard* backgroundCard = new ElaImageCard(this);
    //backgroundCard->setBorderRadius(10);
    //backgroundCard->setFixedHeight(340);
    //backgroundCard->setCardImage(QImage(":/Home_Background.png"));

    TreeView* MainTreeView = new TreeView(this);
    //MainTreeView->set

    QWidget* centralWidget = new QWidget(this);
    //centralWidget->setWindowTitle("主界面");
    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setSpacing(0);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    //centerVLayout->addWidget(backgroundCard);
    centerVLayout->addWidget(MainTreeView);
    centerVLayout->addStretch();
    addCentralWidget(centralWidget);

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
