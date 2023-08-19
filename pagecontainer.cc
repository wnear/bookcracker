#include "pagecontainer.h"

#include "pageview.h"
#include <QHBoxLayout>

class PageContainer::Private {
  public:
    Poppler::Document *document{nullptr};
    PageView *focus{nullptr};
    PageView *view{nullptr};
};

PageContainer::PageContainer(QWidget *parent) : QWidget(parent) {
    d = new Private;
    d->view = new PageView(this);
    d->focus = d->view;

    auto lay = new QHBoxLayout(this);
    lay->setContentsMargins({0, 0, 0, 0});
    lay->addWidget(d->view);

}

void PageContainer::setDocument(Poppler::Document *doc) {
    d->document = doc;
    //file -> data.
    d->view->load(doc);
}
void PageContainer::go_to(int n) {
    d->focus->go_to(n);
}

void PageContainer::go_next() {
    d->focus->go_next();
}

void PageContainer::go_prev() {
    d->focus->go_prev();
}

void PageContainer::scale_bigger() {}

void PageContainer::scale_smaller() {}

