
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

    void test_load_outline();
    void test_scan_annotations();

    Poppler::HighlightAnnotation *make_highlight(QRectF region);


    QStringList check_wordlevel(const QStringList &cur);
    QStringList words_forCurPage();
    QStringList words_forDocument();
    // QStringList scavenge_impl(){}
    void update_filter();

  private:
    class Private;
    Private *d;
};
