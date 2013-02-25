#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QString>
#include <QUrl>
#include "BoxOAuth2.h"

typedef void (*urlCallback)(const QUrl &url);
namespace Ui {
    class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent, OAuth2* oauth);
    ~LoginDialog();
    void setLoginUrl(const QString& url);

signals:
    void urlChanged(const QUrl& url);

public slots:
    void viewUrlChanged(const QUrl& url);
    void loadStarted();
    void loadFinished(bool);


private:
    Ui::LoginDialog *ui;
    OAuth2* m_oauth;
};

#endif // LOGINDIALOG_H
