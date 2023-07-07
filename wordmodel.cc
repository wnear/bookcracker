#include "wordmodel.h"
#include <iostream>
using namespace std;

QModelIndex WordModel::index(int row, int column, const QModelIndex &parent) const {
    auto *item = &(m_data[m_data.keys()[row]]);
    return (row >= 0 && row < this->rowCount()) ? createIndex(row, column, item)
                                                : QModelIndex();
}

QModelIndex WordModel::parent(const QModelIndex &child) const {
    Q_UNUSED(child);
    return QModelIndex();
}

int WordModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_data.size();
}

int WordModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return COLUMN_END;
}

QVariant WordModel::data(const QModelIndex &index, int role) const {
    if (index.isValid()) {
        if (role == Qt::DisplayRole) {
            auto data = m_data.value(m_data.keys()[index.row()]);
            switch (index.column()) {
                case COLUMN_WORD: {
                    return data.original;
                }
                case COLUMN_VISIBLE: {
                    return data.isVisible();
                }
                case COLUMN_MEANING: {
                    return data.meaning;
                }
                case COLUMN_PAGE: {
                    return data.id_page;
                }
                case COLUMN_POS_IN_PAGE: {
                    return data.id_idx;
                }
                default: {
                    return "";
                }
            }
        }
    }
    return QVariant();
}

WordModel::WordModel(modeldata_t &document, QObject *parent)
    : QAbstractItemModel(parent), m_data(document) {}

void WordSortFilterProxyModel::updateFilter() { invalidateFilter(); }

bool WordSortFilterProxyModel::filterAcceptsRow(int sourceRow,
                                                const QModelIndex &sourceParent) const {
    auto isVisible = sourceModel()
                         ->index(sourceRow, WordModel::COLUMN_VISIBLE, sourceParent)
                         .data()
                         .toBool();

    bool res = isVisible;
    if (!m_match.isEmpty()) {
        auto word = sourceModel()
                        ->index(sourceRow, WordModel::COLUMN_WORD, sourceParent)
                        .data()
                        .toString();
        auto res2 = word.toLower().contains(m_match);
        res = res and res2;
    }
    return res;
}
void WordSortFilterProxyModel::setSearchPattern(const QString &ptn) {
    m_match = ptn.toLower();
    invalidateFilter();
}

bool WordSortFilterProxyModel::lessThan(const QModelIndex &l,
                                        const QModelIndex &r) const {
    assert(l.column() == r.column());
    switch (l.column()) {
        case WordModel::COLUMN_WORD: {
            auto cmp =
                QString::localeAwareCompare(l.data().toString(), r.data().toString());
            return (cmp < 0);
        }
        case WordModel::COLUMN_POS_IN_PAGE:
            return l.data().toInt() > r.data().toInt();
    }
    return true;
}
