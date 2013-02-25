#ifndef OAUTH2_H
#define OAUTH2_H

#include <QString>
#include <QObject>
#include "network.h"

class LoginDialog;

class OAuth2 : public QObject
{
    Q_OBJECT

public:
    OAuth2(QWidget* parent, Network* network);
    QString accessToken();
    bool isAuthorized();
    void startLogin(bool bForce);

    //Functions to set application's details.
    void setScope(const QString& scope);
    void setClientID(const QString& clientID);
    void setRedirectURI(const QString& redirectURI);
    void setCompanyName(const QString& companyName);
    void setAppName(const QString& appName);
    void GetTokensFromRefreshToken(QString refresh_token);

    void LoginUrlChanged(const QUrl& url);

    QString loginUrl();
    QString AccessUrl();


private:
    void GetTokensFromCode();

    Network* m_network;
    QString m_code;
    QString m_strAccessToken; //? scrap

    QString m_codeRequestURL;
    QString m_AccessRequestURL;
    QString m_strScope;
    QString m_strClientID;
    QString m_clientSecret;
    QString m_strRedirectURI;
    QString m_strResponseType;

    QString m_strCompanyName;
    QString m_strAppName;

    LoginDialog* m_pLoginDialog;
    QWidget* m_pParent;
};

#endif // OAUTH2_H
