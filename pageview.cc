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

PageView::PageView(QWidget *parent) : QWidget(parent) {
    this->setLayout(new QVBoxLayout);

    m_photoItem = new PageItem;
    m_photoItem->setPos(0, 0);
    m_filelist = QDir("/home/bill/Pictures").entryInfoList({"*.png", "*.jpg"});
    go_next();

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
    m_photoItem->setBoardSize(boardSize());
    m_photoItem->setBoardBackground(m_view->backgroundBrush().color());
    m_photoItem->recalScale();
    qDebug() << __LINE__;
    displayInfo();

    autoscale();

    {
        auto widget = new QWidget;
        auto lay = new QHBoxLayout(widget);
        this->layout()->addWidget(widget);
        auto btn_next = new QPushButton("next");
        auto btn_previous = new QPushButton("prev");
        auto btn_zoom_smaller = new QPushButton("zoom-in");
        auto btn_zoom_bigger = new QPushButton("zoom-out");

        lay->addWidget(btn_next);
        lay->addWidget(btn_previous);
        lay->addWidget(btn_zoom_smaller);
        lay->addWidget(btn_zoom_bigger);
        connect(btn_next, &QPushButton::clicked, this, &PageView::go_next);
        connect(btn_previous, &QPushButton::clicked, this, &PageView::go_prev);
        connect(m_photoItem, &PageItem::photoChanged, this, &PageView::autoscale);

        connect(btn_zoom_smaller, &QAbstractButton::clicked, this,
                [this]() { m_view->scale(1.1, 1.1); });
        connect(btn_zoom_bigger, &QAbstractButton::clicked, this,
                [this]() { m_view->scale(0.9, 0.9); });
    }

    this->layout()->addWidget(m_view);
}

void PageView::setScale(float incr) {
    m_scale += incr;
    if (m_scale < 0.1) m_scale = 0.1;
}

void PageView::autoscale() {
    m_photoItem->setPos(0, 0);
    auto itemsize = m_photoItem->boundingRect();
    auto s = m_photoItem->getScale();
    m_scene->setSceneRect(itemsize);
    m_view->resetTransform();
    m_view->scale(s, s);
    m_scene->update();
    m_photoItem->setBoardSize(boardSize());
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
            m_view->contentsRect().height() - 2 * m_padding_topbottom
    };
}
void PageView::go_next() {
    m_index++;
    if (m_index == m_filelist.size()) m_index = 0;
    m_pixmap = QPixmap::fromImage(QImage(m_filelist[m_index].absoluteFilePath()));
    m_photoItem->setImage(m_pixmap);
    assert(!m_pixmap.isNull());
}

void PageView::go_prev() {
    m_index--;
    if (m_index == -1) m_index = m_filelist.size() - 1;
    m_pixmap = QPixmap::fromImage(QImage(m_filelist[m_index].absoluteFilePath()));
    m_photoItem->setImage(m_pixmap);
    assert(!m_pixmap.isNull());
}
class PageView::Private {
public:
    int page_cur;
    double scale;
    int page_max; //[0, pagemax] 0-based.
    QStringList words_page_all;  // all words in current page.
    WordItemMap wordItems_in_page;
    std::shared_ptr<SqlSave> wordstore{nullptr};

    QTransform normalizedTransform;

    std::shared_ptr<Poppler::Page> pdfpage{nullptr};
    QSizeF pageSize;
};

void PageView::go_to(int n) {

    int page_max = d->page_max;
    int pageno = n;
    if (pageno < 0) pageno = 0;
    if (pageno >= page_max) {
        pageno = page_max - 1;
    }
    // wont turn page.
    if (pageno == d->page_cur) return;
    this->load_page(pageno);
}
QStringList PageView::words_forCurPage() {
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

void PageView::load_page(int n) {

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
    // d->edit_setPage->setText(QString("%1").arg(d->page_cur + 1));
    // d->pdfpage = d->document->page(d->page_cur);
    d->pdfpage = std::unique_ptr<Poppler::Page>(m_pages[d->page_cur]);
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

Poppler::HighlightAnnotation *PageView::make_highlight(QRectF region) {
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
QStringList PageView::check_wordlevel(const QStringList &wordlist) {
    QStringList res;
    for (auto word : d->wordItems_in_page.keys()) {
        auto lv = d->wordstore->getWordLevel(word);
        d->wordItems_in_page[word]->wordlevel = lv;
        isValidLevel(lv);
    }

    return {};
}

void PageView::update_filter() { check_wordlevel(d->words_page_all); }
void PageView::update_image() {

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
    //d->label->setPixmap(QPixmap::fromImage(image));
}

