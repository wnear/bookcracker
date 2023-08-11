#include <QWidget>
#include "worditem.h"

class WordlistWidget : public QWidget {
    Q_OBJECT
  public:
    WordlistWidget(QWidget *parent = nullptr);
    void setWords(const QStringList &words);
    void setupModel(WordItemMap *data);
    bool showx();
  signals:
    void updateFilter();
  public slots:
    void onPageLoadBefore();
    void onPageLoadAfter();
    void onListViewContextMenu(const QPoint &pos);

  protected:

  private:
    struct PrivateData *d;
};
