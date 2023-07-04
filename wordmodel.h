#pragma once

#include "worditem.h"
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <memory>

class WordModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    using modeldata_t = QMap<QString, WordItem>;
    WordModel(modeldata_t &document, QObject *parent = nullptr);
    ~WordModel() = default;

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void reset_data_before(){
        beginResetModel();
    }
    void reset_data_after(){
        endResetModel();
    }

    // Qt::ItemFlags flags(const QModelIndex &index) const override;

  private:
    void setupModelData();
    modeldata_t &m_data;
};

// class WordModel
//
class WordSortFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    WordSortFilterProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}

  protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    // bool lessThan(const QModelIndex &source_left,
    //               const QModelIndex &source_right) const override;
    void updateFilter();

  private:
};

class WordListModel : public QAbstractListModel {
  public:
    using modeldata_t = QMap<QString, WordItem>;
    WordListModel(modeldata_t &document, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_data(document){};

    QVariant data(const QModelIndex &index, int role) const override {
        if (role != Qt::DisplayRole) return {};
        auto x = index.row();
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
        return parent.isValid()? 0: m_data.size();
    }

  private:
    const modeldata_t &m_data;
};
