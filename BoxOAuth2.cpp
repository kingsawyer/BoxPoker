#include "BoxOAuth2.h"
#include <QDebug>
#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QUrlQuery>
#include "logindialog.h"
#include "network.h"

OAuth2::OAuth2(QWidget* parent, Network* network) : m_network(network)
{
    m_codeRequestURL = "https://api.box.com/oauth2/authorize";
    m_AccessRequestURL = "https://api.box.com/oauth2/token";
    m_strScope = "";
    m_strClientID = "fnqtaemvpity6abkov5kmw3z7lwbuftc";  // insert your client ID
    m_clientSecret = "VCYd1Zo6gr1FRrd07jhhgaCP3O1YJqjN"; // insert your client secret
    m_strRedirectURI = "https://www.box.com/poker"; // insert a dummy URI here
    m_strResponseType = "code";
    m_code = "";
    m_strAccessToken = "";

    m_strCompanyName = "Box";
    m_strAppName = "Box Video Poker Slot Machine";

    m_pLoginDialog = new LoginDialog(parent, this);
    m_pParent = parent;
}

void OAuth2::setScope(const QString& scope)
{
    m_strScope = scope;
}

void OAuth2::setClientID(const QString& clientID)
{
    m_strClientID = clientID;
}

void OAuth2::setRedirectURI(const QString& redirectURI)
{
    m_strRedirectURI = redirectURI;
}

void OAuth2::setCompanyName(const QString& companyName)
{
    m_strCompanyName = companyName;
}

void OAuth2::setAppName(const QString& appName)
{
    m_strAppName = appName;
}

QString OAuth2::loginUrl()
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("client_id", m_strClientID);
    urlQuery.addQueryItem("response_type", m_strResponseType);

    QString str = QString("%1?%2").arg(m_codeRequestURL).arg(urlQuery.query(QUrl::FullyEncoded));
    // optional if you have registered redirectURL at Box
    str += QString("&redirect_uri=%1").arg(QString(QUrl::toPercentEncoding(m_strRedirectURI)));
    qDebug() << "Login URL" << str;
    return str;
}

void OAuth2::LoginUrlChanged(const QUrl& url)
{
    qDebug() << "LoginUrlChanged in OAUTH login =" << url;
    QUrlQuery query(url);

    // Not on Box, but other OAuth vendors may do this. Wow, got access right away.
    if (query.hasQueryItem("access_token")) {
        m_strAccessToken = query.queryItemValue("access_token");
        m_pLoginDialog->accept();
    }

    // Expected. Got a code good for 30 seconds.
    if (query.hasQueryItem("code")) {
        //QSettings settings(m_strCompanyName, m_strAppName);
        m_code = query.queryItemValue("code");
        //settings.setValue("code", m_code);
        m_pLoginDialog->accept();
    }
}

void OAuth2::GetTokensFromCode()
{
    QNetworkRequest request(m_AccessRequestURL);

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("grant_type", "authorization_code");
    urlQuery.addQueryItem("code", m_code);
    urlQuery.addQueryItem("client_id", m_strClientID);
    urlQuery.addQueryItem("client_secret", m_clientSecret);
    QByteArray postData;
    postData.append(urlQuery.toString(QUrl::FullyEncoded));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    m_network->PostLoginRequest(request, postData);
}

void OAuth2::GetTokensFromRefreshToken(QString refresh_token)
{
    QNetworkRequest request(m_AccessRequestURL);

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("grant_type", "refresh_token");
    urlQuery.addQueryItem("refresh_token", refresh_token);
    urlQuery.addQueryItem("client_id", m_strClientID);
    urlQuery.addQueryItem("client_secret", m_clientSecret);
    QByteArray postData;
    postData.append(urlQuery.toString(QUrl::FullyEncoded));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    m_network->PostLoginRequest(request, postData);
}

QString OAuth2::accessToken()
{
    return m_strAccessToken;
}

void OAuth2::startLogin(bool bForce)
{
    QSettings settings(m_strCompanyName, m_strAppName);
    QString str = settings.value("access_token", "").toString();

    qDebug() << "OAuth2::startLogin, token from Settings" << str;

    if(str.isEmpty() || bForce)
    {
        m_pLoginDialog->setLoginUrl(loginUrl());
        m_pLoginDialog->exec();
        GetTokensFromCode();
    }
    else
    {
        m_strAccessToken = str;
    }
}


