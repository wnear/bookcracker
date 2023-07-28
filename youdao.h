#pragma once

// code from  https://github.com/rekols/redict-dtk.git
#include <QObject>
#include "qnetworkaccessmanager.h"

class QNetworkAccessManager;
class YouDaoApi : public QObject {
    Q_OBJECT

  public:
    YouDaoApi(QObject *parent = nullptr);
    static YouDaoApi *instance() {
        static YouDaoApi *i = new YouDaoApi;
        return i;
    }

    static void test();

    void queryword(const QString &word);
  public slots:
    void handleQueryWordFinished();

  private:
    QNetworkAccessManager *m_http{nullptr};
};
