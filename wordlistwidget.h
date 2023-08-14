#include <QWidget>
#include "worditem.h"
#include "words.h"

class WordlistWidget : public QWidget {
    Q_OBJECT
  public:
    WordlistWidget(QWidget *parent = nullptr);
    void setWordStore(std::shared_ptr<SqlSave> wordstore);
    void setupModel(WordItemMap *data);
  signals:
    void updateFilter();
  public slots:
    void onPageLoadBefore();
    void onPageLoadAfter();
    void onListViewContextMenu(const QPoint &pos);

  private:
    void markSelectionWithLevel(wordlevel_t lv);

  private:
    struct PrivateData *d;
};
