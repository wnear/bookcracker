
#include "words.h"
#include <QStandardPaths>
#include <QDebug>
#include <fstream>
#include <iostream>

using namespace std;

namespace {

bool findword(std::set<QString>& container, const QString& word) {
    return container.find(word) != container.end();
}

bool searchword(std::set<QString>& words_cache, const QString& cur) {
    QStringList suffixes{"s", "es", "ed", "ing"};

    bool contains = true;
    do {
        // if cur's transformation in container, record the after-transformed.
        // else, record the original.
        // For People/Place name.
        // auto tmp = cur;
        QString tmp = cur;
        if (findword(words_cache, cur)) {
            break;
        } else if (cur[0].isDigit()) {
            break;
        } else if (cur[0].isUpper()) {
            tmp.clear();
            std::transform(cur.begin(), cur.end(), back_inserter(tmp),
                           [](auto&& c) { return c.toLower(); });
            if (findword(words_cache, tmp)) {
                break;
            } else {
            }
        } else {
        }
        bool findRelated = std::any_of(suffixes.begin(), suffixes.end(), [&](auto&& sf) {
            if (tmp.endsWith(sf) && tmp.size() > 3 + sf.size()) {
                tmp = tmp.left(tmp.size() - sf.size());
                if (findword(words_cache, tmp)) {
                    return true;
                }
            }
            return false;
        });
        if (findRelated) break;
        contains = false;
    } while (0);
    return contains;
}
}  // namespace

Words::Words() {
    rootdir.setPath(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
}

TextSave::TextSave() : Words() {
    m_fileOfKnown = QDir(rootdir).absoluteFilePath(QString::fromStdString(c_known));
    m_fileOfDict = QDir(rootdir).absoluteFilePath(QString::fromStdString(c_dict));
    m_fileOfIgnore = QDir(rootdir).absoluteFilePath(QString::fromStdString(c_ignore));

    this->load();
}

//TODO: wip.
void TextSave::save() {
    std::map<std::string, std::set<QString>&> op = {
        {m_fileOfKnown.toStdString(), this->m_known_list},
        {m_fileOfDict.toStdString(), this->m_dict_list},
        {m_fileOfIgnore.toStdString(), this->m_ignore_list}};
    for (auto& [file, cache] : op) {
        std::ofstream output(file, std::ios::out | std::ios::trunc);
        if (!output.is_open()) {
            // continue;
            throw std::runtime_error("cannot open file for write");
        }
        cout << "save ok to :" << file << endl;
        for (auto w : cache) {
            output << w.toStdString() << '\n';
        }
        // std::string line;
        // while (std::getline(input, line)){
        //     cout << line ;
        // }
    }
    QMap<int, QString> levelToFile{
        {WORD_IS_KNOWN, m_fileOfKnown},
        {WORD_IS_LEARNING, m_fileOfDict},
        {WORD_IS_IGNORED, m_fileOfDict},
    };
    for(auto word: m_all_list.keys()){
        switch(m_all_list.value(word)){
            case WORD_IS_KNOWN:

            case WORD_IS_LEARNING:
            case WORD_IS_IGNORED:
            default:
            break;
        }
    }
}

void TextSave::load() {
    std::map<std::string, wordlevel_t> fileToLevel = {
        {m_fileOfKnown.toStdString(), WORD_IS_KNOWN},
        {m_fileOfDict.toStdString(), WORD_IS_LEARNING},
        {m_fileOfIgnore.toStdString(), WORD_IS_IGNORED}};
    for (auto& [file, level] : fileToLevel) {
        cout << file << endl;
        std::ifstream input(file, std::ios::in);
        if (!input.is_open()) {
            continue;
            // throw std::runtime_error("cannot open file for read");
        }
        std::string line;
        while (std::getline(input, line)) {
            if (line.length() > 0 && line.find_first_not_of(' ') != line.npos) {
                m_all_list.insert(QString::fromStdString(line), level);
            }
        }
    }
}

TextSave::~TextSave() {
    std::cout << "destructor." << std::endl;
    this->save();
}

bool Words::isKnown(const QString& word) {
    return searchword(m_known_list, word);
    // return m_known_list.find(word) != m_known_list.end();
}

bool Words::isIgnored(const QString& word) {
    return m_ignore_list.find(word) != m_ignore_list.end();
}

bool Words::isInDict(const QString& word) { return searchword(m_dict_list, word); }

SqlSave::SqlSave() :Words(){
    this->load();
}
void SqlSave::load() {
    qDebug()<<"begin loading";
    SQLManager::instance()->getwords(m_all_list);
    qDebug()<<"loading done: " << m_all_list.size();
}

