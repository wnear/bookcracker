#include "pagecontainer.h"

#include "pageview.h"
#include <QHBoxLayout>

class PageContainer::Private {
  public:
    Poppler::Document *document{nullptr};
    PageView *view{nullptr};
};

PageContainer::PageContainer(QWidget *parent) : QWidget(parent) {
    d = new Private;
    d->view = new PageView(this);

    auto lay = new QHBoxLayout(this);
    lay->setContentsMargins({0, 0, 0, 0});
    lay->addWidget(d->view);

}

void PageContainer::setDocument(Poppler::Document *doc) {
    d->document = doc;
    //file -> data.
    d->view->load(doc);
}
