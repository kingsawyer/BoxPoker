#include <QObject>
#include <QUrl>
#include <QtNetwork>
#include <QUrlQuery>
#include <QMessageBox>
#include "network.h"
#include "mainwindow.h"

Network::Network(MainWindow* ownerWindow)
{
    m_owner = ownerWindow;
    connect(&m_QTAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinish(QNetworkReply*)));

}
void Network::replyFinish(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        int err = reply->error();
        if (err > 200 && err < 300){
            // etag mismatch. Get latest etag and download file.
            m_owner->ReportEtagMismatch();
        }
        QString message = QString("A network error occurred: %1").arg(reply->errorString());
        if (reply == m_loginRequest)
            message = "Login failed";
        m_owner->ReportNetworkError(message);
        return;
    }
    // We must check for redirection. Box redirects downloads
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectionTarget.isNull()) {
        m_current_download_URL = m_current_download_URL.resolved(redirectionTarget.toUrl());
        *m_current_download = DownloadFileFromUrl(m_current_download_URL);
        return;
    }
    if (reply == m_getuser)
        ParseGetUser(reply);
    if (reply == m_getJackpots)
        ParseJackpotReply(reply);
    if (reply == m_getMoney)
        ParseGetMoneyReply(reply);
    if (reply == m_tableinfo)
        ParseTableInfo(reply);
    if (reply == m_loginRequest)
        ParseLoginReply(reply);
    if (reply == m_setJackpots)
        ParseJackpotUploadReply(reply);
    if (reply == m_newMoney)
        ParseNewMoneyFileReply(reply);
    delete reply;
}
void Network::SaveFile(QNetworkReply* reply)
{
    QFile download_file(m_current_download_destination);
    if (!download_file.open(QIODevice::WriteOnly)) {
        QMessageBox::information(m_owner, tr("Box"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(m_current_download_destination).arg(download_file.errorString()));
    }
    else {
        download_file.write(reply->readAll());
        download_file.flush();
        download_file.close();
    }
}

void Network::ParseJackpotUploadReply(QNetworkReply* reply)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    QJsonValue json_items = jsonDoc.object().value("entries");
    QJsonArray json_item_array = json_items.toArray();
    QString etag;
    for (QJsonArray::const_iterator it = json_item_array.constBegin(); it != json_item_array.constEnd(); it++) {
        QJsonObject job = (*it).toObject();
        if (m_jackpot_id.compare(job.value("id").toString()) == 0) {
            etag = job.value("etag").toString();
            break;
        }
    }
    m_owner->ReportJackpotUpload(etag);
}

void Network::ParseGetUser(QNetworkReply* reply)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    QVariantMap map;
    if (jsonDoc.isObject())
        map = jsonDoc.object().toVariantMap();
    QString user_id = map["id"].toString();
    QString user_name = map["name"].toString();
    m_owner->SetUserID(user_id, user_name);
}

void Network::ParseJackpotReply(QNetworkReply* reply)
{
    SaveFile(reply);
    m_owner->JackpotFileDownloaded();
}
void Network::ParseNewMoneyFileReply(QNetworkReply* reply)
{
    QString money_file_id;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    QJsonValue json_items = jsonDoc.object().value("entries");
    QJsonArray json_item_array = json_items.toArray();
    for (QJsonArray::const_iterator it = json_item_array.constBegin(); it != json_item_array.constEnd(); it++) {
        QJsonObject job = (*it).toObject();
        money_file_id = job.value("id").toString();
        break;
    }
    m_owner->ReportMoneyFileID(money_file_id);
}
void Network::ParseGetMoneyReply(QNetworkReply* reply)
{
    SaveFile(reply);
    m_owner->ReportMoneyFileDownloaded();
}

void Network::ParseTableInfo(QNetworkReply* reply)
{
    QString jackpot_id, jackpot_etag;
    QString moneyfile_id;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    QJsonValue json_items = jsonDoc.object().value("item_collection").toObject().value("entries");
    QJsonArray json_item_array = json_items.toArray();
    QString jackpot_filename("jackpots.txt");
    QString money_filename = m_owner->moneyFileName();
    for (QJsonArray::const_iterator it = json_item_array.constBegin(); it != json_item_array.constEnd(); it++) {
        QJsonObject job = (*it).toObject();
        if (jackpot_filename.compare(job.value("name").toString()) == 0) {
            jackpot_id = job.value("id").toString();
            jackpot_etag = job.value("etag").toString();
        }
        if (money_filename.compare(job.value("name").toString()) == 0) {
            moneyfile_id = job.value("id").toString();
        }
        if (moneyfile_id != "" && jackpot_id != "")
            break; // we have both we can stop looping
    }
    m_owner->SetTableInfo(jackpot_id, jackpot_etag, moneyfile_id);
}
void Network::ParseLoginReply(QNetworkReply* reply)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    QString accessToken = jsonDoc.object().value("access_token").toString();
    QString refreshToken = jsonDoc.object().value("refresh_token").toString();
    int expires_in = jsonDoc.object().value("expires_in").toString().toUInt();
    m_owner->SetTokens(accessToken, refreshToken, expires_in);
}

QNetworkReply* Network::GetRequest(QUrl url)
{
    QNetworkRequest Request(url);
    Request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    AddAccessToken(&Request);
    return m_QTAccessManager.get(Request);

}

void Network::PostLoginRequest(const QNetworkRequest& request, const QByteArray& data)
{
    m_loginRequest = m_QTAccessManager.post(request, data);
}

QNetworkReply* Network::DownloadFileFromUrl(QUrl url)
{
    QNetworkRequest Request(url);
    Request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    AddAccessToken(&Request);
    return m_QTAccessManager.get(Request);
}
QNetworkReply* Network::UploadOverExistingFile(QString filename, QString file_id, QString etag)
{
    QFileInfo finfo(filename);
    QString urlString = QString("https://api.box.com/2.0/files/%1/content").arg(file_id);
    QNetworkRequest Request(urlString);

    QString bound="---------------------------723690991551375881941828999";
    QByteArray data(QString("--"+bound+"\r\n").toLatin1());
    data += "Content-Disposition: form-data; name=\"action\"\r\n\r\n";
    data += "\r\n";
    data += QString("--" + bound + "\r\n").toLatin1();
    data += "Content-Disposition: form-data; name=\"file\"; filename=\""+finfo.baseName()+"\"\r\n";
    data += "Content-Type: image/"+finfo.suffix().toLower()+"\r\n\r\n";
    QFile file(finfo.absoluteFilePath());
    file.open(QIODevice::ReadOnly);
    data += file.readAll();
    data += "\r\n";
    data += QString("--" + bound + "\r\n").toLatin1();
    Request.setRawHeader(QString("Accept-Encoding").toLatin1(), QString("gzip,deflate").toLatin1());
    Request.setRawHeader(QString("Content-Type").toLatin1(),QString("multipart/form-data; boundary=" + bound).toLatin1());
    Request.setRawHeader(QString("Content-Length").toLatin1(), QString::number(data.length()).toLatin1());

    AddAccessToken(&Request);
    if (etag.size() > 0)
        // insist on etag match. Re-download if mismatch
        Request.setRawHeader(QString("If-Match").toLatin1(), etag.toLatin1());
    return m_QTAccessManager.post(Request, data);
}
QNetworkReply*  Network::UploadNewFile(QString filename, QString folder_id)
{
    QFileInfo finfo(filename);
    QString urlString("https://api.box.com/2.0/files/content");
    QNetworkRequest Request(urlString);

    QString bound="---------------------------723690991551375881941828999";
    QByteArray data;
    data += QString("--"+bound+"\r\n").toLatin1();
    data += "Content-Disposition: form-data; name=\"folder_id\"\r\n";
    data += "Content-Type: text/plain\r\n\r\n";
    data += folder_id.toLatin1();
    data += "\r\n";
    data += QString("--"+bound+"\r\n").toLatin1();
    data += "Content-Disposition: form-data; name=\"action\"\r\n\r\n";
    data += "\r\n";
    data += QString("--" + bound + "\r\n").toLatin1();
    data += "Content-Disposition: form-data; name=\"file\"; filename=\""+finfo.baseName()+"\"\r\n";
    data += "Content-Type: image/"+finfo.suffix().toLower()+"\r\n\r\n";
    QFile file(finfo.absoluteFilePath());
    file.open(QIODevice::ReadOnly);
    data += file.readAll();
    data += "\r\n";
    data += QString("--" + bound + "\r\n").toLatin1();
    Request.setRawHeader(QString("Accept-Encoding").toLatin1(), QString("gzip,deflate").toLatin1());
    Request.setRawHeader(QString("Content-Type").toLatin1(),QString("multipart/form-data; boundary=" + bound).toLatin1());
    Request.setRawHeader(QString("Content-Length").toLatin1(), QString::number(data.length()).toLatin1());

    AddAccessToken(&Request);
    return m_QTAccessManager.post(Request, data);
}

void Network::AddAccessToken(QNetworkRequest* request)
{
    QByteArray ba;
    ba.append(QString("Bearer %1").arg(m_BoxAccessToken));
    request->setRawHeader("Authorization", ba);
}

void Network::GetUserID()
{
    QUrl url(QString("https://api.box.com/2.0/users/me"));
    m_getuser = GetRequest(url);
}

void Network::FindTableFiles(QString tableName)
{
    QUrl url(QString("https://api.box.com/2.0/folders/%1").arg(tableName));
    m_tableinfo = GetRequest(url);
}

void Network::DownloadJackpots(QString file_id, QString destination)
{
    QString urlString = QString("https://api.box.com/2.0/files/%1/content").arg(file_id);
    m_current_download_URL = QUrl(urlString);
    m_current_download = &m_getJackpots;
    m_current_download_destination = destination;
    m_getJackpots = DownloadFileFromUrl(m_current_download_URL);
}
void Network::DownloadMoneyFile(QString file_id, QString destination)
{
    QString urlString = QString("https://api.box.com/2.0/files/%1/content").arg(file_id);
    m_current_download_URL = QUrl(urlString);
    m_current_download = &m_getMoney;
    m_current_download_destination = destination;
    m_getMoney = DownloadFileFromUrl(m_current_download_URL);
}
void Network::UploadJackpots(QString filename, QString file_id, QString etag)
{
    m_jackpot_id = file_id;
    m_setJackpots = UploadOverExistingFile(filename, file_id, etag);
}
void Network::UploadNewMoneyFile(QString filename, QString parent_id)
{
    m_newMoney = UploadNewFile(filename, parent_id);
}

void Network::UploadExistingMoneyFile(QString filename, QString file_id)
{
    UploadOverExistingFile(filename, file_id, "");
}
