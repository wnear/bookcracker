
#include <QMainWindow>
#include <poppler-annotation.h>

#include "outline.h"


class Mainwindow : public QMainWindow {
    Q_OBJECT
  public:
    enum WordType { KNEW, DICT, IGNORED, NEW };
    Mainwindow();
    ~Mainwindow();

    void setupBtns();

    void openFile(const QString &filename);

  signals:
    //NOTE: directConnection signal-slot <=> cb on event.
    void pageLoadBefore();
    void PageLoadDone();

  private:
    void load_settings();

    void load_page(int n);
    // NOTE: following two method is called by load_page(),
    // do the job before and after the page-no is changed,
    // now maily related to annotation management.

    void test_load_outline();
    // NOTE: debug

    void go_next();
    void go_previous();
    void scale_bigger();
    void scale_smaller();
    void update_image();
    void test_scan_annotations();

    Poppler::HighlightAnnotation *make_highlight(QRectF region);


    QStringList check_wordlevel(const QStringList &cur);
    QStringList words_forCurPage();
    QStringList words_forDocument();
    // QStringList scavenge_impl(){}
    void update_filter();

    void go_to(int n);

  private:
    class Private;
    Private *d;
};
