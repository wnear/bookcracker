#include "sqlmanager.h"
#include "utils.h"
#include <QDebug>
#include <QRegularExpression>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>

SQLManager::SQLManager() {
    QString m_location = datadir().absoluteFilePath("words.sqlite3");

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
    // import_txt_with_level(datadir().absoluteFilePath("plain_known.txt"), WORD_IS_KNOWN);
    // import_txt_with_level(datadir().absoluteFilePath("plain_ignore.txt"), WORD_IS_IGNORED);
}

void SQLManager::init_id() {}

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

void SQLManager::import_txt_with_level(const QString &filename, wordlevel_t lv) {
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("file read error.");
        return;
    }
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine();
        this->addword(line, lv);
    }
}
void SQLManager::export_level_to_text(wordlevel_t lv, const QString &filename) {

}

bool SQLManager::addword(const QString &word, wordlevel_t lv) {
    // qDebug() <<__PRETTY_FUNCTION__<<":"<<__LINE__;
    QSqlQuery q;
    QString cmdstr =
        QString("INSERT INTO word_info (word, proficiency) VALUES (%1, %2)")
            .arg(":word")
            .arg(":proficiency");
    q.prepare(cmdstr);

    q.bindValue(":word", word);
    q.bindValue(":proficiency", static_cast<int>(lv));

    auto ok = q.exec();
    checkReturn(ok, q, "add word");
    if(not ok)
        assert(0);
    return ok;
}

// UPDATE COMPANY SET ADDRESS = 'Texas' WHERE ID = 6
bool SQLManager::updateword(const QString &word, wordlevel_t lv) {
    // qDebug() <<__PRETTY_FUNCTION__<<":"<<__LINE__;
    QSqlQuery q;
    QString cmdstr =
        QString("update word_info set proficiency = %1 where word=\"%2\"").arg(static_cast<int>(lv)).arg(word);
    q.prepare(cmdstr);

    auto ok = q.exec();
    checkReturn(ok, q, "add word");
    if(not ok)
        assert(0);
    return ok;

}
wordlevel_t SQLManager::findword(const QString &word) {
    QSqlQuery q;
    QString cmdstr =
        QString("select proficiency from word_info where word = \"%1\"").arg(1);
    q.prepare(cmdstr);

    auto ok = q.exec();
    checkReturn(ok, q, "find word");
    if(!ok){
        assert(0);
    }
    if(q.next()){
        auto val = q.value("proficiency").toInt();
        return static_cast<wordlevel_t>(val);
    }
    return LEVEL_UNKOWN;
}

void SQLManager::checkReturn(bool ok, QSqlQuery &q, const QString &msg, int line) {
    if (not ok) {
        if (line == -1)
            qDebug() << QString("[sql error]: %1").arg(msg);
        else
            qDebug() << QString("[sql error at L%1]: %2").arg(line).arg(msg);
        logSqlError(q.lastError());
    }
}

void SQLManager::getwords(QMap<QString, wordlevel_t> &res) {
    QSqlQuery q("SELECT word, proficiency from word_info");
    while(q.next()){
        auto word = q.value(0).toString();
        auto lv = q.value(1).toInt();
        res.insert(word, static_cast<wordlevel_t>(lv));
    }
}

