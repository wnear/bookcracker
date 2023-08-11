#pragma once

#include "worditem.h"
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <memory>

// NOTE:  implementation notes:
// 1. column 1 is word. use for wordlist of QListView.
// 1. column 2,3,4, for different usage, may contains:
//    - word visibility => simple filter.
//    - word definition, show in widget.
//    - word rect, used for annotation.

class WordModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    using modeldata_t = WordItemMap;
    enum COLUMN_NO {
        COLUMN_WORD,
        COLUMN_VISIBLE,
        COLUMN_MEANING,
        COLUMN_PAGE,
        COLUMN_POS_IN_PAGE,
        COLUMN_END
    };
    WordModel( QObject *parent = nullptr);
    void setupModelData(modeldata_t *document);
    ~WordModel() = default;

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void reset_data_before() { beginResetModel(); }
    void reset_data_after() { endResetModel(); }

    // Qt::ItemFlags flags(const QModelIndex &index) const override;

  private:
    modeldata_t *m_data;
};

class WordSortFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

  public:
    WordSortFilterProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}

    void updateFilter();
    void setSearchPattern(const QString &ptn);

  protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &source_left,
                  const QModelIndex &source_right) const override;

  private:
    QString m_match{};
};

// TODO: may be this one is more suitable than WordModel.
class WordListModel : public QAbstractListModel {
  public:
    using modeldata_t = WordItemMap;
    WordListModel(modeldata_t *data, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_data(data){};

    QVariant data(const QModelIndex &index, int role) const override {
        if (role != Qt::DisplayRole) return {};
        // if (x < 0 || x >= m_data.length()) return {};
        // auto &&cur = m_data[x];
        switch (role) {
            case Qt::DisplayRole:
                return "hello";
            default:
                break;
        }
        return {};
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : m_data->size();
    }

  private:
    const modeldata_t *m_data;
};
