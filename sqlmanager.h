#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFile>
#include "worditem.h"

class SQLManager {
  public:
    SQLManager();
    ~SQLManager() {}
    static SQLManager *instance() {
        static SQLManager *instance{nullptr};
        if(instance == nullptr){
            instance = new SQLManager;
            instance->init();
        }
        return instance;
    }

    void init();
    void init_id();

    void import_txt_with_level(const QString &filename, wordlevel_t lv);

    void close() { m_sqldb.close(); }
    bool runScript(QString fileName);
    bool runScript(QFile *file, QSqlQuery *query);

    bool addword(const QString &word, wordlevel_t lv);
    wordlevel_t findword(const QString &word);
    void addword(const QString &word , WordItem *lv);
    void updateword(const QString &word, WordItem *lv);
    void updateword(const QString &word, wordlevel_t old_level, wordlevel_t new_level);



    bool logSqlError(QSqlError error, bool fatal = false);
    void checkReturn(bool ok, QSqlQuery &q, const QString &msg = "", int line = -1);

  private:
    QSqlDatabase m_sqldb;
};

