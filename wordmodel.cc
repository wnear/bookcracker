#include "wordmodel.h"


QModelIndex WordModel::index(int row, int column, const QModelIndex &parent) const {
    return (row >= 0 && row < this->rowCount()) ? createIndex(row, column, nullptr)
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
    return 1;
}

QVariant WordModel::data(const QModelIndex &index, int role) const {
    if(index.isValid()){
        if(role == Qt::DisplayRole ){
            auto data= m_data.value(m_data.keys()[index.row()]);
            return data.original;
        }
    }
    return QVariant();
}


// Qt::ItemFlags WordModel::flags(const QModelIndex &index) const {}
WordModel::WordModel(modeldata_t &document, QObject *parent)
    : QAbstractItemModel(parent), m_data(document) {}

void WordSortFilterProxyModel::updateFilter() {
    invalidateFilter();
}

// bool WordSortFilterProxyModel::lessThan(const QModelIndex &source_left,
//                                         const QModelIndex &source_right) const {
//     return true;
// }

bool WordSortFilterProxyModel::filterAcceptsRow(int sourceRow,
                                                const QModelIndex &sourceParent) const {
    return true;
}

