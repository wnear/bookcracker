#include "outline.h"
#include <iostream>
#include <QDebug>

using namespace std;
class Outline::Private {
  public:
    Outline_t outline;
    document_t* document{nullptr};
    ~Private(){
        document = nullptr;
    }
};

Outline::Outline() {
    d = new Private;
}

void load_section_cur(Section &section, Poppler::OutlineItem &item) {
    section.title = item.name();
    auto dest = item.destination();
    if (dest) section.link.page = dest->pageNumber();
    for (auto i : item.children()) {
        Section sub;
        load_section_cur(sub, i);
        section.children.push_back(sub);
    }
}

void Outline::load_outlie() {
    d->outline.clear();

    auto items = d->document->outline();
    if (items.isEmpty()) {
        cout << "no outine available.";
    }
    for (auto i : items) {
        Section cur_section;
        load_section_cur(cur_section, i);
        if (cur_section.link.page == -1) {
            // TODO:
            // maybe exit from here.
        }
        d->outline.push_back(cur_section);
    }
}

void display_section_cur(const Section &sect, int depth = 0) {
    for (int i = 0; i < depth; i++) cout << "    ";  // use indent as layer indicator.
    qDebug() << QString("level %1 %2 at page %3")
                    .arg(depth)
                    .arg(sect.title)
                    .arg(sect.link.page);
    for (auto sub : sect.children) {
        display_section_cur(sub, depth + 1);
    }
}

void Outline::display_outline() {
    for (auto sec : d->outline) {
        display_section_cur(sec);
    }
}
void Outline::setDocument(document_t * doc) {
    d->document = doc;
}

