
#include "About.h"
#include <QHBoxLayout>
#include <QIcon>
#include <QVBoxLayout>

#include "ElaImageCard.h"
#include "ElaText.h"
About::About(QWidget* parent)
    : ElaDialog(parent)
{
    setFixedSize(400, 300);
    setWindowTitle("关于..");
    setWindowIcon(QIcon(":/about_page_pic.png"));
    this->setIsFixedSize(true);
    setWindowModality(Qt::ApplicationModal);
    setWindowButtonFlags(ElaAppBarType::CloseButtonHint);
    ElaImageCard* pixCard = new ElaImageCard(this);
    pixCard->setFixedSize(60, 60);
    pixCard->setIsPreserveAspectCrop(false);
    pixCard->setCardImage(QImage(":/about_page_pic.png"));

    QVBoxLayout* pixCardLayout = new QVBoxLayout();
    pixCardLayout->addWidget(pixCard);
    pixCardLayout->addStretch();

    ElaText* versionText = new ElaText("编队效能评估", this);
    QFont versionTextFont = versionText->font();
    versionTextFont.setWeight(QFont::Bold);
    versionText->setFont(versionTextFont);
    versionText->setWordWrap(false);
    versionText->setTextPixelSize(18);

    //ElaText* licenseText = new ElaText("MIT授权协议", this);
    //licenseText->setWordWrap(false);
    //licenseText->setTextPixelSize(14);
    ElaText* supportText = new ElaText("Windows支持版本: Win11", this);
    supportText->setWordWrap(false);
    supportText->setTextPixelSize(14);
    /*ElaText* contactText = new ElaText("作者: 80985@qq.com\n交流群: 850243692(QQ)", this);
    contactText->setWordWrap(false);
    contactText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contactText->setTextPixelSize(14);*/
    //ElaText* helperText = new ElaText("用户手册及API文档付费提供\n提供额外的专业技术支持", this);
    //helperText->setWordWrap(false);
    //helperText->setTextPixelSize(14);
    ElaText* descriptionText = new ElaText("针对海上无人船编队效能评估\n提供一站式解决方案");
    descriptionText->setWordWrap(true);
    descriptionText->setTextPixelSize(14);
    ElaText* copyrightText = new ElaText("版权所有 © 2026 Yahei", this);
    copyrightText->setWordWrap(false);
    copyrightText->setTextPixelSize(14);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(15);
    textLayout->addWidget(versionText);
    //textLayout->addWidget(licenseText);
    textLayout->addWidget(supportText);
    //textLayout->addWidget(contactText);
    //textLayout->addWidget(helperText);
    textLayout->addWidget(descriptionText);
    textLayout->addWidget(copyrightText);
    textLayout->addStretch();

    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->addSpacing(30);
    contentLayout->addLayout(pixCardLayout);
    contentLayout->addSpacing(30);
    contentLayout->addLayout(textLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 25, 0, 0);
    mainLayout->addLayout(contentLayout);
}

About::~About()
{
}
