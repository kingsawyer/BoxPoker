#ifndef NETWORK_H
#define NETWORK_H

#include <QUrl>
#include <QObject>
#include <QNetworkRequest>
#include <QNetworkReply>
class MainWindow;

class Network : public QObject
{
    Q_OBJECT
public:
    Network(MainWindow* ownerWindow);
    void SetAccessToken(QString access) {m_BoxAccessToken = access; }
    void GetUserInformation();
    void FindTableFiles(QString tableName);
    void PostLoginRequest(const QNetworkRequest& request, const QByteArray& data);
    void UploadJackpots(QString filename, QString file_id, QString etag);
    void UploadNewMoneyFile(QString filename, QString parent_id);
    void UploadExistingMoneyFile(QString filename, QString file_id);
    void DownloadJackpots(QString jackpot_id, QString destination);
    void DownloadMoneyFile(QString file_id, QString destination);

private slots:
    void replyFinish(QNetworkReply *reply);

protected:

    QNetworkReply* GetRequest(QUrl url);
    QNetworkReply* DownloadFileFromUrl(QUrl url);
    QNetworkReply*  UploadNewFile(QString filename, QString folder_id);
    QNetworkReply* UploadOverExistingFile(QString filename, QString file_id, QString etag);
    QNetworkAccessManager m_QTAccessManager;
    QString m_BoxAccessToken;
    void ParseGetUser(QNetworkReply* reply);
    void ParseGetMoneyReply(QNetworkReply* reply);
    void ParseTableInfo(QNetworkReply* reply);
    void ParseLoginReply(QNetworkReply* reply);
    void ParseJackpotReply(QNetworkReply* reply);
    void ParseJackpotUploadReply(QNetworkReply* reply);
    void ParseNewMoneyFileReply(QNetworkReply* reply);

private:
    void AddAccessToken(QNetworkRequest* request);
    void SaveFile(QNetworkReply* reply);

    QNetworkReply* m_getuser;
    QNetworkReply* m_tableinfo;
    QNetworkReply* m_loginRequest;
    QNetworkReply* m_getJackpots;
    QNetworkReply* m_setJackpots;
    QNetworkReply* m_getMoney;
    QNetworkReply* m_newMoney;
    QNetworkReply** m_current_download;
    QUrl m_current_download_URL;
    QString m_jackpot_id;
    QString m_current_download_destination;

    MainWindow* m_owner;
};

#endif // NETWORK_H
