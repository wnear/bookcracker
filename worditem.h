#pragma once
#include <string>
#include <QRectF>
#include <QString>
#include <QVector>
#include <poppler-annotation.h>

using wordlevel_t = enum {
    LEVEL_UNKOWN,
    WORD_IS_KNOW,
    WORD_IS_LERNING,
    WORD_TOO_HARD,
    WORD_IS_IGNORED,
};

struct WordItem {
    QString content;   // after cleanup.
    QString original;  // directed frm the text
    //
    QString context_sentence;

    QVector<std::pair<QRectF, Poppler::HighlightAnnotation*>> highlight;
    std::pair<int, int> id;  // page-number.
    int id_page;
    int id_idx;
    bool hasNo;
    bool isHighlighted{false};
    wordlevel_t wordlevel{LEVEL_UNKOWN};
    bool isKnown{false};
    bool isIgnored{false};
    bool isIndict{false};
    QString meaning;  // get from dictionary
    bool isVisible(){ // visible in wordlist and highlight.
        // return isIndict == false &&
        return wordlevel == LEVEL_UNKOWN;
        return false;
    }
};

using WordItemList = QVector<WordItem>;
using WordItemMap = QMap<QString, WordItem>;
