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

    //init
    void load(Poppler::Document *docu) {
        for (int i = 0; i < docu->numPages(); i++) {
            m_pages.push_back(docu->page(i));
        }
    }
    void setViewSize(){}
    QSize boardSize() const;


    //scale
    void autoscale();
    void setScale(float incr);
    void scaleToPageWidth(){}
    void scaleToPageHeight(){}
    void scaleToPageFit(){}

    void displayInfo() const;

    void load_page(int n);
    void update_image();

    //word.
    QStringList words_forCurPage();
    void update_filter();
    QStringList check_wordlevel(const QStringList &wordlist);

    //highlight
    Poppler::HighlightAnnotation *make_highlight(QRectF region);

    //jump
    void go_next();
    void go_prev();
    void go_to(int n);
  signals:
    // NOTE: directConnection signal-slot <=> cb on event.
    void pageLoadBefore();
    void PageLoadDone();

  private:
    class Private;
    Private *d;
    QGraphicsScene *m_scene{nullptr};
    QGraphicsView *m_view{nullptr};
    PageItem *m_photoItem{nullptr};

    QList<Poppler::Page *> m_pages;  // TODO: optimi
    QFileInfoList m_filelist{};
    int m_index{0};
    QPixmap m_pixmap;

    int m_padding_topbottom = 8;
    int m_padding_leftright = 8;
    float m_scale = 1.0;
    const int m_padding = 0;
};
