#include "youdao.h"

#include <QDateTime>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#include <QDebug>

YouDaoApi::YouDaoApi(QObject *parent)
    : QObject(parent), m_http(new QNetworkAccessManager(this)) {}

void YouDaoApi::queryword(const QString &word) {
    QUrl url("http://dict.youdao.com/jsonresult");
    QUrlQuery query;
    query.addQueryItem("q", word);
    query.addQueryItem("type", "1");
    query.addQueryItem("client", "deskdict");
    query.addQueryItem("keyfrom", "deskdict_deepin");
    query.addQueryItem("pos", "-1");
    query.addQueryItem("len", "eng");
    url.setQuery(query.toString(QUrl::FullyEncoded));

    QNetworkRequest request(url);
    QNetworkReply *reply = m_http->get(request);

    connect(reply, &QNetworkReply::finished, this, &YouDaoApi::handleQueryWordFinished);
}

void YouDaoApi::handleQueryWordFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->error() != QNetworkReply::NoError) {
        return;
    } else {
        qDebug() << "youdao error:" << reply->errorString();
    }

    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    QJsonObject object = document.object();

    if (!document.isEmpty()) {
        QString req = object.value("rq").toString();
        QString ukPhonetic = object.value("uksm").toString();
        QString usPhonetic = object.value("ussm").toString();
        QString basicExplains("");
        QString webReferences("");

        QJsonArray explain = object.value("basic").toArray();

        // get the basic data.
        for (const QJsonValue &value : explain) {
            basicExplains.append(value.toString());
            basicExplains.append("<br>");
        }

        // Access to the web references.
        QJsonArray webRefArray = object.value("web").toArray();
        if (!webRefArray.isEmpty()) {
            for (const QJsonValue &value : webRefArray) {
                QJsonObject obj = value.toObject();
                QString key = obj.keys().first();
                QJsonArray arr = obj.value(key).toArray();

                for (const QJsonValue &value : arr) {
                    webReferences += "<br>";
                    webReferences += QString("• %1 : %2").arg(key).arg(value.toString());
                    webReferences += "</br>";
                }
            }
        }
        qDebug()
            << QString(
                   "search result for %1: \nphonetic(%2, %3). \nexplain(%4). \nref(%5)")
                   .arg(req)
                   .arg(ukPhonetic)
                   .arg(usPhonetic)
                   .arg(basicExplains)
                   .arg(webReferences);
        // emit searchFinished(std::make_tuple(queryWord, ukPhonetic, usPhonetic,
        // basicExplains, webReferences));
    }
}
void YouDaoApi::test() { YouDaoApi::instance()->queryword("hello"); }
