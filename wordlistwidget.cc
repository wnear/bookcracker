#include "wordlistwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListView>

#include <QMenu>
#include <QContextMenuEvent>
#include <QTextEdit>
#include <QListWidget>
#include <QListView>
#include <QStringListModel>
#include <QLabel>
#include <QFileInfo>
#include <QDebug>
#include <QPushButton>
#include <QStringListModel>
#include "wordmodel.h"
#include "words.h"
#include <QCheckBox>
#include <QThread>

void removeSeletedRowsFromView(QAbstractItemView *view) {
    auto selections = view->selectionModel()->selectedIndexes();
    if (selections.isEmpty()) return;
    // sort indexes with  x.row() from big to small.
    std::sort(selections.begin(), selections.end(),
              [](auto &&l, auto &&r) { return l.row() > r.row(); });
    auto model = view->model();
    for (auto idx : selections) {
        model->removeRow(idx.row());
    }
}

struct PrivateData {
    QListView *wordlistview{nullptr};
    QStringListModel *model{nullptr};
    WordModel *sourceModel{nullptr};
    WordSortFilterProxyModel *proxyModel{nullptr};
    WordModel::COLUMN_NO sortorder{WordModel::COLUMN_POS_IN_PAGE};

    QCheckBox *btn_showdict{nullptr};
    QCheckBox *btn_showConciseWordOnly{nullptr};
    QCheckBox *btn_showConciseOnly{nullptr};
    QPushButton *btn_scanscope;

    std::shared_ptr<SqlSave> wordstore{nullptr};
};

WordlistWidget::WordlistWidget(QWidget *parent) : QWidget(parent) {
    d = new PrivateData;
    auto wordwgt = this;
    // layout.
    auto lay = new QVBoxLayout;
    wordwgt->setLayout(lay);
    {
        {
            d->btn_showdict = new QCheckBox("Show word in dict", this);
            lay->addWidget(d->btn_showdict);
            connect(d->btn_showdict, &QPushButton::clicked, this,
                    &WordlistWidget::updateFilter);
        }

        {
            d->btn_showConciseWordOnly = new QCheckBox("Show concise word only", this);

            lay->addWidget(d->btn_showConciseWordOnly);
            connect(d->btn_showConciseWordOnly, &QPushButton::clicked, this,
                    &WordlistWidget::updateFilter);
        }

        {
            d->btn_scanscope = new QPushButton("Switch between doc-page", this);
            lay->addWidget(d->btn_scanscope);

            connect(d->btn_scanscope, &QPushButton::clicked, this,
                    &WordlistWidget::updateFilter);
        }
    }
    d->wordlistview = new QListView(this);
    lay->addWidget(d->wordlistview);

    // data.

    d->sourceModel = new WordModel(nullptr);
    d->proxyModel = new WordSortFilterProxyModel(this);
    d->proxyModel->setSourceModel(d->sourceModel);

    d->wordlistview->setModel(d->proxyModel);
    d->wordlistview->setSelectionMode(QAbstractItemView::ExtendedSelection);
    d->wordlistview->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d->wordlistview, &QWidget::customContextMenuRequested, this,
            &WordlistWidget::onListViewContextMenu);
}

void WordlistWidget::setWordStore(std::shared_ptr<SqlSave> wordstore) {
    d->wordstore = wordstore;
}

void WordlistWidget::setupModel(WordItemMap *data) {
    qDebug() << __PRETTY_FUNCTION__;
    if (d->sourceModel == nullptr) {
        return;
    }
    d->sourceModel->setupModelData(data);
}

void WordlistWidget::onListViewContextMenu(const QPoint &pos) {
    auto menu = new QMenu;
    menu->addAction("test");
    menu->addAction("sort by position", [this] {
        d->sortorder = WordModel::COLUMN_POS_IN_PAGE;
        d->proxyModel->sort(d->sortorder);
    });
    menu->addAction("sort by alphabeta", [this] {
        d->sortorder = WordModel::COLUMN_WORD;
        d->proxyModel->sort(d->sortorder);
    });
    auto sels = d->wordlistview->selectionModel()->selectedIndexes();
    if (!sels.isEmpty()) {
        menu->addSeparator();
        // sort indexes with  x.row() from big to small.
        menu->addAction("knew", [this]() {
            qDebug() << "mark as knew.";
            this->markSelectionWithLevel(WORD_IS_KNOWN);
        });
        menu->addAction("ignore",
                        [this]() { this->markSelectionWithLevel(WORD_IS_IGNORED); });
        menu->addAction("add to dict",
                        [this]() { this->markSelectionWithLevel(WORD_IS_LEARNING); });
    }
    menu->exec(mapToGlobal(pos));
}

// NOTE: need to do multiple-things
// edit the page-view highlight
// edit the item status, remove from list-view.
void WordlistWidget::markSelectionWithLevel(wordlevel_t lv) {
    auto sels = d->wordlistview->selectionModel()->selectedIndexes();
    std::sort(sels.begin(), sels.end(),
              [](auto &&l, auto &&r) { return l.row() > r.row(); });
    QList<WordItem *> edits;
    QStringList words;
    for(auto idx: sels){
        auto src_idx = d->proxyModel->mapToSource(idx);
        auto *item = static_cast<WordItem*>(src_idx.internalPointer());
        auto word = item->content;
        edits.push_back(item);
        words.push_back(word);
        d->wordstore->updateWord(word, item->wordlevel, lv);
    }

    emit markItemsLevel(words, lv);

    d->proxyModel->updateFilter();
}

void WordlistWidget::onPageLoadBefore() { d->sourceModel->reset_data_before(); }

void WordlistWidget::onPageLoadAfter() {
    d->sourceModel->reset_data_after();
    d->wordlistview->reset();
    d->proxyModel->sort(d->sortorder);
}
