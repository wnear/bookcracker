#include "pageview.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>


//FIX: when move item, the frame and shadow is out of view, couldn't see.
//
PageItem::PageItem(QGraphicsItem *parent) : QGraphicsObject(parent) {
    // this->setFlags(QGraphicsItem::ItemIsMovable);
}

void PageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                      QWidget *widget) {
    painter->drawPixmap(m_pixmap.rect(), m_pixmap);

    // line thickness should depend on the scale.
    painter->setPen(QPen(Qt::black, 1 / m_scale));
    painter->drawRect(m_pixmap.rect());

    auto shadow_real = m_padding_shadow / m_scale;
    auto p1 = this->boundingRect().topRight();
    auto p2 = this->boundingRect().bottomRight();
    auto p3 = this->boundingRect().bottomLeft();
    QGradientStops stops;
    stops << QGradientStop(0.0, Qt::black);
    stops << QGradientStop(1.0, m_board_bgcolor);

    QLinearGradient gradient(this->boundingRect().topRight(),
                             this->boundingRect().topRight() + QPointF(shadow_real, 0));
    painter->setPen(Qt::NoPen);
    gradient.setStops(stops);
    painter->setBrush(QBrush(gradient));
    QPointF points[4] = {
        p1, p1 + QPointF(shadow_real, 1 *shadow_real),
        p2 + QPointF(shadow_real, 1* shadow_real), p2
    };
    painter->drawConvexPolygon(points, 4);

    QLinearGradient gradient2(this->boundingRect().bottomLeft(),
                             this->boundingRect().bottomLeft() + QPointF(0, shadow_real));
    gradient2.setStops(stops);
    painter->setBrush(QBrush(gradient2));
    QPointF points2[4] = {
        p2, p2 + QPointF(shadow_real, 1 *shadow_real),
        p3 + QPointF(shadow_real, 1* shadow_real), p3
    };
    painter->drawConvexPolygon(points2, 4);

    // painter->setBrush(Qt::darkGray);
    // painter->drawRect(m_pixmap.rect());
}

void PageItem::setBoardSize(QSize val) { m_board_size = val; }

void PageItem::recalScale() {
    auto itemsize = this->boundingRect();

    auto w = itemsize.width();
    auto h = itemsize.height();

    auto tw = m_board_size.width();
    auto th = m_board_size.height();

    auto s1 = (tw - shadowPadding()) / w;
    auto s2 = (th - shadowPadding()) / h;

    auto s = std::min(s1, s2);
    s = std::min(s, m_scaleMax);  // s will not excede 2.0 .

    qDebug() << QString("size:%1 x %2 inside %3 x %4").arg(w).arg(h).arg(tw).arg(th);
    qDebug() << QString("s1:%1, s2:%2, s:%3").arg(s1).arg(s2).arg(s);
    qDebug() << "";

    m_scale = s;
}

double PageItem::getScale() { return m_scale; }

void PageItem::setImage(const QPixmap &pix) {
    m_pixmap = pix;
    recalScale();
    assert(!m_pixmap.isNull());
    emit photoChanged();
}


QRectF PageItem::boundingRect() const { return m_pixmap.rect(); }

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
PageView::PageView(QWidget *parent) : QWidget(parent) {
    this->setLayout(new QVBoxLayout);

    m_photoItem = new PageItem;
    m_photoItem->setPos(0, 0);
    m_filelist = QDir("/home/bill/Pictures").entryInfoList({"*.png", "*.jpg"});
    next();

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
        connect(btn_next, &QPushButton::clicked, this, &PageView::next);
        connect(btn_previous, &QPushButton::clicked, this, &PageView::prev);
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
void PageView::next() {
    m_index++;
    if (m_index == m_filelist.size()) m_index = 0;
    m_pixmap = QPixmap::fromImage(QImage(m_filelist[m_index].absoluteFilePath()));
    m_photoItem->setImage(m_pixmap);
    assert(!m_pixmap.isNull());
}

void PageView::prev() {
    m_index--;
    if (m_index == -1) m_index = m_filelist.size() - 1;
    m_pixmap = QPixmap::fromImage(QImage(m_filelist[m_index].absoluteFilePath()));
    m_photoItem->setImage(m_pixmap);
    assert(!m_pixmap.isNull());
}

