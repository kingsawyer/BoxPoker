#include "logindialog.h"
#include "ui_logindialog.h"

#include <QDebug>
#include <QWebView>
#include <QWebFrame>
#include <QUrlQuery>

LoginDialog::LoginDialog(QWidget *parent , OAuth2 *oauth) :
    QDialog(parent),
    m_oauth(oauth),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    connect(ui->webView, &QWebView::urlChanged, this, &LoginDialog::viewUrlChanged);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::viewUrlChanged(const QUrl &url)
{
    qDebug() << "URL =" << url;
    m_oauth->LoginUrlChanged(url);

    QUrlQuery query(url);

    if (query.hasQueryItem("code")) {
        ui->webView->hide(); // hide ugly "URL not found" we're just using the redirect to get data, not web content
    }
}


void LoginDialog::setLoginUrl(const QString& url)
{
    ui->webView->setUrl(url);
}
