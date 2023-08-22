#pragma once
#include <string>
#include <QRectF>
#include <QString>
#include <QVector>
#include <poppler-annotation.h>

enum wordlevel_t {
    LEVEL_UNKOWN,
    WORD_IS_KNOWN,
    WORD_IS_LEARNING,
    WORD_TOO_HARD,
    WORD_IS_IGNORED,
};

inline QString levelString(wordlevel_t v) {
    QString res;
    switch (v) {
        case LEVEL_UNKOWN:
            res = "LEVEL_UNKOWN";
            break;
        case WORD_IS_KNOWN:
            res = "WORD_IS_KNOWN";
            break;
        case WORD_IS_LEARNING:
            res = "WORD_IS_LEARNING";
            break;
        case WORD_TOO_HARD:
            res = "WORD_TOO_HARD";
            break;
        case WORD_IS_IGNORED:
            res = "WORD_IS_IGNORED";
            break;
        default:
        break;
    }
    return res;
}

bool isValidLevel(wordlevel_t lv);

struct sentence_t {
    QString word;
    int page;
    int location;// index of page->textbox();
    int wordidx; // textbox may contain multi word, like this_is_good, return three.
    QString sentence;
    // sentence_t(){}
};

struct WordItem {
    QString word;

    QList<sentence_t> sentenceContext{};
    QVector<std::pair<QRectF, Poppler::HighlightAnnotation*>> highlight;


    bool hasNo;
    wordlevel_t wordlevel{LEVEL_UNKOWN};
    bool isKnown{false};
    bool isIgnored{false};
    bool isIndict{false};
    QString meaning;    // get from dictionary/disk, may need to cache to disk.
    bool isVisible() {  // visible in wordlist and highlight.
        return wordlevel == LEVEL_UNKOWN or wordlevel == WORD_IS_LEARNING;
    }
};

using WordItemList = QList<WordItem>;
using WordItemMap = QMap<QString, WordItem*>;

Q_DECLARE_METATYPE(WordItem)
