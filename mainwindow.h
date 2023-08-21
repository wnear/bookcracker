
#include <QMainWindow>
#include <poppler-annotation.h>

#include "outline.h"


class Mainwindow : public QMainWindow {
    Q_OBJECT
  public:
    Mainwindow();
    ~Mainwindow();

    void setupToolbar();

    void openFile(const QString &filename);

  signals:
    //NOTE: directConnection signal-slot <=> cb on event.
    void pageLoadBefore();
    void PageLoadDone();

  private:
    void load_settings();

    void test_load_outline();
    void test_scan_annotations();

    QStringList words_forDocument();
    // QStringList scavenge_impl(){}

  private:
    class Private;
    Private *d;
};
