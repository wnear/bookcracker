#include "worditem.h"

bool isValidLevel(wordlevel_t lv) {
    int x = static_cast<int>(lv);
    return x>=0 and x<=4;
}
