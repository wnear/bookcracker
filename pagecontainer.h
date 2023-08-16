#pragma once

#include <QWidget>
#include <poppler-qt5.h>

class PageContainer : public QWidget {
  public:
    PageContainer(QWidget *parent = nullptr);

    void setDocument(Poppler::Document *doc);

    QWidget *focus();

  private:
    class Private;
    Private *d{nullptr};
};
