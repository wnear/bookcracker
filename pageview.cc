#include "pageview.h"
#include "worditem.h"
#include "words.h"

#include <iostream>
#include <string>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

// TODO:
// 0. auto scale. (scale to best view.)
//   0. size with value.
//   1. size with hidth/weight of item.
// 0. draw frame and shadow around the single page.
// x. use item as layoutitem, (two-page, or continuous mode)
// x. put qwidget in view. (e.g. put qtext, in qgraphics)
// 0. drag item. (look around)
// 1. parent-item relationship's existence.
//   0. setPos.
// 1. two-page
// 2. continuous page.
//

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

    std::string res;
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
}  // namespace

class PageView::Private {
  public:
    int page_cur{-1};
    double scale;
    int page_max;  //[0, pagemax] 0-based.
    WordItemMap wordItems_in_page;
    QList<Poppler::HighlightAnnotation *> annos;
    std::shared_ptr<SqlSave> wordstore{nullptr};

    Poppler::Document *document;
    QTransform normalizedTransform;

    std::shared_ptr<Poppler::Page> pdfpage{nullptr};
    QSizeF pageSize;
};

PageView::PageView(QWidget *parent) : QWidget(parent) {
    d = new Private;
    d->wordstore = make_shared<SqlSave>();

    this->setLayout(new QVBoxLayout);

    m_photoItem = new PageItem;
    m_photoItem->setPos(0, 0);

    m_scene = new QGraphicsScene(0, 0, 400, 400);
    m_scene->addItem(m_photoItem);
    m_scene->setSceneRect(m_photoItem->boundingRect());
    m_photoItem->setPos(0, 0);
    // scene->addText("hello world");

    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setBackgroundBrush(QColor(230, 200, 167));
    m_view->setWindowTitle("Drag and Drop Robot");
    m_view->show();
    // NOTE: a little tricky,  wait for gui inited, then autoscale.
    QCoreApplication::processEvents();
    // displayInfo();

    autoscale();

    {
        auto widget = new QWidget;
        auto lay = new QHBoxLayout(widget);
        this->layout()->addWidget(widget);
        auto btn_next = new QPushButton("next");
        auto btn_previous = new QPushButton("prev");
        auto btn_zoom_smaller = new QPushButton("zoom-in");
        auto btn_zoom_bigger = new QPushButton("zoom-out");
        auto btn_test = new QPushButton("test");

        lay->addWidget(btn_next);
        lay->addWidget(btn_previous);
        lay->addWidget(btn_zoom_smaller);
        lay->addWidget(btn_zoom_bigger);
        lay->addWidget(btn_test);
        connect(btn_next, &QPushButton::clicked, this, &PageView::go_next);
        connect(btn_previous, &QPushButton::clicked, this, &PageView::go_prev);
        connect(m_photoItem, &PageItem::photoChanged, this, &PageView::autoscale);

        connect(btn_zoom_smaller, &QAbstractButton::clicked, this, [this]() {
            m_item_sizescale -= 0.1;
            m_item_sizescale = std::max(0.3f, m_item_sizescale);
            update_image();
        });
        connect(btn_zoom_bigger, &QAbstractButton::clicked, this, [this]() {
            m_item_sizescale += 0.1;
            m_item_sizescale = std::min(5.0f, m_item_sizescale);
            update_image();
        });
        connect(btn_test, &QAbstractButton::clicked, this, [this]() {
            for (auto w : d->annos) {
                if (w) {
                    qDebug() << "remove anno.";
                    d->pdfpage->removeAnnotation(w);
                    w = nullptr;
                }
            }
            d->annos.clear();
            update_image();
        });
    }

    this->layout()->addWidget(m_view);
}

void PageView::setScale(float incr) {
    m_scale += incr;
    if (m_scale < 0.1) m_scale = 0.1;
}

void PageView::autoscale() {
    auto itemsize = m_photoItem->boundingRect();
    m_scene->setSceneRect(itemsize);
}

void PageView::displayInfo() const {
    auto crec = m_view->contentsRect();
    auto rect = m_view->rect();
    auto size = m_view->size();
    qDebug() << QString("contentsRect:") << crec;
    qDebug() << QString("rect:") << rect;
    qDebug() << QString("size:") << size;
}

QSize PageView::boardSize() const {
    return {m_view->contentsRect().width() - 2 * m_padding_leftright,
            m_view->contentsRect().height() - 2 * m_padding_topbottom};
}

void PageView::go_next() { go_to(d->page_cur + 1); }

void PageView::go_prev() { go_to(d->page_cur - 1); }

void PageView::go_to(int n) {
    if (n == -1 || n == d->page_max) return;
    // wont turn page.
    if (n == d->page_cur) return;
    this->load_page(n);
}

QList<std::pair<QString, QRectF>> display(Poppler::TextBox *tb) {
    QList<std::pair<QString, QRectF>> word_with_bounding;
    word_with_bounding.push_back({{}, {}});
    QString orig = tb->text();
    QChar last(' ');
    for (int i = 0; i < tb->text().size(); i++) {
        if (orig[i].isLetter()) {
            if (!last.isLetter()) {
                word_with_bounding.push_back({{}, {}});
            }
            word_with_bounding.back().first.push_back(orig[i]);
            word_with_bounding.back().second |= tb->charBoundingBox(i);
        }
        last = orig[i];
    }
    return word_with_bounding;
}

bool PageView::near(Poppler::TextBox *l, Poppler::TextBox *r) {
    auto dist_v = l->boundingBox().bottom() - r->boundingBox().bottom();

    if (dist_v == 0) {
        return true;
    } else {
        bool res = dist_v / m_scale < 30;
        if (res == false) qDebug() << "not near:" << dist_v << "," << m_scale;
        return dist_v * m_scale < 10;
    }
}

QStringList PageView::parsePage() {
    auto tx = d->pdfpage->textList();
    if (tx.isEmpty()) return {};
    QStringList suffixes{"s", "es", "ed", "ing"};

    QString sentence;

    QStringList res;
    QSet<QString> words_cache;
    QString carried;
    d->wordItems_in_page.values();
    for (auto &i : d->wordItems_in_page.values()) {
        delete i;
    }
    // d->pdfpage->DontSaveAndRestore
    d->wordItems_in_page.clear();

    sentence.push_back(tx[0]->text());

    Poppler::TextBox *tb_last{nullptr}, *tb_cur{nullptr};
    bool isSentenceStart{true};
    for (auto i = tx.begin(); i < tx.end(); i++) {
        // check last push is line end.
        if (tb_last == nullptr) {
            tb_last = tb_cur = *i;
        } else {
            tb_cur = *i;
        }

        if (tb_cur != tb_last) {
            if (tb_cur->text().back() == '.' or (not near(tb_last, tb_cur))) {
                sentence.push_back((*i)->text());
                // qDebug() << QString("sentence:%1").arg(sentence);
                sentence.clear();
            } else {
                if (sentence.isEmpty()) {
                    isSentenceStart = true;
                } else {
                    isSentenceStart = false;
                    sentence.push_back(" ");
                }
                sentence.push_back((*i)->text());
            }
        }
        tb_last = tb_cur;

#if 0
        //simple logic version of get sentence.
        auto cur = *i;
        auto next_tb = cur->nextWord();
        if (next_tb == nullptr   // for eol and page end, next_tb will be nullptr.
            or cur->text().back() == '.' ) {
            qDebug()<<QString("sentence:%1").arg(sentence);
            sentence.clear();
        } else {
            sentence.push_back(" ");
        }
#endif

        int el_idx = 0;
        for (auto [word_cur, bounding] : display(*i)) {
            el_idx++;
            QString word_clean{word_cur};
            if (word_cur.size() <= 1) continue;
            if (word_cur[0].isDigit()) continue;
            if (word_cur.size() > 1 and word_cur[0].isUpper() and
                (not word_cur[1].isUpper())) {
                word_clean = word_cur.toLower();
            }

            WordItem *curItem = nullptr;
            if (words_cache.contains(word_clean)) {
                curItem = d->wordItems_in_page[word_clean];
                assert(curItem != nullptr);
            } else {
                words_cache.insert(word_clean);
                curItem = new WordItem;
                curItem->word = word_clean;
                d->wordItems_in_page[word_clean] = curItem;
            }
            assert(curItem != nullptr);
            auto anno_region = make_pair(bounding, nullptr);
            curItem->highlight.push_back(anno_region);
            sentence_t sen_pos;
            sen_pos.word = word_cur;
            sen_pos.page = d->page_cur;
            sen_pos.location = std::distance(i, tx.begin());
            sen_pos.wordidx = el_idx;
            sen_pos.sentence = sentence;
            curItem->sentenceContext.push_back(sen_pos);
        }
    }
    return res;
}

void PageView::load_page(int n) {
    qDebug() << __PRETTY_FUNCTION__;
    qDebug() << "want load:" << n;
    if (d->page_cur == n) {
        qDebug() << "same page.";
        return;
    }
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
    // TODO: store some info of the d->wordItems_in_page to sql.
    // like if the word is in learning level, the dict word sentence context.
    emit pageLoadBefore();
    d->wordItems_in_page.clear();
    // d->wordwgt->update();
    QCoreApplication::processEvents();

    // 2. load new page, directly from the qpoppler, draw it for display.
    d->page_cur = n;
    qDebug() << "page size:" << m_pages.size();
    // d->edit_setPage->setText(QString("%1").arg(d->page_cur + 1));
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
    parsePage();
    update_filter();
    for (auto i : d->wordItems_in_page.keys()) {
        update_highlight(i);
    }
    // TODO: necessary? maybe for the selection state.
    emit PageLoadDone();
    update_image();
}

Poppler::HighlightAnnotation *PageView::make_highlight(QRectF region,
                                                       const QColor &color) {
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
    styl.setColor(color);
    styl.setOpacity(0.5);

    auto *myann = new Poppler::HighlightAnnotation;
    // myann->setHighlightType(Poppler::HighlightAnnotation::Underline);
    myann->setHighlightQuads(quads);
    myann->setBoundary(boundary);

    myann->setStyle(styl);
    d->pdfpage->addAnnotation(myann);
    d->annos.push_back(myann);
    return myann;
}

void PageView::update_highlight(const QString &word) {
    auto *item = d->wordItems_in_page[word];
    auto color = item->wordlevel == LEVEL_UNKOWN ? Qt::red : Qt::blue;
    auto &hl = item->highlight;
    auto visible = item->isVisible();
    for (auto i = hl.begin(); i != hl.end(); i++) {
        if (i->second != nullptr) {
            d->pdfpage->removeAnnotation(i->second);
            i->second = nullptr;
        }
        if (visible) i->second = this->make_highlight(i->first, color);
    }
}

void PageView::update_highlight(QStringList words) {
    for (auto w : words) {
        update_highlight(w);
    }
    update_image();
}

QStringList PageView::check_wordlevel() {
    QStringList res;
    for (auto word : d->wordItems_in_page.keys()) {
        auto lv = d->wordstore->getWordLevel(word);
        d->wordItems_in_page[word]->wordlevel = lv;
        isValidLevel(lv);
    }

    return {};
}

void PageView::update_filter() { check_wordlevel(); }
void PageView::update_image() {
    // FIXME: compile fail. even with `CONFIG += C++20`
    // cout << std::format("renderToImage parameters: width:{}", width)<<endl;

    auto [xres, yres] = make_pair(72, 72);
    auto scale = m_item_sizescale;
    auto cursize = m_pages[d->page_cur]->pageSize() * scale;
    auto img = m_pages[d->page_cur]->renderToImage(scale * xres, scale * yres, 0, 0,
                                                   cursize.width(), cursize.height());
    qDebug() << QString("render params: %1, %2, %3, %4, sacle %5")
                    .arg(scale * xres)
                    .arg(scale * yres)
                    .arg(cursize.width())
                    .arg(cursize.height())
                    .arg(scale);

    m_pixmap = QPixmap::fromImage(img);
    m_photoItem->setImage(m_pixmap);
    m_photoItem->update();
    // d->label->setPixmap(QPixmap::fromImage(image));
}

void PageView::scale_bigger() {}

void PageView::scale_smaller() {}
void PageView::load(Poppler::Document *docu) {
    for (int i = 0; i < docu->numPages(); i++) {
        m_pages.push_back(docu->page(i));
    }
    d->document = docu;
    d->page_cur = -1;
    d->page_max = m_pages.size();
    this->load_page(0);
}
WordItemMap *PageView::getWordItems() { return &(d->wordItems_in_page); }
