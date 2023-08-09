#include "utils.h"

#include <QStandardPaths>

// used for,
QStringList extendword(const QString& word) {
    QStringList res;
    return {};
}

// used for,
// 1. create a menu to choose from and record.
// 2. shoren, then serch it in the datasets.
QStringList shortenword(const QString& word) {
    QStringList res{word};
    // n.
    // adj
    // v.
    return {};
}

QDir datadir() {
    QString app_datapath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    auto data_dir = QDir(app_datapath);
    if ( !data_dir.exists() )
      data_dir.mkpath(".");
    return data_dir;
}

