
#include <QMainWindow>

class Mainwindow : public QMainWindow {
  public:
    enum WordType { KNEW, DICT, IGNORED, NEW };
    Mainwindow();
    ~Mainwindow();

    void setupBtns();

    void openFile(const QString &filename);

  private:
    void load_settings();

    void load_page(int n);

    void go_next();
    void go_previous();
    void scale_bigger();
    void scale_smaller();
    void update_image();
    void test_scan_annotations();

    bool shouldShowWordType(WordType wt) const;

    QStringList do_filter(const QStringList &cur);
    QStringList words_forCurPage();
    QStringList words_forDocument();
    // QStringList scavenge_impl(){}
    void update_filter();

    void go_to(int n);

  private:
    class Private;
    Private *d;
};
