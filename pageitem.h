#pragma once

#include <QGraphicsObject>

class PageItem : public QGraphicsObject {
    Q_OBJECT
  public:
    PageItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void setImage(const QPixmap &pix);
    // void next();
    int shadowPadding()const {
        return m_padding_shadow;
    }
    // void prev();
    double getScale();
    void recalScale();
    void setBoardSize(QSize val);
    void setBoardBackground(const QColor &color){
        m_board_bgcolor = color;
    }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

  signals:
    void photoChanged();

  private:
    double m_scale{1.0};
    double m_scaleMax{2.0};
    QSize m_board_size;
    QColor m_board_bgcolor;

    int m_padding_shadow = 6;
    QPixmap m_pixmap;
};
