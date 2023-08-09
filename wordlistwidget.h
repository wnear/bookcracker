#include <QWidget>

class WordlistWidget : public QWidget {
    Q_OBJECT
  public:
    WordlistWidget(QWidget *parent = nullptr);
    void setWords(const QStringList &words);
    bool showx();
  signals:
    void updateFilter();

  protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

  private:
    struct PrivateData *d;
};
