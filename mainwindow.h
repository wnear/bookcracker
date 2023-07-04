
#include <QMainWindow>


struct Position;
struct Section;
using Outline_t = std::vector<Section>;

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
    //NOTE: following two method is called by load_page(),
    //do the job before and after the page-no is changed,
    //now maily related to annotation management.
    void load_page_before();
    void load_page_after();

    void load_outline();
    //NOTE: debug
    void display_outline();

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
