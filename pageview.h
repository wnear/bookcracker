#pragma once

#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QFileInfo>

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


class PageView : public QWidget {
    Q_OBJECT
  public:
    PageView(QWidget *parent = nullptr);
    void autoscale();
    void setScale(float incr);
    QSize boardSize() const;
    void displayInfo() const;
    void next();
    void prev();

  private:
    QGraphicsScene *m_scene{nullptr};
    QGraphicsView *m_view{nullptr};
    PageItem *m_photoItem{nullptr};

    QFileInfoList m_filelist{};
    int m_index{0};
    QPixmap m_pixmap;

    int m_padding_topbottom = 8;
    int m_padding_leftright = 8;
    float m_scale = 1.0;
    const int m_padding = 0;
};
