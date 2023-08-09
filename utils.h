#pragma once

#include <QString>
#include <QStringList>
#include <QDir>

//spelling related.
extern QStringList extendword(const QString& word);
extern QStringList shortenword(const QString& word);
extern QDir datadir();
