#pragma once
#include <QDir>
#include <set>
#include "worditem.h"
#include "sqlmanager.h"
#include <QMap>
#include <unordered_map>

// NOTE: words is designed as two layer-ed, in memory and in-disk persistance.
class Words {
  protected:
    QDir rootdir;

    std::set<QString> m_known_list;
    std::set<QString> m_dict_list;
    std::set<QString> m_ignore_list;

    QMap<QString, wordlevel_t> m_all_list;
    QMap<int, std::set<QString> *> m_dicts{
        {WORD_IS_KNOWN, &m_known_list},
        {WORD_IS_LEARNING, &m_dict_list},
        {WORD_IS_IGNORED, &m_ignore_list},
    };

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

    wordlevel_t getWordLevel(const QString &word) {
        return m_all_list.value(word, LEVEL_UNKOWN);
    }
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

    virtual void updateWord(const QString &word, wordlevel_t old_level,
                            wordlevel_t new_level) {
        assert(isValidLevel(old_level));
        assert(isValidLevel(new_level));
        m_dicts[new_level]->insert(word);
        if (old_level != LEVEL_UNKOWN) {
            m_dicts[old_level]->erase(word);
        }
    }
};

class TextSave : public Words {
    const std::string c_known = "plain_known.txt";
    const std::string c_ignore = "plain_ignore.txt";
    const std::string c_dict = "plain_dict.txt";

    QString m_fileOfKnown;
    QString m_fileOfDict;
    QString m_fileOfIgnore;

  public:
    TextSave();
    virtual ~TextSave();
    virtual void save() override;
    virtual void load() override;
};

class SqlSave : public Words {
  public:
    SqlSave();
    virtual ~SqlSave() {}
    virtual void updateWord(const QString &word, wordlevel_t old_level,
                            wordlevel_t new_level) override {
        Words::updateWord(word, old_level, new_level);
        if (old_level == LEVEL_UNKOWN) {
            // add
            SQLManager::instance()->addword(word, new_level);
        } else {
            // update.
            SQLManager::instance()->updateword(word, new_level);
        }
    }
    virtual void save() override {}
    virtual void load() override;
};

class Leveldb : public Words {};
