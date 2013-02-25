#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QDialog>
#include <QFile>
#include <QtNetwork/QNetworkReply>
#include <QPropertyAnimation>
#include <QElapsedTimer>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QSequentialAnimationGroup>
#include <QTime>
#include "pokerhand.h"
#include "network.h"

#define NOCARD -1
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    enum GameState { Login, Start, Pick, Refilling, WaitingForPayout, Results, Bankrupt };
    enum Celebrate { None, Small, Medium, Large };
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void ReportEtagMismatch();
    void ReportJackpotUpload(QString etag);
    void ReportNetworkError(QString message);
    void ReportMoneyFileID(QString money_file_id);
    void ReportMoneyFileDownloaded();
    QString moneyFileName();
    void SetTableInfo(QString jackpot_id, QString jackpot_etag, QString moneyfile_id);
    void SetTokens(QString m_accessToken, QString m_refreshToken, int expires_in);
    void SetUserID(QString user_id, QString username);
    void JackpotFileDownloaded();

private slots:
    void on_tableName_returnPressed();

    void on_action_clicked();

    void on_card1_clicked();

    void on_card2_clicked();

    void on_card3_clicked();

    void on_card4_clicked();

    void on_card5_clicked();

    void on_bet0_clicked();

    void on_bet1_clicked();

    void on_bet2_clicked();

    void on_bet3_clicked();

    void on_LoginButton_clicked();

    void on_tester_clicked();

    void _on_animation_finished();

private:
    void PlaySound(QString sound);
    void DoCelebrate(Celebrate amount);
    void ClearCelebrate();

    void WonJackpot(HandRank rank);
    void ClickedBet();

    // for sound effects
    QMediaPlayer* m_player;


    void goto_State(GameState state);
    void UpdateAccount(int delta);
    bool UpdateJackpots();
    int BetAmount();
    bool UserGoofed();


    QPushButton* getButton(int buttonNum);
    void EnableCards(bool enable);

    void DrawHand();
    void DisplayHand();
    void ReplenishHand();
    void Evaluate();

    void swap_card(int card);

    int hand[5];
    int used[5];
    int jackpots[6];
    int jackpot_kitty;
    int loss_count;
    int hands_since_last_money_update;
    QString winners[6];
    int money;
    int bet;
    int lastJackpotWin;
    Celebrate m_celebrate;
    HandRank lastJackpotRank;
    int saved_progressive_win;
    // user identification
    QString m_user_id; // unchanging number
    QString m_username; // user can change. Human readable "John Doe"
    QString GetMoneyFilename();

    QString m_accessToken;
    QString m_refreshToken;
    QTime m_refreshTime;
    QTime m_refresh_limit_Time; // At this time we will no longer refresh tokens

    QString m_tableFolder_id; //box_id of folder
    QString m_jackpot_id; //box_id
    QString m_money_file_id; //box_id
    QString m_jackpot_etag; //last known etag on Box

    // temporary folder for storing game data
    QString m_localFolder;

    GameState m_gamestate;

    // for animations
    QSequentialAnimationGroup m_anim_group;
    QPropertyAnimation* m_anim_celebrate;
    QPropertyAnimation* m_anim_celebrate_fade;
    QPropertyAnimation* m_anim_deal_card[5];
    int m_celebrate_offscreen_y;

    void SetCardImage(QPushButton* button, int cardID);

    void MoveCardOffscreen(int card_num);
    void clearCards();
    void EndOfHand();
    void SetupMoney();

    void UploadJackpotsFile();
    void UploadMoney();


    bool ReadJackpotsAndWinners();
    bool WriteJackpotsAndWinners();
    void ReadMoneyFile();
    void WriteMoneyFile();

    Network m_network;

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
