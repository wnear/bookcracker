#include "mainwindow.h"
#include "words.h"
// #include <poppler-qt6.h>
#include <iostream>
#include <algorithm>
#include <format>

#include <poppler/qt6/poppler-qt6.h>
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

using namespace std;

namespace {
std::optional<QString> getWord(QString &carried, QString cur) {
    if (cur.endsWith("-")) {
        cur.chop(1);
        carried += cur;
        return nullopt;
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
        return nullopt;
    }
    return QString::fromStdString(cur_word);
}
}  // namespace
class Mainwindow::Private {
  public:
    // gui
    QTextEdit *x{nullptr};
    QLabel *label{nullptr};
    // QListWidget *listwidget{nullptr};
    QListView *listview{nullptr};
    QStringListModel *model{nullptr};
    QCheckBox *btn_showdict{nullptr};
    QCheckBox *btn_showcapitalword{nullptr};

    QPushButton *btn_scanscope;

    // pdf
    QString filename;
    int pageno{0};
    int pagewidth = -1;
    std::unique_ptr<Poppler::Page> pdfpage{nullptr};
    std::unique_ptr<Poppler::Document> document{nullptr};

    std::unique_ptr<TextSave> wordstore{nullptr};
    // data, should be consistent.
    QSet<QString> words_knew;
    QSet<QString> words_ignore;

    QStringList words_all;               // all words in current page.
    QStringList words_afterFilter;       // after filter.
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
            d->btn_showcapitalword = new QCheckBox("Show word with captical word");
            d->btn_showcapitalword->setChecked(true);
            lay->addWidget(d->btn_showcapitalword);
            // connect(d->btn_showcapitalword, &QPushButton::clicked,
            // d->btn_showcapitalword, &QPushButton::toggle);
            connect(d->btn_showcapitalword, &QAbstractButton::clicked, this,
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
    d->pagewidth = d->label->width();
    d->words_docu_all = words_forDocument();
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
    this->loadPage(d->pageno);
}

void Mainwindow::go_previous() {
    cout << __func__ << endl;
    this->go_to(d->pageno - 1);
}

void Mainwindow::go_next() {
    cout << __func__ << endl;
    this->go_to(d->pageno + 1);
}

void Mainwindow::loadPage(int n) {
    d->pageno = n;
    d->pdfpage = d->document->page(d->pageno);
    cout << "load page:" << n << endl;
    // int xres = 100;
    // int yres = 100;
    // int x = 0;
    // int y = 0;
    // int width = 100;
    // int height = 100;
    int width = d->pagewidth;
    if (d->pagewidth == -1) {
        width = d->label->width() * 2;
    }
    width = d->label->width() * 2;

    // FIXME: compile fail. even with `CONFIG += C++20`
    // cout << std::format("renderToImage parameters: width:{}", width)<<endl;

    qDebug() << QString("renderToImage parameters: width:%1").arg(width);
    auto image = d->pdfpage->renderToImage(120, 120, 0, 0, -1, d->label->height());
    d->label->setPixmap(QPixmap::fromImage(image));

    d->words_all = words_forCurPage();
    d->words_afterFilter = do_filter(d->words_all);

    d->model->setStringList(d->words_afterFilter);

    // highlight word.
    for (auto &w : d->words_afterFilter) {
        auto tx = d->pdfpage->textList();
        QString carried;
        for (auto i = tx.begin(); i < tx.end(); i++) {
            auto cur = i->get()->text().simplified();
            i->get()->boundingBox();
            if (auto res = getWord(carried, cur); res == nullopt) {
                continue;
            } else {
                cur = res.value();
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

void Mainwindow::setupBtns() {}
void Mainwindow::scale_bigger() {
    if (d->pagewidth == -1) {
        d->pagewidth = d->label->width();
    }
    if (d->pagewidth > 50) d->pagewidth -= 10;
}

void Mainwindow::scale_smaller() {
    if (d->pagewidth == -1) {
        d->pagewidth = d->label->width();
    }
    if (d->pagewidth < 5460) d->pagewidth += 10;
}

QStringList Mainwindow::do_filter(const QStringList &cur) {
    QStringList res;
    for (auto i : cur) {
        // if (d->words_knew.contains(i) || d->words_ignore.contains(i)) continue;
        if (d->wordstore->isKnown(i) && !this->shouldShowWordType(KNEW)) continue;
        if (d->wordstore->isIgnored(i) && !this->shouldShowWordType(IGNORED)) continue;
        if (d->wordstore->isInDict(i) && !this->shouldShowWordType(DICT)) continue;
        if (d->btn_showcapitalword and i[0].isUpper() and
            !d->btn_showcapitalword->isChecked()) {
            continue;
        }

        res.push_back(i);
    }
    return res;
}

void Mainwindow::update_filter() {
    if (d->scopeIsPage) {
        cout << __func__ << "before, size:" << d->words_afterFilter.size() << endl;
        d->words_afterFilter = do_filter(d->words_all);
        cout << __func__ << "after, size:" << d->words_afterFilter.size() << endl;
        d->model->setStringList(d->words_afterFilter);
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
        auto cur = i->get()->text().simplified();
        i->get()->boundingBox();
        if (auto res = getWord(carried, cur); res == nullopt) {
            continue;
        } else {
            cur = res.value();
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
            auto cur = i->get()->text().simplified();
            if (auto res = getWord(carried, cur); res == nullopt) {
                continue;
            } else {
                cur = res.value();
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
    for (auto w : res) {
        cout << words_cntr[w] << ": " << w.toStdString() << endl;
    }
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
