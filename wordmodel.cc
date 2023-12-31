#include "wordmodel.h"
#include <iostream>
#include <QDebug>
using namespace std;

QModelIndex WordModel::index(int row, int column, const QModelIndex &parent) const {
    if (m_data == nullptr)
        return QModelIndex();
    auto x = m_data->value(m_data->keys()[row]);
    // auto *item = &(x);
    return (row >= 0 && row < this->rowCount()) ? createIndex(row, column, x)
                                                : QModelIndex();
}

QModelIndex WordModel::parent(const QModelIndex &child) const {
    Q_UNUSED(child);
    return QModelIndex();
}

int WordModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    if (m_data == nullptr)
        return 0;
    else
        return m_data->size();
}

int WordModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return COLUMN_END;
}

QVariant WordModel::data(const QModelIndex &index, int role) const {
    if (index.isValid()) {
        if (role == Qt::DisplayRole) {
            auto data = m_data->value(m_data->keys()[index.row()]);
            switch (index.column()) {
                case COLUMN_WORD: {
                    return data->word;
                }
                case COLUMN_VISIBLE: {
                    return data->isVisible();
                }
                case COLUMN_MEANING: {
                    return data->meaning;
                }
                case COLUMN_PAGE: {
                    return data->sentenceContext[0].page;
                }
                case COLUMN_POS_IN_PAGE: {
                    return data->sentenceContext[0].location;
                }
                default: {
                    return "";
                }
            }
        }
    }
    return QVariant();
}

WordModel::WordModel(QObject *parent) : QAbstractItemModel(parent) {}

void WordModel::setupModelData(modeldata_t *document) { m_data = document; }

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
