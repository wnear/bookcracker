#pragma once
#include <QDir>
#include <set>
#include "worditem.h"

class Words {
  protected:
    QDir rootdir;
    std::set<QString> m_known_list;
    std::set<QString> m_ignore_list;
    std::set<QString> m_dict_list;

    QString m_fileOfKnown;
    QString m_fileOfDict;
    QString m_fileOfIgnore;

    virtual void save() = 0;
    virtual void load() = 0;

  public:
    Words();
    virtual ~Words() = default;

    bool isKnown(const QString &word);

    bool isIgnored(const QString &word);

    bool isInDict(const QString &word);

    void resetdict(const QString &word) {
        m_ignore_list.erase(word);
        m_dict_list.erase(word);
        m_ignore_list.erase(word);
    }
    //TODO:
    void setWordWithLevel(const QString &word, wordlevel_t level) {
        resetdict(word);
        switch (level) {
            case WORD_IS_KNOWN:
                break;
            case WORD_IS_LEARNING:
                break;
            case WORD_IS_IGNORED:
                break;
            default:
                break;
        }
    }

    void addWordToKnown(const QString &word) {
        resetdict(word);
        m_known_list.insert(word);
    }
    void addWordToIgnore(const QString &word) {
        resetdict(word);
        m_ignore_list.insert(word);
    }
    void addWordToDict(const QString &word) {
        resetdict(word);
        m_dict_list.insert(word);
    }

    void addWordToKnown(const QStringList &words) {
        for (auto word : words) m_known_list.insert(word);
    }
    void addWordToIgnore(const QStringList &words) {
        for (auto word : words) m_ignore_list.insert(word);
    }
};

class TextSave : public Words {
    const std::string c_known = "plain_known.txt";
    const std::string c_ignore = "plain_ignore.txt";
    const std::string c_dict = "plain_dict.txt";

  public:
    TextSave();
    virtual ~TextSave();
    virtual void save() override;
    virtual void load() override;
};

class Leveldb : public Words {};
