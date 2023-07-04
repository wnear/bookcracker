#pragma once
#include <string>
#include <QRectF>
#include <QString>
#include <QVector>


struct WordItem{
    QString content; // after cleanup.
    QString original; // directed frm the text
    //
    QString context_sentence;

    QVector<std::pair<QRectF, void*>> highlight;
    std::pair<int, int> id; //page-number.
    bool hasNo;
    bool isHighlighted{false};
    bool isKnown{false};
    bool isIgnored{false};
    bool isIndict{false};
    std::string meaning; //get from dictionary
};


