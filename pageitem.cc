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

    m_effect = new QGraphicsDropShadowEffect;
    m_effect->setOffset(3);
    // qDebug()<<m_effect->color();
    // m_effect->setColor(Qt::black);
    m_effect->setBlurRadius(20);
    this->setGraphicsEffect(m_effect);
}

void PageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                      QWidget *widget) {
    painter->drawPixmap(m_pixmap.rect(), m_pixmap);

    //NOTE: how to get itm's current scale, api exist?
    // line thickness should depend on the scale.
    painter->setPen(QPen(Qt::black, 1 / m_scale));
    painter->drawRect(m_pixmap.rect());
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
    m_effect->setOffset(3);
    m_effect->setColor(Qt::black);
    m_effect->setBlurRadius(20);
    assert(!m_pixmap.isNull());
    emit photoChanged();
}


QRectF PageItem::boundingRect() const { return m_pixmap.rect(); }
