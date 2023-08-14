#pragma once
#include <string>
#include <QRectF>
#include <QString>
#include <QVector>
#include <poppler-annotation.h>

enum wordlevel_t{
    LEVEL_UNKOWN,
    WORD_IS_KNOWN,
    WORD_IS_LEARNING,
    WORD_TOO_HARD,
    WORD_IS_IGNORED,
};

bool isValidLevel(wordlevel_t lv);

struct WordItem {
    QString content;   // after cleanup.
    QString original;  // directed frm the text

    QString context_sentence;

    QVector<std::pair<QRectF, Poppler::HighlightAnnotation*>> highlight;
    std::pair<int, int> id;  // page-number.
    int id_page;
    int id_idx;
    bool hasNo;
    wordlevel_t wordlevel{LEVEL_UNKOWN};
    bool isKnown{false};
    bool isIgnored{false};
    bool isIndict{false};
    QString meaning;  // get from dictionary/disk, may need to cache to disk.
    bool isVisible(){ // visible in wordlist and highlight.
        return wordlevel == LEVEL_UNKOWN;
    }
};

using WordItemList = QList<WordItem>;
using WordItemMap = QMap<QString, WordItem>;

Q_DECLARE_METATYPE(WordItem)
