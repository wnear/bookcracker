#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFile>

class SQLManager {
  public:
    SQLManager();
    ~SQLManager() {}
    static SQLManager *instance() {
        static SQLManager *instance = new SQLManager;
        return instance;
    }

    void init();
    void init_id();


    void close() { m_sqldb.close(); }
    bool runScript(QString fileName);
    bool runScript(QFile *file, QSqlQuery *query);

    bool logSqlError(QSqlError error, bool fatal = false);
    void checkReturn(bool ok, QSqlQuery &q, const QString &msg = "", int line = -1);

  private:
    QSqlDatabase m_sqldb;
};

