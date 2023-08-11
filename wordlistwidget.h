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

  protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

  private:
    struct PrivateData *d;
};
