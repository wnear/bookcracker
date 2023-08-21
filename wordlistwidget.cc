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
    qDebug() << __PRETTY_FUNCTION__ << ":" << __LINE__;
    auto selections = view->selectionModel()->selectedIndexes();
    if (selections.isEmpty()) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << __LINE__;
        return;
    }
    // sort indexes with  x.row() from big to small.
    std::sort(selections.begin(), selections.end(),
              [](auto &&l, auto &&r) { return l.row() > r.row(); });
    auto model = view->model();
    for (auto idx : selections) {
        qDebug() << "remove row :" << idx.row();
        model->removeRow(idx.row());
    }
    view->update();
    qDebug() << __PRETTY_FUNCTION__ << ":" << __LINE__;
}

struct PrivateData {
    QListView *wordlistview{nullptr};
    WordItemMap *data;
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
    d->btn_showdict = new QCheckBox("Show word in dict", this);
    lay->addWidget(d->btn_showdict);
    connect(d->btn_showdict, &QPushButton::clicked, this, &WordlistWidget::updateFilter);

    d->btn_showConciseWordOnly = new QCheckBox("Show concise word only", this);

    lay->addWidget(d->btn_showConciseWordOnly);
    connect(d->btn_showConciseWordOnly, &QPushButton::clicked, this,
            &WordlistWidget::updateFilter);
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
    d->data = data;
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

//NOTE: if should know the level before update:
//1. for sql, add/update, should know.
//2. highlight, doesn't need, highlight use ptr.
void WordlistWidget::markSelectionWithLevel(wordlevel_t lv) {
    auto sels = d->wordlistview->selectionModel()->selectedIndexes();
    qDebug() << "sels size:" << sels.size();
    std::sort(sels.begin(), sels.end(),
              [](auto &&l, auto &&r) { return l.row() > r.row(); });
    QList<WordItem *> edits;
    QStringList words;
    for (auto idx : sels) {
        auto src_idx = d->proxyModel->mapToSource(idx);
        auto *item = static_cast<WordItem *>(src_idx.internalPointer());
        auto word = item->content;
        edits.push_back(item);
        words.push_back(word);
        d->wordstore->updateWord(word, item->wordlevel, lv);
        // qDebug() << QString("update word %1 from level %2 to level %3")
        //                 .arg(word)
        //                 .arg(levelString(item->wordlevel))
        //                 .arg(levelString(lv));
        item->wordlevel = lv;
        d->data->value(word)->wordlevel = lv;
    }
    // TODO:  make connections to edit highlight.
    emit markItemsLevel(words, lv);

    // TODO: mark to
    // 1. knew, hide.
    // 2. dict, hide or show, checkings. filter should check settings then show/hide.
    // 3. ignore, hide.
    d->proxyModel->updateFilter();

    d->wordlistview->update();
}

void WordlistWidget::onPageLoadBefore() { d->sourceModel->reset_data_before(); }

void WordlistWidget::onPageLoadAfter() {
    d->sourceModel->reset_data_after();
    d->wordlistview->reset();
    d->proxyModel->sort(d->sortorder);
}
