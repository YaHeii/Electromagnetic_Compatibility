#include "TreeView.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollBar.h"
#include "ElaText.h"
#include "ElaTreeView.h"
#include "ModelView/T_TreeViewModel.h"

TreeView::TreeView(QWidget* parent)
    : BasePage(parent)
{
    createCustomWidget(QStringLiteral("当前页只读展示 DataModel 快照，支持刷新、展开、折叠和关键字查找"));

    _treeModel = new T_TreeViewModel(this);

    auto* treeLayout = new QVBoxLayout();
    treeLayout->setContentsMargins(0, 0, 10, 0);

    auto* treeSettingWidget = new QWidget(this);
    auto* treeSettingWidgetLayout = new QVBoxLayout(treeSettingWidget);
    treeSettingWidgetLayout->setContentsMargins(0, 0, 0, 0);
    treeSettingWidgetLayout->setSpacing(15);

    _dataText = new ElaText(this);
    _dataText->setTextPixelSize(15);

    _searchEdit = new ElaLineEdit(this);
    _searchEdit->setPlaceholderText(QStringLiteral("输入参数名、船只 ID 或设备 ID"));
    connect(_searchEdit, &QLineEdit::returnPressed, this, &TreeView::onSearchRequested);

    auto* refreshButton = new ElaPushButton(QStringLiteral("刷新"), this);
    refreshButton->setFixedWidth(80);
    connect(refreshButton, &ElaPushButton::clicked, this, &TreeView::syncViewWithModel);

    auto* searchButton = new ElaPushButton(QStringLiteral("查找"), this);
    searchButton->setFixedWidth(80);
    connect(searchButton, &ElaPushButton::clicked, this, &TreeView::onSearchRequested);

    auto* expandButton = new ElaPushButton(QStringLiteral("展开全部"), this);
    expandButton->setFixedWidth(90);
    connect(expandButton, &ElaPushButton::clicked, this, &TreeView::expandAll);

    auto* collapseButton = new ElaPushButton(QStringLiteral("折叠全部"), this);
    collapseButton->setFixedWidth(90);
    connect(collapseButton, &ElaPushButton::clicked, this, &TreeView::collapseAll);

    _searchResultText = new ElaText(QStringLiteral("输入关键字后可定位首个匹配项"), this);
    _searchResultText->setTextPixelSize(14);

    auto* toolLayout = new QHBoxLayout();
    toolLayout->setContentsMargins(0, 0, 0, 0);
    toolLayout->addWidget(refreshButton);
    toolLayout->addSpacing(10);
    toolLayout->addWidget(_searchEdit, 1);
    toolLayout->addSpacing(10);
    toolLayout->addWidget(searchButton);
    toolLayout->addSpacing(20);
    toolLayout->addWidget(collapseButton);
    toolLayout->addSpacing(10);
    toolLayout->addWidget(expandButton);

    treeSettingWidgetLayout->addWidget(_dataText);
    treeSettingWidgetLayout->addLayout(toolLayout);
    treeSettingWidgetLayout->addWidget(_searchResultText);
    treeSettingWidgetLayout->addStretch();

    _treeView = new ElaTreeView(this);
    auto* treeViewFloatScrollBar = new ElaScrollBar(_treeView->verticalScrollBar(), _treeView);
    treeViewFloatScrollBar->setIsAnimation(true);
    QFont headerFont = _treeView->header()->font();
    headerFont.setPixelSize(16);
    _treeView->header()->setFont(headerFont);
    _treeView->setFixedHeight(450);
    _treeView->setModel(_treeModel);
    _treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    _treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* treeViewLayout = new QVBoxLayout();
    treeViewLayout->setContentsMargins(0, 0, 0, 0);
    treeViewLayout->addWidget(_treeView);
    treeViewLayout->addStretch();

    treeLayout->addWidget(treeSettingWidget);
    treeLayout->addLayout(treeViewLayout);

    auto* centralWidget = new QWidget(this);
    centralWidget->setWindowTitle(QStringLiteral("树视图"));
    auto* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->addLayout(treeLayout);
    addCentralWidget(centralWidget, true, false, 0);

    syncViewWithModel();
}

TreeView::~TreeView() = default;

void TreeView::syncViewWithModel()
{
    if (!_treeModel || !_treeView) {
        return;
    }

    _treeModel->reloadFromDataModel();
    updateSummaryText();
    _treeView->collapseAll();
    for (int row = 0; row < _treeModel->rowCount(); ++row) {
        _treeView->expand(_treeModel->index(row, 0));
    }
    _searchResultText->setText(QStringLiteral("树视图已刷新"));
}

void TreeView::expandAll()
{
    if (_treeView) {
        _treeView->expandAll();
    }
}

void TreeView::collapseAll()
{
    if (_treeView) {
        _treeView->collapseAll();
    }
}

bool TreeView::findAndSelect(const QString& keyword)
{
    if (!_treeModel || !_treeView) {
        return false;
    }

    const QString trimmedKeyword = keyword.trimmed();
    if (trimmedKeyword.isEmpty()) {
        _searchResultText->setText(QStringLiteral("请输入查找关键字"));
        return false;
    }

    const QModelIndex matchedIndex = _treeModel->findItemIndex(trimmedKeyword);
    if (!matchedIndex.isValid()) {
        _searchResultText->setText(QStringLiteral("未找到关键字：%1").arg(trimmedKeyword));
        return false;
    }

    QModelIndex parentIndex = matchedIndex.parent();
    while (parentIndex.isValid()) {
        _treeView->expand(parentIndex);
        parentIndex = parentIndex.parent();
    }

    _treeView->setCurrentIndex(matchedIndex);
    if (_treeView->selectionModel()) {
        _treeView->selectionModel()->select(
            matchedIndex,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    _treeView->scrollTo(matchedIndex);
    _searchResultText->setText(QStringLiteral("已定位：%1").arg(_treeModel->data(matchedIndex, Qt::DisplayRole).toString()));
    return true;
}

void TreeView::onSearchRequested()
{
    findAndSelect(_searchEdit ? _searchEdit->text() : QString());
}

void TreeView::updateSummaryText()
{
    if (_dataText && _treeModel) {
        _dataText->setText(QStringLiteral("当前树节点数：%1").arg(_treeModel->getItemCount()));
    }
}
