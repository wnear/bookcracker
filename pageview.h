#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QFileInfo>

#include "pageitem.h"
#include <poppler-qt5.h>


class PageView : public QWidget {
    Q_OBJECT
  public:
    PageView(QWidget *parent = nullptr);
    void load(Poppler::Document *docu){
        for(int i = 0; i< docu->numPages(); i++){
            m_pages.push_back(docu->page(i));
        }
    }
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

    QList<Poppler::Page*> m_pages; //TODO: optimi
    QFileInfoList m_filelist{};
    int m_index{0};
    QPixmap m_pixmap;

    int m_padding_topbottom = 8;
    int m_padding_leftright = 8;
    float m_scale = 1.0;
    const int m_padding = 0;
};
