#include "pageitem.h"

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
