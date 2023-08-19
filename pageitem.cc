#include "pageitem.h"

#include "pageview.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

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
    painter->setPen(QPen(Qt::black, 1 ));
    painter->drawRect(m_pixmap.rect());
}


void PageItem::setImage(const QPixmap &pix) {
    m_pixmap = pix;
    emit photoChanged();
}


QRectF PageItem::boundingRect() const {
    auto rect = m_pixmap.rect();
    rect.setWidth(rect.width() +12);
    rect.setHeight(rect.height() +12);
    return rect;
}
