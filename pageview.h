#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QFileInfo>

#include "pageitem.h"


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
