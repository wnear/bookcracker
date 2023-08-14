#include "pageview.h"
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

