#include "mainwindow.h"
#include "words.h"
// #include <poppler-qt6.h>
#include <iostream>
#include <algorithm>
#include <format>

#include <poppler/qt5/poppler-qt5.h>
#include <poppler/qt5/poppler-annotation.h>
#include <QTextEdit>
#include <QListWidget>
#include <QListView>
#include <QStringListModel>
#include <QLabel>
#include <QFileInfo>
#include <QPushButton>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>

#include <QToolButton>
#include <QMenu>

#include <QDebug>

using namespace std;

namespace {
QString getWord(QString &carried, QString cur) {
    if (cur.endsWith("-")) {
        cur.chop(1);
        carried += cur;
        return "";
    }
    if (!carried.isEmpty()) {
        cur = carried + cur;
        carried.clear();
    }

    string cur_word;
    // deal with punc and numbers.
    for (auto c : cur.toStdString()) {
        if (isalpha(c) or isdigit(c) or c == '-') {
            cur_word.push_back(c);
        }
        if (c == '\'') break;
    }
    if (cur_word.empty()) {
        return "";
    }
    return QString::fromStdString(cur_word);
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
    QListView *listview{nullptr};
    QStringListModel *model{nullptr};
    QCheckBox *btn_showdict{nullptr};
    QCheckBox *btn_showConciseWordOnly{nullptr};
    QCheckBox *btn_showConciseOnly{nullptr};
    QPushButton *btn_about;

    QPushButton *btn_scanscope;

    // pdf
    QString filename;
    int pageno{0};
    int pagewidth = -1;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    std::unique_ptr<Poppler::Page> pdfpage{nullptr};
    std::unique_ptr<Poppler::Document> document{nullptr};
#else
    Poppler::Page *pdfpage{nullptr};
    Poppler::Document *document{nullptr};
#endif

    std::unique_ptr<TextSave> wordstore{nullptr};
    // data, should be consistent.
    QSet<QString> words_knew;
    QSet<QString> words_ignore;

    QStringList words_page_all;          // all words in current page.
    QStringList words_page_afterFilter;  // after filter.
    set<QString> words_page_afterFilter_set;
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
    auto splitter = new QSplitter(this);
    // d->listwidget = new QListWidget(this);
    d->wordstore = make_unique<TextSave>();
    {
        d->listview = new QListView(this);
        d->model = new QStringListModel();
        d->listview->setModel(d->model);
        d->listview->setSelectionMode(QAbstractItemView::ExtendedSelection);
        d->listview->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(d->listview, &QWidget::customContextMenuRequested, [this](auto &&pos) {
            auto menu = new QMenu;
            menu->addAction("test");
            menu->addAction("knew", [this]() {
                // auto cur = d->listview.
                auto selectionmodel = d->listview->selectionModel();
                for (auto idx : selectionmodel->selectedIndexes()) {
                    auto data = d->model->itemData(idx);
                    d->words_knew.insert(data[0].toString());
                    d->wordstore->addWordToKnown(data[0].toString());
                }
                update_filter();
                cout << selectionmodel->selectedIndexes().size() << endl;
            });
            menu->addAction("ignore", [this]() {
                // auto cur = d->listview.
                auto selectionmodel = d->listview->selectionModel();
                for (auto idx : selectionmodel->selectedIndexes()) {
                    auto data = d->model->itemData(idx);
                    d->words_ignore.insert(data[0].toString());
                    d->wordstore->addWordToIgnore(data[0].toString());
                }
                update_filter();
            });
            menu->addAction("add to dict", [this]() {
                // auto cur = d->listview.
                auto selectionmodel = d->listview->selectionModel();
                for (auto idx : selectionmodel->selectedIndexes()) {
                    auto data = d->model->itemData(idx);
                    d->words_ignore.insert(data[0].toString());
                    d->wordstore->addWordToIgnore(data[0].toString());
                }
                update_filter();
            });

            menu->exec(mapToGlobal(pos));
        });
    }
    // d->listwidget->setModelColumn
    auto pageShower = new QWidget(this);
    auto pageLay = new QVBoxLayout;
    {
        {
            auto btn_lay = new QHBoxLayout;
            auto btn1 = new QPushButton("prev", this);
            btn_lay->addWidget(btn1);
            connect(btn1, &QPushButton::clicked, this, &Mainwindow::go_previous);
            auto btn2 = new QPushButton("next", this);
            btn_lay->addWidget(btn2);
            connect(btn2, &QPushButton::clicked, this, &Mainwindow::go_next);
            auto btn3 = new QPushButton("big", this);
            btn_lay->addWidget(btn3);
            connect(btn3, &QPushButton::clicked, this, &Mainwindow::scale_bigger);
            auto btn4 = new QPushButton("small", this);
            btn_lay->addWidget(btn4);
            connect(btn4, &QPushButton::clicked, this, &Mainwindow::scale_smaller);
            auto btn5 = new QPushButton("xx");
            btn_lay->addWidget(btn5);
            pageLay->addLayout(btn_lay);
            pageLay->addSpacerItem(new QSpacerItem(1, 1));
        }

        d->label = new QLabel(this);
        pageLay->addWidget(d->label);
    }

    pageShower->setLayout(pageLay);
    auto wordwgt = new QWidget;
    {
        auto lay = new QVBoxLayout;
        wordwgt->setLayout(lay);
        {
            d->btn_showdict = new QCheckBox("Show word in dict");
            lay->addWidget(d->btn_showdict);
            // connect(d->btn_showdict, &QPushButton::clicked, d->btn_showdict,
            // &QPushButton::toggle);
            connect(d->btn_showdict, &QAbstractButton::clicked, this,
                    &Mainwindow::update_filter);
        }

        if (1) {
            d->btn_showConciseWordOnly = new QCheckBox("Show concise word only");
            // d->btn_showConciseWordOnly->setChecked();
            lay->addWidget(d->btn_showConciseWordOnly);
            // connect(d->btn_showcapitalword, &QPushButton::clicked,
            // d->btn_showcapitalword, &QPushButton::toggle);
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
        if (0) {
            d->btn_showConciseOnly = new QCheckBox("Show word with captical word");
            d->btn_showConciseOnly->setChecked(true);
            lay->addWidget(d->btn_showConciseOnly);
            // connect(d->btn_showConciseOnly, &QPushButton::clicked,
            // d->btn_showConciseOnly, &QPushButton::toggle);
            connect(d->btn_showConciseOnly, &QAbstractButton::clicked, this,
                    &Mainwindow::update_filter);
        }
        lay->addWidget(d->listview);
    }

    splitter->addWidget(wordwgt);
    splitter->addWidget(pageShower);
    this->setCentralWidget(splitter);
    this->resize(100, 100);
}

void Mainwindow::openFile(const QString &filename) {
    qDebug() << "should load file:" << filename;

    if (QFileInfo(filename).exists() == false) {
        cout << "load file fail, file not exist" << endl;
        return;
    }

    d->document = Poppler::Document::load(filename);
    // cout <<"backend: "<< d->document->availableRenderBackends().size()<<endl;
    // cout << "backend: now"<< d->document->renderBackend() << endl;
    // d->document->setRenderBackend(Poppler::Document::QPainterBackend);
    d->document->setRenderHint(Poppler::Document::Antialiasing);
    d->document->setRenderHint(Poppler::Document::TextAntialiasing);
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
    d->pageno = pageno;
    this->load_page(d->pageno);
}

void Mainwindow::go_previous() {
    cout << __func__ << endl;
    this->go_to(d->pageno - 1);
}

void Mainwindow::go_next() {
    cout << __func__ << endl;
    this->go_to(d->pageno + 1);
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void Mainwindow::load_page(int n) {
    d->pageno = n;
    d->pdfpage = d->document->page(d->pageno);
    cout << "load page:" << n << endl;

    d->words_page_all = words_forCurPage();
    update_filter();
    // d->words_page_afterFilter = do_filter(d->words_page_all);

    d->model->setStringList(d->words_page_afterFilter);

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
    d->pageno = n;
    d->pdfpage = d->document->page(d->pageno);
    cout << "load page:" << n << endl;

    d->words_page_all = words_forCurPage();
    update_filter();
    // d->words_page_afterFilter = do_filter(d->words_page_all);

    d->model->setStringList(d->words_page_afterFilter);

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
                auto cur = (*i)->text().simplified();
                auto rect = (*i)->boundingBox();

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
                        auto region = (*i)->boundingBox();

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
#endif

void Mainwindow::setupBtns() {}
void Mainwindow::scale_bigger() {
    if (d->scale >= 5.0) {
        return;
    }
    d->scale += 0.2;
    update_image();
}

void Mainwindow::scale_smaller() {
    if (d->scale <= 0.1) {
        return;
    }
    d->scale -= 0.1;
    update_image();
}

QStringList Mainwindow::do_filter(const QStringList &wordlist) {
    QStringList filtered_wordlist;

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
        cout << __func__ << "before, size:" << d->words_page_afterFilter.size() << endl;
        d->words_page_afterFilter = do_filter(d->words_page_all);

        cout << __func__ << "after, size:" << d->words_page_afterFilter.size() << endl;
        d->model->setStringList(d->words_page_afterFilter);
    } else {
        cout << __func__ << "before, size:" << d->words_docu_afterFilter.size() << endl;
        d->words_docu_afterFilter = do_filter(d->words_docu_all);
        cout << __func__ << "before, size:" << d->words_docu_afterFilter.size() << endl;
        d->model->setStringList(d->words_docu_afterFilter);
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
        if (contains) {
        } else {
            words_cache.insert(cur);
            res.push_back(cur);
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
    int width = d->pagewidth;
    if (d->pagewidth == -1) {
        width = d->label->width() * 2;
    }
    width = d->label->width() * 2;

    // FIXME: compile fail. even with `CONFIG += C++20`
    // cout << std::format("renderToImage parameters: width:{}", width)<<endl;

    // qDebug() << QString("renderToImage parameters: width:%1").arg(width);
    // qDebug() << "x :" << this->physicalDpiX();
    // qDebug() << "y :" << this->physicalDpiY();
    // auto [xres, yres] = make_pair(this->physicalDpiY() * d->scale, this->physicalDpiY()
    // * d->scale);
    auto [xres, yres] = make_pair(72, 72);

    auto image = d->pdfpage->renderToImage(d->scale * xres, d->scale *yres, 0, 0,
                                           d->pdfpage->pageSize().width() * d->scale,
                                           d->pdfpage->pageSize().height() * d->scale);
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
