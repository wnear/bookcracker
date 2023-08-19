#pragma once

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <poppler-qt5.h>

struct Position;
struct Section;
using Outline_t = std::vector<Section>;
using document_t = Poppler::Document;

struct Position {
    QString urlOrFilename;
    int page{-1};
    QPointF left_top;
    QRectF boundry;
};

struct Section {
    QString title;
    Position link;
    Outline_t children;
};

class Outline {
  public:
    Outline();
    void setDocument(document_t* doc);

    void load_outlie();
    void display_outline();
    ~Outline(){

    }

  private:
    class Private;
    Private *d{nullptr};
};
