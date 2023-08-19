#pragma once

#include <QGraphicsObject>
#include <QGraphicsDropShadowEffect>

class PageItem : public QGraphicsObject {
    Q_OBJECT
  public:
    PageItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void setImage(const QPixmap &pix);
    // void next();
    int shadowPadding()const { return m_padding_shadow; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

  signals:
    void photoChanged();

  private:
    QGraphicsDropShadowEffect *m_effect{nullptr};

    int m_padding_shadow = 6;
    QPixmap m_pixmap;
};
