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
#include "pagecontainer.h"

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
    Poppler::Document *document{nullptr};

    // beware, it's 1-idnexed.
    QLineEdit *edit_setPage{nullptr};
    QLabel *label_showpage_cur{nullptr};

    // gui
    PageContainer *page_continer{nullptr};
    WordlistWidget *wordwgt{nullptr};

    std::shared_ptr<SqlSave> wordstore{nullptr};  // data, should be consistent.
    WordItemMap wordItems_in_page;
    QStringList words_docu_all;  // all words in current page.

    ~Private() { cout << __PRETTY_FUNCTION__ << endl; }
};

Mainwindow::Mainwindow() {
    d = new Private;
    auto base = new QWidget;
    auto baseVbox = new QVBoxLayout(base);

    d->wordstore = make_shared<SqlSave>();
    d->wordwgt = new WordlistWidget(this);
    d->wordwgt->setWordStore(d->wordstore);
    auto pageShower = new QWidget(this);
    auto pageLay = new QHBoxLayout;
    {
        d->page_continer = new PageContainer(this);
        pageLay->addWidget(d->page_continer);
        pageLay->setStretch(0, 1);
    }
    d->wordwgt->setupModel(d->page_continer->focus()->getWordItems());

    pageShower->setLayout(pageLay);

    auto splitter = new QSplitter(this);
    splitter->addWidget(d->wordwgt);
    splitter->addWidget(pageShower);

    auto toolbar = setupToolbar();
    baseVbox->addWidget(toolbar, 0);

    baseVbox->addWidget(splitter, 1);

    this->setCentralWidget(base);

    load_settings();
    this->resize(100, 100);

    connect(d->page_continer->focus(), &PageView::pageLoadBefore, d->wordwgt,
            &WordlistWidget::onPageLoadBefore);
    connect(d->page_continer->focus(), &PageView::PageLoadDone, d->wordwgt,
            &WordlistWidget::onPageLoadAfter);
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
    // this function will only run once, update later.
    d->words_docu_all = words_forDocument();
    d->page_continer->setDocument(d->document);
    test_load_outline();
    test_scan_annotations();
}
Mainwindow::~Mainwindow() {
    delete d;
    d = nullptr;
    cout << __PRETTY_FUNCTION__ << endl;
}

QWidget *Mainwindow::setupToolbar() {
    QWidget *base = new QWidget(this);
    // as toolbar.
    auto toolbar_lay = new QHBoxLayout(base);

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
        // this->go_to(txt.toInt() - 1);
    });

    auto btn1 = new QToolButton(this);
    btn1->setIcon(QIcon::fromTheme("arrow-left"));
    btn1->setIconSize({32, 32});
    toolbar_lay->addWidget(btn1);
    // connect(btn1, &QAbstractButton::clicked, this, &Mainwindow::go_previous);

    auto btn2 = new QToolButton(this);
    btn2->setIcon(QIcon::fromTheme("arrow-right"));
    btn2->setIconSize({32, 32});
    toolbar_lay->addWidget(btn2);
    // connect(btn2, &QAbstractButton::clicked, this, &Mainwindow::go_next);

    auto btn3 = new QToolButton(this);
    btn3->setIcon(QIcon::fromTheme("zoom-in"));
    btn3->setIconSize({32, 32});
    toolbar_lay->addWidget(btn3);
    // connect(btn3, &QAbstractButton::clicked, this, &Mainwindow::scale_bigger);

    auto btn4 = new QToolButton(this);
    btn4->setIcon(QIcon::fromTheme("zoom-out"));
    btn4->setIconSize({32, 32});
    toolbar_lay->addWidget(btn4);
    // connect(btn4, &QAbstractButton::clicked, this, &Mainwindow::scale_smaller);

    auto btn5 = new QPushButton("xx");
    btn5->setIconSize({32, 32});
    toolbar_lay->addWidget(btn5);
    toolbar_lay->addStretch(1);

    return base;
}

QStringList Mainwindow::words_forDocument() {
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

void Mainwindow::test_scan_annotations() {
    for (int i = 0; i < d->document->numPages(); i++) {
        auto page = d->document->page(i);
        auto annos = page->annotations();
        if (annos.size()) {
            qDebug() << QString("page %1 has %2 annotations.").arg(i).arg(annos.size());
        }
    }
}
void Mainwindow::load_settings() {
    // d->scale = Settings::instance()->pageScale();
}

void Mainwindow::test_load_outline() {
    auto outline = Outline();
    outline.setDocument(d->document);
    outline.load_outlie();
    outline.display_outline();
}
