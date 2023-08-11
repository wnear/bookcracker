#include "mainwindow.h"
#include "words.h"
#include "worditem.h"
#include "wordmodel.h"
#include "settings.h"
#include "wordlistwidget.h"
// #include <poppler-qt6.h>
#include <iostream>
#include <algorithm>
#include <format>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

#include <poppler-qt6.h>

#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)

#include <poppler-qt5.h>

#else

#include <poppler-qt4.h>

#endif  // QT_VERSION
#include <poppler-annotation.h>
#include <QTextEdit>
#include <QListWidget>
#include <QListView>
#include <QStringListModel>
#include <QLabel>
#include <QFileInfo>
#include <QPushButton>

#include <QTransform>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>

#include <QToolButton>
#include <QMenu>
#include <QLineEdit>

#include <QDebug>

using namespace std;

namespace {
QString getWord(QString &carried, QString cur) {
    auto orig = cur.toStdString();
    if (cur.endsWith("-")) {
        cur.chop(1);
        carried += cur;
        return "";
    }
    if (!carried.isEmpty()) {
        cur = carried + cur;
        carried.clear();
    }

    string res;
    size_t i = 0;
    for (; i < orig.size(); i++) {
        if (!isalpha(orig[i])) {
            continue;
        } else {
            break;
        }
    }
    for (; i < orig.size(); i++) {
        if (isalpha(orig[i])) {
            res.push_back(orig[i]);
        } else {
            break;
        }
    }

    return QString::fromStdString(res);
}

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
}  // namespace
class Mainwindow::Private {
  public:
    // settings
    double scale = 2.5;

    // gui
    QTextEdit *x{nullptr};
    QLabel *label{nullptr};
    // QListWidget *listwidget{nullptr};

    WordlistWidget *wordwgt{nullptr};
    QListView *wordlistview{nullptr};
    WordModel *sourceModel{nullptr};
    WordSortFilterProxyModel *proxyModel{nullptr};
    WordModel::COLUMN_NO sortorder{WordModel::COLUMN_POS_IN_PAGE};

    QCheckBox *btn_showdict{nullptr};
    QCheckBox *btn_showConciseWordOnly{nullptr};
    QCheckBox *btn_showConciseOnly{nullptr};
    QPushButton *btn_scanscope;


    // beware, it's 1-idnexed.
    QLineEdit *edit_setPage{nullptr};
    QLabel *label_showPageNo{nullptr};

    // pdf dispay helper
    QTransform normalizedTransform;
    QSizeF pageViewSize;
    QSizeF pageSize;

    // pdf
    QString filename;
    int pageno{0};
    int pagewidth = -1;

    // #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    std::unique_ptr<Poppler::Page> pdfpage{nullptr};
    std::unique_ptr<Poppler::Document> document{nullptr};
    Outline_t outline;
    // #else
    //     Poppler::Page *pdfpage{nullptr};
    //     Poppler::Document *document{nullptr};
    // #endif

    std::unique_ptr<TextSave> wordstore{nullptr};
    // data, should be consistent.
    QSet<QString> words_knew;
    QSet<QString> words_ignore;

    QStringList words_page_all;          // all words in current page.
    QStringList words_page_afterFilter;  // after filter.
    set<QString> words_page_afterFilter_set;
    WordItemMap wordstruct_in_page;

    QStringList words_docu_all;          // all words in current page.
    QStringList words_docu_afterFilter;  // after filter.
    //
    bool scopeIsPage{true};

    ~Private() {
        cout << __PRETTY_FUNCTION__ << endl;
        delete x;
    }
};

Mainwindow::Mainwindow() {
    d = new Private;
    auto base = new QWidget;
    auto baseVbox = new QVBoxLayout;
    base->setLayout(baseVbox);

    auto splitter = new QSplitter(this);
    // d->listwidget = new QListWidget(this);
    d->wordstore = make_unique<TextSave>();
    if(0){
        // TODO: about wordlist's view, two versions are needed.
        //  1. listview. fixed sort order, may configrued in setting,
        //  2. detail table view, with frequence, meaning, hardlevel,
        //
        d->wordlistview = new QListView(this);
        d->sourceModel = new WordModel( this);
        d->sourceModel->setupModelData(&d->wordstruct_in_page);
        d->proxyModel = new WordSortFilterProxyModel(this);
        d->proxyModel->setSourceModel(d->sourceModel);

        d->wordlistview->setModel(d->proxyModel);
        d->wordlistview->setSelectionMode(QAbstractItemView::ExtendedSelection);
        d->wordlistview->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(d->wordlistview, &QWidget::customContextMenuRequested,
                [this](auto &&pos) {
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
                        std::sort(sels.begin(), sels.end(),
                                  [](auto &&l, auto &&r) { return l.row() > r.row(); });
                        menu->addAction("knew", [this, sels]() {
                            for (auto idx : sels) {
                                auto src_idx = d->proxyModel->mapToSource(idx);
                                auto *ptr = src_idx.internalPointer();
                                auto *item = static_cast<WordItem *>(ptr);
                                item->wordlevel = WORD_IS_KNOWN;
                                auto word = item->content;
                                d->words_knew.insert(word);
                                d->wordstore->addWordToKnown(word);
                                d->proxyModel->updateFilter();
                                qDebug()<<QString("mark %1 as knew.").arg(word);
                            }
                        });
                        menu->addAction("ignore", [this, sels]() {
                            for (auto idx : sels) {
                                auto src_idx = d->proxyModel->mapToSource(idx);
                                auto *ptr = src_idx.internalPointer();
                                auto *item = static_cast<WordItem *>(ptr);
                                item->wordlevel = WORD_IS_IGNORED;
                                auto word = item->content;
                                d->words_ignore.insert(word);
                                d->wordstore->addWordToIgnore(word);
                                d->proxyModel->updateFilter();
                                qDebug()<<QString("mark %1 as ignore.").arg(word);
                            }
                            // auto cur = d->listview.
                            auto selectionmodel = d->wordlistview->selectionModel();
                            for (auto idx : selectionmodel->selectedIndexes()) {
                                auto data = d->wordlistview->model()->itemData(idx);
                                d->words_ignore.insert(data[0].toString());
                                d->wordstore->addWordToIgnore(data[0].toString());
                            }
                            removeSeletedRowsFromView(d->wordlistview);
                        });
                        menu->addAction("add to dict", [this, sels]() {
                            for (auto idx : sels) {
                                auto src_idx = d->proxyModel->mapToSource(idx);
                                auto *ptr = src_idx.internalPointer();
                                auto *item = static_cast<WordItem *>(ptr);
                                item->wordlevel = WORD_IS_LEARNING;
                                auto word = item->content;
                                d->words_ignore.insert(word);
                                d->wordstore->addWordToIgnore(word);
                                d->proxyModel->updateFilter();
                            }
                        });
                    }

                    menu->exec(mapToParent(pos));
                });
    }
    // d->listwidget->setModelColumn
    auto wordwgt = new QWidget;
    d->wordwgt = new WordlistWidget(this);
    d->wordwgt->setupModel(&d->wordstruct_in_page);
    connect(this, &Mainwindow::pageLoadBefore, d->wordwgt, &WordlistWidget::onPageLoadBefore);
    connect(this, &Mainwindow::PageLoadDone, d->wordwgt, &WordlistWidget::onPageLoadAfter);
   if(0) {
        auto lay = new QVBoxLayout;
        wordwgt->setLayout(lay);
        {
            d->btn_showdict = new QCheckBox("Show word in dict");
            lay->addWidget(d->btn_showdict);
            connect(d->btn_showdict, &QAbstractButton::clicked, this,
                    &Mainwindow::update_filter);
        }

        {
            d->btn_showConciseWordOnly = new QCheckBox("Show concise word only");
            // d->btn_showConciseWordOnly->setChecked();
            lay->addWidget(d->btn_showConciseWordOnly);
            connect(d->btn_showConciseWordOnly, &QAbstractButton::clicked, this,
                    &Mainwindow::update_filter);
        }

        {
            d->btn_scanscope = new QPushButton("Switch between doc-page");
            lay->addWidget(d->btn_scanscope);

            connect(d->btn_scanscope, &QPushButton::clicked, this, [this]() {
                this->d->scopeIsPage = !this->d->scopeIsPage;
                update_filter();
            });
        }
        lay->addWidget(d->wordlistview);
    }
    auto pageShower = new QWidget(this);
    auto pageLay = new QVBoxLayout;
    {
        d->label = new QLabel(this);
        pageLay->addWidget(d->label);
    }

    pageShower->setLayout(pageLay);

    splitter->addWidget(d->wordwgt);
    splitter->addWidget(pageShower);

    {
        // as toolbar.
        auto toolbar_lay = new QHBoxLayout;

        auto btn_showoutline = new QToolButton(this);
        btn_showoutline->setIcon(QIcon::fromTheme("arrow-left"));
        btn_showoutline->setIconSize({32, 32});
        toolbar_lay->addWidget(btn_showoutline);
        connect(btn_showoutline, &QAbstractButton::clicked, this, [=]() {
            // wordwgt->isHidden();
            d->wordwgt->setHidden(!d->wordwgt->isHidden());
        });
        d->edit_setPage = new QLineEdit(this);
        d->edit_setPage->setFixedWidth(100);
        d->edit_setPage->setAlignment(Qt::AlignRight);
        toolbar_lay->addWidget(d->edit_setPage);
        // toolbar_lay->addWidget(new QLabel(" of "));
        d->label_showPageNo = new QLabel(this);
        toolbar_lay->addWidget(d->label_showPageNo);
        connect(d->edit_setPage, &QLineEdit::returnPressed, this, [this]() {
            auto txt = d->edit_setPage->text();
            this->go_to(txt.toInt() - 1);
        });

        auto btn1 = new QToolButton(this);
        btn1->setIcon(QIcon::fromTheme("arrow-left"));
        btn1->setIconSize({32, 32});
        toolbar_lay->addWidget(btn1);
        connect(btn1, &QAbstractButton::clicked, this, &Mainwindow::go_previous);

        auto btn2 = new QToolButton(this);
        btn2->setIcon(QIcon::fromTheme("arrow-right"));
        btn2->setIconSize({32, 32});
        toolbar_lay->addWidget(btn2);
        connect(btn2, &QAbstractButton::clicked, this, &Mainwindow::go_next);

        auto btn3 = new QToolButton(this);
        btn3->setIcon(QIcon::fromTheme("zoom-in"));
        btn3->setIconSize({32, 32});
        toolbar_lay->addWidget(btn3);
        connect(btn3, &QAbstractButton::clicked, this, &Mainwindow::scale_bigger);

        auto btn4 = new QToolButton(this);
        btn4->setIcon(QIcon::fromTheme("zoom-out"));
        btn4->setIconSize({32, 32});
        toolbar_lay->addWidget(btn4);
        connect(btn4, &QAbstractButton::clicked, this, &Mainwindow::scale_smaller);

        auto btn5 = new QPushButton("xx");
        btn5->setIconSize({32, 32});
        toolbar_lay->addWidget(btn5);
        toolbar_lay->addStretch(1);
        baseVbox->addLayout(toolbar_lay);
    }
    baseVbox->addWidget(splitter);

    this->setCentralWidget(base);

    load_settings();
    this->resize(100, 100);
}

void Mainwindow::openFile(const QString &filename) {
    qDebug() << "should load file:" << filename;

    if (QFileInfo(filename).exists() == false) {
        cout << "load file fail, file not exist" << endl;
        return;
    }

    d->document = std::unique_ptr<Poppler::Document>(Poppler::Document::load(filename));
    d->label_showPageNo->setText(QString(" of %1").arg(d->document->numPages()));
    d->edit_setPage->setValidator(new QIntValidator(1, d->document->numPages(), this));
    // cout <<"backend: "<< d->document->availableRenderBackends().size()<<endl;
    // cout << "backend: now"<< d->document->renderBackend() << endl;
    // d->document->setRenderBackend(Poppler::Document::QPainterBackend);
    d->document->setRenderHint(Poppler::Document::Antialiasing);
    d->document->setRenderHint(Poppler::Document::TextAntialiasing);
    d->document->outline();
    load_outline();
    display_outline();
    d->pagewidth = d->label->width();
    // this function will only run once, update later.
    d->words_docu_all = words_forDocument();
    test_scan_annotations();
    this->go_to(0);
}
Mainwindow::~Mainwindow() {
    delete d;
    d = nullptr;
    cout << __PRETTY_FUNCTION__ << endl;
}

void Mainwindow::go_to(int n) {
    int page_max = this->d->document->numPages();
    int pageno = n;
    if (pageno < 0) pageno = 0;
    if (pageno >= page_max) {
        pageno = page_max - 1;
    }
    // wont turn page.
    if (pageno == d->pageno) return;
    this->load_page(pageno);
}

void Mainwindow::go_previous() { this->go_to(d->pageno - 1); }

void Mainwindow::go_next() { this->go_to(d->pageno + 1); }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void Mainwindow::load_page(int n) {
    d->pageno = n;
    d->pdfpage = d->document->page(d->pageno);
    cout << "load page:" << n << endl;

    d->words_page_all = words_forCurPage();
    update_filter();
    // d->words_page_afterFilter = do_filter(d->words_page_all);


    // highlight word.
    auto myann = new Poppler::HighlightAnnotation;
    myann->setHighlightType(Poppler::HighlightAnnotation::Highlight);

    // myann->setBoundary(region);
    //
    const QList<Poppler::HighlightAnnotation::Quad> quads = {
        {{{0, 0.1}, {0.2, 0.3}, {0.4, 0.5}, {0.6, 0.7}}, false, false, 0},
        // {{{0.8, 0.9}, {0.1, 0.2}, {0.3, 0.4}, {0.5, 0.6}}, true, false, 0.4}
    };
    myann->setHighlightQuads(quads);
    Poppler::Annotation::Style styl;
    styl.setColor(Qt::red);
    styl.setOpacity(0.5);
    myann->setStyle(styl);

    // d->pdfpage->addAnnotation(myann);
    double width = d->pdfpage->pageSizeF().width();
    double height = d->pdfpage->pageSizeF().height();
    // d->pdfpage->removeAnnotation();
    if (0) {
        for (auto &w : d->words_page_afterFilter) {
            auto tx = d->pdfpage->textList();
            QString carried;
            for (auto i = tx.begin(); i < tx.end(); i++) {
                auto cur = i->get()->text().simplified();
                auto rect = i->get()->boundingBox();

                qDebug() << QString("rect: (%1, %2, %3, %4)")
                                .arg(rect.top() / height)
                                .arg(rect.left() / width)
                                .arg(rect.right() / width)
                                .arg(rect.bottom() / height);

                if (auto res = getWord(carried, cur); res.isEmpty()) {
                    continue;
                } else {
                    cur = res;
                    if (cur == w) {
                        auto region = i->get()->boundingBox();

                        auto myann = new Poppler::HighlightAnnotation;
                        myann->setHighlightType(Poppler::HighlightAnnotation::Underline);
                        myann->setBoundary(region);
                        d->pdfpage->addAnnotation(myann);
                    }
                }
            }
        }
    }
    update_image();
}
#else
void Mainwindow::load_page_before() {
    for (auto i : d->wordstruct_in_page.keys()) {
        auto &hl = d->wordstruct_in_page[i].highlight;
        for (auto i = hl.begin(); i != hl.end(); i++) {
            if (i->second != nullptr) {
                d->pdfpage->removeAnnotation(i->second);
                i->second = nullptr;
            }
        }
    }
    // d->sourceModel->reset_data_before();
    emit pageLoadBefore();
    d->wordstruct_in_page.clear();
}

void Mainwindow::load_page_after() {
    // cout << "now have word: " << d->wordstruct_in_page.size()<<endl;
    d->words_page_all = words_forCurPage();
    update_filter();
    // d->sourceModel->reset_data_after();
    for (auto i : d->wordstruct_in_page.keys()) {
        auto &hl = d->wordstruct_in_page[i].highlight;
        auto is_visible = d->wordstruct_in_page[i].isVisible();
        for (auto i = hl.begin(); i != hl.end(); i++) {
            if (is_visible && i->second == nullptr) {
                i->second = this->make_highlight(i->first);
            }
        }
    }
    // TODO: necessary? maybe for the selection state.
    // d->wordlistview->reset();
    // d->proxyModel->sort(WordModel::COLUMN_WORD);
    // d->proxyModel->sort(d->sortorder);
    emit PageLoadDone();
    update_image();
}

void Mainwindow::load_page(int n) {
    assert(d->pageno != n);

    //1. before swtich page, release load for current page.
    load_page_before();

    //2. load new page, directly from the qpoppler, draw it for display.
    d->pageno = n;
    d->edit_setPage->setText(QString("%1").arg(d->pageno + 1));
    // d->pdfpage = d->document->page(d->pageno);
    d->pdfpage = std::unique_ptr<Poppler::Page>(d->document->page(d->pageno));
    d->pageSize = d->pdfpage->pageSizeF();
    d->normalizedTransform.reset();
    d->normalizedTransform.scale(d->pageSize.width(), d->pageSize.height());

    cout << "load page:" << n << endl;

    update_image();

    //3. do heavy job after the load.
    //calcualte the words after filter, and redraw.
    load_page_after();
}
#endif

void Mainwindow::setupBtns() {}
void Mainwindow::scale_bigger() {
    if (d->scale >= 5.0) {
        return;
    }
    d->scale += 0.2;
    Settings::instance()->setPageScale(d->scale);
    update_image();
}

void Mainwindow::scale_smaller() {
    if (d->scale <= 0.2) {
        return;
    }
    d->scale -= 0.1;
    Settings::instance()->setPageScale(d->scale);

    update_image();
}

QStringList Mainwindow::do_filter(const QStringList &wordlist) {
    QStringList filtered_wordlist;
    if (d->scopeIsPage) {
        for (auto word : d->wordstruct_in_page.keys()) {
            // auto word = d->wordstruct_in_page[i].content;
            if (d->wordstore->isKnown(word)) {
                d->wordstruct_in_page[word].wordlevel = WORD_IS_KNOWN;
            }
            if (d->wordstore->isIgnored(word)) {
                d->wordstruct_in_page[word].wordlevel = WORD_IS_IGNORED;
            }
            if (d->wordstore->isInDict(word)) {
                d->wordstruct_in_page[word].wordlevel = WORD_IS_LEARNING;
            }
            // d->wordstruct_in_page[word].isKnown = d->wordstore->isKnown(word);
            // d->wordstruct_in_page[word].isIgnored = d->wordstore->isIgnored(word);
            // d->wordstruct_in_page[word].isIndict = d->wordstore->isInDict(word);
        }
    }

    for (auto word : wordlist) {
        // if (d->words_knew.contains(i) || d->words_ignore.contains(i)) continue;
        if (d->wordstore->isKnown(word) && !this->shouldShowWordType(KNEW)) continue;
        if (d->wordstore->isIgnored(word) && !this->shouldShowWordType(IGNORED)) continue;
        if (d->wordstore->isInDict(word) && !this->shouldShowWordType(DICT)) continue;
        if (d->btn_showConciseWordOnly and d->btn_showConciseWordOnly->isChecked()) {
            QStringList suffiexes = {"er", "ing", "ed", "s",  "ful",
                                     "ly", "est", "en", "ish"};
            auto it = std::find_if(suffiexes.begin(), suffiexes.end(),
                                   [word](auto &&sf) { return word.endsWith(sf); });
            if (it != suffiexes.end() or word[0].isUpper()) {
                continue;
            }
        }

        filtered_wordlist.push_back(word);
    }
    return filtered_wordlist;
}

void Mainwindow::update_filter() {
    if (d->scopeIsPage) {
        d->words_page_afterFilter = do_filter(d->words_page_all);
    } else {
        d->words_docu_afterFilter = do_filter(d->words_docu_all);
    }
}

QStringList Mainwindow::words_forCurPage() {
    auto tx = d->pdfpage->textList();

    QStringList suffixes{"s", "es", "ed", "ing"};

    QStringList res;
    QSet<QString> words_cache;
    QString carried;
    for (auto i = tx.begin(); i < tx.end(); i++) {
        auto cur = (*i)->text().simplified();
        (*i)->boundingBox();
        if (auto res = getWord(carried, cur); res.isEmpty()) {
            continue;
        } else {
            cur = res;
        }

        bool contains = true;
        do {
            if (words_cache.contains(cur)) break;
            if (cur[0].isUpper()) {
                for (auto &c : cur) {
                    c = c.toLower();
                }
                if (words_cache.contains(cur)) break;
            }
            if (cur[0].isDigit()) break;
            bool findRelated =
                std::any_of(suffixes.begin(), suffixes.end(), [&](auto &&sf) {
                    if (cur.endsWith(sf) && cur.size() > 3 + sf.size()) {
                        auto sf = cur.left(cur.size() - 2);
                        if (words_cache.contains(sf)) {
                            return true;
                        }
                    }
                    return false;
                });
            if (findRelated) break;
            contains = false;
        } while (0);
        auto anno_region = make_pair((*i)->boundingBox(), nullptr);
        if (contains) {
            d->wordstruct_in_page[cur].highlight.push_back(anno_region);
        } else {
            words_cache.insert(cur);
            res.push_back(cur);

            WordItem x;
            x.original = cur;
            x.content = cur;
            x.id = make_pair(d->pageno, std::distance(i, tx.begin()));
            x.id_page = d->pageno;
            x.id_idx = std::distance(i, tx.begin());
            // x.boundingbox = (*i)->boundingBox();
            x.highlight = {anno_region};
            d->wordstruct_in_page[cur] = std::move(x);
        }
    }
    return res;
}

// TODO: refactor words_forDocument/Page => use following two subroutine.
template <typename c_t>
bool findword(c_t c, QString &word) {
    return true;
}

QStringList Mainwindow::words_forDocument() {
    // auto pagex = d->pageno;
    QStringList suffixes{"s", "es", "ed", "ing"};
    QStringList res;
    QSet<QString> words_cache;
    QMap<QString, int> words_cntr;
    for (int i = 0; i < d->document->numPages(); i++) {
        auto page = d->document->page(i);

        QString carried = "";
        auto tx = page->textList();
        for (auto i = tx.begin(); i < tx.end(); i++) {
            auto cur = (*i)->text().simplified();
            if (auto res = getWord(carried, cur); res.isEmpty()) {
                continue;
            } else {
                cur = res;
            }

            bool contains = words_cache.contains(cur);

            // deal with word duplication in one page.
            if (contains) {
                words_cntr[cur]++;
            } else {
                words_cache.insert(cur);
                res.push_back(cur);
                words_cntr[cur] = 1;
            }
        }
    }
    res = words_cntr.keys();
    std::sort(res.begin(), res.end(),
              [&](auto &&a, auto &&b) { return words_cntr[a] > words_cntr[b]; });
    // test:
    // for (auto w : res) {
    //     cout << words_cntr[w] << ": " << w.toStdString() << endl;
    // }

    return res;
}

bool Mainwindow::shouldShowWordType(WordType wt) const {
    switch (wt) {
        case KNEW:
            return false;
            break;
        case DICT:
            return d->btn_showdict == nullptr ? false : d->btn_showdict->isChecked();
            break;
        case IGNORED:
            return false;
            break;
        case NEW:
            return true;
            break;
        default:
            assert(0);
            break;
    }
    return false;
}
void Mainwindow::update_image() {
    // FIXME: compile fail. even with `CONFIG += C++20`
    // cout << std::format("renderToImage parameters: width:{}", width)<<endl;

    // qDebug() << QString("renderToImage parameters: width:%1").arg(width);
    // qDebug() << "x :" << this->physicalDpiX();
    // qDebug() << "y :" << this->physicalDpiY();
    // auto [xres, yres] = make_pair(this->physicalDpiY() * d->scale, this->physicalDpiY()
    // * d->scale);
    auto [xres, yres] = make_pair(72, 72);
    auto page_view_size = d->pageSize * d->scale;

    auto image =
        d->pdfpage->renderToImage(d->scale * xres, d->scale * yres, 0, 0,
                                  page_view_size.width(), page_view_size.height());
    d->label->setPixmap(QPixmap::fromImage(image));
}
void Mainwindow::test_scan_annotations() {
    for (int i = 0; i < d->document->numPages(); i++) {
        auto page = d->document->page(i);
        auto annos = page->annotations();
        if (annos.size()) {
            qDebug() << QString("page %1 has %2 annotations.").arg(i).arg(annos.size());
        }
    }
}
void Mainwindow::load_settings() { d->scale = Settings::instance()->pageScale(); }

void load_section_cur(Section &section, Poppler::OutlineItem &item) {
    section.title = item.name();
    auto dest = item.destination();
    if (dest) section.link.page = dest->pageNumber();
    for (auto i : item.children()) {
        Section sub;
        load_section_cur(sub, i);
        section.children.push_back(sub);
    }
}

void Mainwindow::load_outline() {
    d->outline.clear();

    auto items = d->document->outline();
    if (items.isEmpty()) {
        cout << "no outine available.";
    }
    for (auto i : items) {
        Section cur_section;
        load_section_cur(cur_section, i);
        if (cur_section.link.page == -1) {
            // TODO:
            // maybe exit from here.
        }
        d->outline.push_back(cur_section);
    }
}

void display_section_cur(const Section &sect, int depth = 0) {
    for (int i = 0; i < depth; i++) cout << "    ";  // use indent as layer indicator.
    qDebug() << QString("level %1 %2 at page %3")
                    .arg(depth)
                    .arg(sect.title)
                    .arg(sect.link.page);
    for (auto sub : sect.children) {
        display_section_cur(sub, depth + 1);
    }
}
void Mainwindow::display_outline() {
    for (auto sec : d->outline) {
        display_section_cur(sec);
    }
}
Poppler::HighlightAnnotation *Mainwindow::make_highlight(QRectF region) {
    auto boundary = d->normalizedTransform.inverted().mapRect(region);
    QList<Poppler::HighlightAnnotation::Quad> quads;
    {
        Poppler::HighlightAnnotation::Quad quad{{}, true, true, 0.1};
        quad.points[0] = boundary.topLeft();
        quad.points[1] = boundary.topRight();
        quad.points[2] = boundary.bottomRight();
        quad.points[3] = boundary.bottomLeft();
        quads.push_back(quad);
    }

    Poppler::Annotation::Style styl;
    styl.setColor(Qt::red);
    styl.setOpacity(0.5);

    auto myann = new Poppler::HighlightAnnotation;
    // myann->setHighlightType(Poppler::HighlightAnnotation::Underline);
    myann->setHighlightQuads(quads);
    myann->setBoundary(boundary);

    myann->setStyle(styl);
    d->pdfpage->addAnnotation(myann);
    return myann;
}
