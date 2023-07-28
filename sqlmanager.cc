#include "sqlmanager.h"
#include "utils.h"
#include <QDebug>
#include <QRegularExpression>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>

SQLManager::SQLManager() {
    // QString m_location = util::datadir().absoluteFilePath("localpod.sqlite3");
    auto m_location = "a.sqlite3";

    // Open the database connection
    m_sqldb = QSqlDatabase::addDatabase("QSQLITE");
    m_sqldb.setDatabaseName(m_location);
    bool ok = m_sqldb.open();
    if (!ok)
        qFatal(
            "Fatal error establishing a connection with Vibrato's sqlite3 database. :(");
}

void SQLManager::init() {
    // Check if 'notes' table exists. If not, create it and import tutorial note.
    // Create any tables that are non-existent.
    runScript(":sql/create.sql");
    init_id();
}

void SQLManager::init_id() {
}

bool SQLManager::runScript(QString fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to load SQL file" << fileName;
        return false;
    }
    QSqlQuery query;
    return runScript(&file, &query);
}

bool SQLManager::runScript(QFile *file, QSqlQuery *query) {
    QString contents = file->readAll();

    // Remove sql comments and
    contents = contents.replace(QRegularExpression("--[^\\n]+\\n"), "\n");
    // Remove unnecesary blank lines
    contents = contents.replace(QRegularExpression("\\n\\n+"), "\n");

    QStringList queryList = contents.split(";");

    for (QString line : queryList) {
        line = line.trimmed();
        if (line.isEmpty()) continue;

        bool success = query->exec(line);
        if (!success) {
            qDebug() << "current line is:" << line;
            logSqlError(query->lastError());
        }
    }

    return query->isActive();
}

bool SQLManager::logSqlError(QSqlError error, bool fatal) {
    if (!error.isValid()) return true;
    QString status = "SQL Error";
    if (fatal) status = "FATAL SQL Error";

    QString msg =
        QString("[%1] %2 %3").arg(status, error.driverText(), error.databaseText());

    // Print the message. Must convert it from QString to char*.
    if (fatal)
        qFatal("%s", msg.toLatin1().constData());
    else
        qWarning("%s", msg.toLatin1().constData());

    return false;
}
