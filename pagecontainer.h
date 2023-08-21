#pragma once

#include <QWidget>
#include <poppler-qt5.h>
#include "pageview.h"

class PageContainer : public QWidget {
  public:
    PageContainer(QWidget *parent = nullptr);

    void setDocument(Poppler::Document *doc);

    PageView *focus();

    void go_to(int n);
    void go_next();
    void go_prev();
    void scale_bigger();
    void scale_smaller();

  private:
    class Private;
    Private *d{nullptr};
};
