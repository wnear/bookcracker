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
    void export_level_to_text(wordlevel_t lv, const QString &filename);

    void close() { m_sqldb.close(); }
    bool runScript(QString fileName);
    bool runScript(QFile *file, QSqlQuery *query);



    // word and wordlevel_t
    wordlevel_t findword(const QString &word);
    void getwords(QMap<QString, wordlevel_t> &res);
    bool addword(const QString &word, wordlevel_t lv);
    bool updateword(const QString &word, wordlevel_t new_level);

    bool addword(const QString &word , WordItem *lv){
        return true;
    }
    bool updateword(const QString &word, WordItem *lv){
        return true;
    }

    bool logSqlError(QSqlError error, bool fatal = false);
    void checkReturn(bool ok, QSqlQuery &q, const QString &msg = "", int line = -1);

  private:
    QSqlDatabase m_sqldb;
};

