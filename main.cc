#include "mainwindow.h"
#include "words.h"

#include <QApplication>
#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("bookcracker");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    Mainwindow w;
    w.show();


    if (argc == 2) {
        QString file = argv[1];
        if (file.endsWith(".pdf")) w.openFile(file);
    }
    return app.exec();
}
