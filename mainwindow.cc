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
#include <QThread>
#include <QCoreApplication>

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

    // beware, it's 1-idnexed.
    QLineEdit *edit_setPage{nullptr};
    QLabel *label_showpage_cur{nullptr};

    // pdf dispay helper
    QTransform normalizedTransform;
    QSizeF pageSize;

    // pdf
    QString filename;
    int page_cur{0};
    int pagewidth = -1;

    // #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    std::shared_ptr<Poppler::Page> pdfpage{nullptr};
    Poppler::Document *document{nullptr};
    // #else
    //     Poppler::Page *pdfpage{nullptr};
    //     Poppler::Document *document{nullptr};
    // #endif

    std::shared_ptr<SqlSave> wordstore{nullptr};
    // data, should be consistent.

    QStringList words_page_all;  // all words in current page.
    WordItemMap wordItems_in_page;

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
    d->wordstore = make_shared<SqlSave>();
    d->wordwgt = new WordlistWidget(this);
    d->wordwgt->setWordStore(d->wordstore);
    d->wordwgt->setupModel(&d->wordItems_in_page);
    connect(d->wordwgt, &WordlistWidget::markItemsLevel, this,
            [this](QStringList words, wordlevel_t lv) {
                qDebug() << "mainwindow thread: " << QThread::currentThreadId();
                // qDebug() << "mark size: " << items.size();
                for (auto &i : words) {
                    qDebug() << "mark to x, with: " << i;
                    d->wordItems_in_page[i]->wordlevel = lv;
                }
            });
    connect(this, &Mainwindow::pageLoadBefore, d->wordwgt,
            &WordlistWidget::onPageLoadBefore);
    connect(this, &Mainwindow::PageLoadDone, d->wordwgt,
            &WordlistWidget::onPageLoadAfter);
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
        d->label_showpage_cur = new QLabel(this);
        toolbar_lay->addWidget(d->label_showpage_cur);
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
    d->document = Poppler::Document::load(filename);

    d->label_showpage_cur->setText(QString(" of %1").arg(d->document->numPages()));
    d->edit_setPage->setValidator(new QIntValidator(1, d->document->numPages(), this));
    // cout <<"backend: "<< d->document->availableRenderBackends().size()<<endl;
    // cout << "backend: now"<< d->document->renderBackend() << endl;
    // d->document->setRenderBackend(Poppler::Document::QPainterBackend);
    d->document->setRenderHint(Poppler::Document::Antialiasing);
    d->document->setRenderHint(Poppler::Document::TextAntialiasing);
    test_load_outline();
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
    int page_cur = n;
    if (page_cur < 0) pageno = 0;
    if (page_cur >= page_max) {
        page_cur = page_max - 1;
    }
    // wont turn page.
    if (page_cur == d->pageno) return;
    this->load_page(page_cur);
}

void Mainwindow::go_previous() { this->go_to(d->page_cur - 1); }

void Mainwindow::go_next() { this->go_to(d->page_cur + 1); }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void Mainwindow::load_page(int n) {
    d->page_cur = n;
    d->pdfpage = d->document->page(d->page_cur);
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

void Mainwindow::load_page(int n) {
    assert(d->page_cur != n);
    // 1. before swtich page, release load for current page.
    for (auto i : d->wordItems_in_page.keys()) {
        auto &hl = d->wordItems_in_page[i]->highlight;
        for (auto i = hl.begin(); i != hl.end(); i++) {
            if (i->second != nullptr) {
                d->pdfpage->removeAnnotation(i->second);
                i->second = nullptr;
            }
        }
    }
    emit pageLoadBefore();
    d->wordItems_in_page.clear();
    // d->wordwgt->update();
    QCoreApplication::processEvents();

    // 2. load new page, directly from the qpoppler, draw it for display.
    d->page_cur = n;
    d->edit_setPage->setText(QString("%1").arg(d->page_cur + 1));
    // d->pdfpage = d->document->page(d->page_cur);
    d->pdfpage = std::unique_ptr<Poppler::Page>(d->document->page(d->page_cur));
    d->pageSize = d->pdfpage->pageSizeF();
    d->normalizedTransform.reset();
    d->normalizedTransform.scale(d->pageSize.width(), d->pageSize.height());
    cout << "load page:" << n << endl;

    update_image();

    // 3. do heavy job after the load.
    // calcualte the words after filter, and redraw.
    // cout << "now have word: " << d->wordstruct_in_page.size()<<endl;
    d->words_page_all = words_forCurPage();
    update_filter();
    for (auto i : d->wordItems_in_page.keys()) {
        auto &hl = d->wordItems_in_page[i]->highlight;
        auto is_visible = d->wordItems_in_page[i]->isVisible();
        for (auto i = hl.begin(); i != hl.end(); i++) {
            if (is_visible && i->second == nullptr) {
                i->second = this->make_highlight(i->first);
            }
        }
    }
    // TODO: necessary? maybe for the selection state.
    emit PageLoadDone();
    update_image();
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

QStringList Mainwindow::check_wordlevel(const QStringList &wordlist) {
    QStringList res;
    for (auto word : d->wordItems_in_page.keys()) {
        auto lv = d->wordstore->getWordLevel(word);
        d->wordItems_in_page[word]->wordlevel = lv;
        isValidLevel(lv);
    }

    return {};
}

void Mainwindow::update_filter() { check_wordlevel(d->words_page_all); }

QStringList Mainwindow::words_forCurPage() {
    auto tx = d->pdfpage->textList();

    QStringList suffixes{"s", "es", "ed", "ing"};

    QStringList res;
    QSet<QString> words_cache;
    QString carried;
    d->wordItems_in_page.values();
    for(auto &i: d->wordItems_in_page.values()){
        delete i;
    }
    d->wordItems_in_page.clear();
    for (auto i = tx.begin(); i < tx.end(); i++) {
        auto cur = (*i)->text();
        if(cur.size() > 1 and cur[0].isUpper() and cur[1].isUpper()){
            cur = cur.simplified();
        }
        if (auto res = getWord(carried, cur); res.isEmpty()) {
            continue;
        } else {
            cur = res;
        }

        bool contains = true;
        do {
            if (words_cache.contains(cur)) break;
            // if (cur[0].isUpper()) {
            //     for (auto &c : cur) {
            //         c = c.toLower();
            //     }
            //     if (words_cache.contains(cur)) break;
            // }
            if (cur[0].isDigit()) break;
            contains = false;
        } while (0);
        auto anno_region = make_pair((*i)->boundingBox(), nullptr);
        if (contains) {
            d->wordItems_in_page[cur]->highlight.push_back(anno_region);
        } else {
            words_cache.insert(cur);
            res.push_back(cur);

            WordItem *x = new WordItem;
            x->original = cur;
            x->content = cur;
            x->id = make_pair(d->page_cur, std::distance(i, tx.begin()));
            x->id_page = d->page_cur;
            x->id_idx = std::distance(i, tx.begin());
            // x.boundingbox = (*i)->boundingBox();
            x->highlight = {anno_region};
            d->wordItems_in_page[cur] = x;
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
    // auto pagex = d->page_cur;
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


void Mainwindow::test_load_outline() {
    auto outline = Outline();
    outline.setDocument(d->document);
    outline.load_outlie();
    outline.display_outline();
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
