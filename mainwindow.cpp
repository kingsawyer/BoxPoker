#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QBitmap>
#include <QUrl>
#include <QtNetwork>

#include "logindialog.h"
#include "BoxOAuth2.h"

static const int kMaxBet = 100;
static const QString kAppName = "Box Video Poker";

// Initializer for main window
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), m_network(this),
    ui(new Ui::MainWindow)
{
    m_gamestate = Login;
    loss_count =0;
    lastJackpotRank = eBust; //aka none
    for (int i=0; i<5; i++) {
        hand[i]=NOCARD;
        used[i]=NOCARD;
    }
    m_localFolder = QDir::tempPath();

    ui->setupUi(this);
    ui->table_instructions->hide();
    ui->tableName->setEnabled(false);
    ui->account->hide();
    ui->action->hide();
    ui->celebrate_pic->hide();
    ui->tester->hide();
    bet = 10;

    m_player = new QMediaPlayer();
    m_player->setVolume(100);
    m_celebrate = None;

    //m_celebrate_start_y = ui->celebrate_pic->pos().y();
    m_celebrate_offscreen_y = - ui->celebrate_pic->size().height();

    m_anim_celebrate = new QPropertyAnimation(ui->celebrate_pic, "pos", this);
    m_anim_celebrate->setEasingCurve(QEasingCurve::OutBounce);
    m_anim_celebrate->setDuration(1500);
    m_anim_celebrate->setEndValue(ui->celebrate_pic->pos());
    m_anim_celebrate->setStartValue(QPoint(ui->celebrate_pic->pos().x(), m_celebrate_offscreen_y));
    m_anim_celebrate_fade = new QPropertyAnimation(ui->celebrate_pic, "pos", this);
    m_anim_celebrate_fade->setEasingCurve(QEasingCurve::InBounce);
    m_anim_celebrate_fade->setDuration(1000);
    m_anim_celebrate_fade->setEndValue(QPoint(this->width(), ui->celebrate_pic->pos().y()));
    m_anim_celebrate_fade->setStartValue(ui->celebrate_pic->pos());
    for (int i=0; i<5; i++) {
        m_anim_deal_card[i] = new QPropertyAnimation(getButton(i), "pos", this);
        m_anim_deal_card[i]->setEasingCurve(QEasingCurve::OutQuad);
        m_anim_deal_card[i]->setDuration(400);
        QPoint pos = getButton(i)->pos();
        m_anim_deal_card[i]->setEndValue(pos);
        m_anim_deal_card[i]->setStartValue(QPoint(this->width() /2 , - pos.ry()));
    }
    goto_State(Login);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Plays a sound located in the same directory as the application
// The media player does not support embedding sounds as resources
void MainWindow::PlaySound(QString sound)
{
    QString appdir_path = QCoreApplication::applicationDirPath();
    QString sound_path = QString("%1/%2").arg(appdir_path).arg(sound);
    QUrl sound_url = QUrl::fromLocalFile(sound_path);
    m_player->setMedia(sound_url);
    m_player->play();
}

// New access and refresh tokens obtained. Replace old tokens
void MainWindow::ReportTokens(QString accessToken, QString refreshToken, int expires_in)
{
    m_network.SetAccessToken(accessToken);
    m_accessToken = accessToken;
    m_refreshToken = refreshToken;
    // if user is active within 30 minutes of expiration or half of token time (whichever is less)
    // auto-refresh their accessToken
    // after refreshTime we'll refresh tokens
    m_refreshTime = QTime::currentTime();
    m_refreshTime.addSecs(expires_in >= 3600 ? 1800 : expires_in / 2);
    // if idle at refresh_limit_time, we'll let access token expire
    m_refresh_limit_Time = QTime::currentTime();
    m_refresh_limit_Time.addSecs(3600);
    // Get user number and user name
    m_network.GetUserInformation();
}

// Attempt to upload the jackpot file
void MainWindow::UploadJackpotsFile()
{
    QString jackpotfileName = m_localFolder + "/jackpots.txt";
    m_network.UploadJackpots(jackpotfileName, m_jackpot_id, m_jackpot_etag);
}
void MainWindow::UploadMoney()
{
    if (m_money_file_id == "")
        m_network.UploadNewMoneyFile(GetMoneyFilename(), m_tableFolder_id);
    else
        m_network.UploadExistingMoneyFile(GetMoneyFilename(), m_money_file_id);
}

void MainWindow::SetupMoney()
{
    hands_since_last_money_update = 0;
    if (m_money_file_id != "") {
        m_network.DownloadMoneyFile(m_money_file_id, GetMoneyFilename());
    }
    else {
        money = 1000; //no existing money file, start with default 1000
        UpdateAccount(0);
        goto_State(Start);
    }
}

bool MainWindow::UpdateJackpots()
{
    // Read in file and update UI
    if (!ReadJackpotsAndWinners())
        return false;
    int jp =0;
    QString payout;
    ui->payouts->clear();
    for (int iHR=eRoyal; iHR>eBust; iHR--) {
        HandRank i = (HandRank)iHR;
        int val = bet * BasePayout(i);
        payout = QString(RankToText(i)) + QString(": ") + QString("%1").arg(val);
        if (i >= eStraight) {
            int extra = jackpots[jp++] * bet / kMaxBet;
            if (extra)
                payout = payout + QString(" + %1").arg(extra);
        }
        ui->payouts->append(payout);
    }
    return true;
}

bool MainWindow::ReadJackpotsAndWinners()
{
    QString jackpotfileName = m_localFolder + "/jackpots.txt";
    QFile jackpotfile(jackpotfileName);
    if (!jackpotfile.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    for (int i=0;i<6;i++) {
        jackpot_kitty=0;
        jackpots[i] = 0;
        winners[i]=QString("");
    }
    QTextStream in(&jackpotfile);
    int count=0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(",");
        if (count < 6) {
            jackpots[count] = fields.at(0).toInt();
            winners[count] = fields.at(1);
            count++;
        }
    }
    jackpotfile.close();
    if (lastJackpotRank) {
        //we're re-reading due to etag mismatch
        WonJackpot(lastJackpotRank);
    }
    ui->winners->clear();
    for (int i=0;i<6;i++)
        ui->winners->append(QString(winners[i]));

    return true;
}

bool MainWindow::WriteJackpotsAndWinners()
{
    QString jackpotfileName = m_localFolder + "/jackpots.txt";
    QFile jackpotfile(jackpotfileName);
    if (!jackpotfile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&jackpotfile);
    for (int i=0;i<6;i++) {
        jackpots[i] += jackpot_kitty / 6;
        out << jackpots[i] << "," << winners[i] << "\r\n";
    }
    jackpotfile.close();
    return true;
}

QString getCardStr(int cardnum)
{
    if (cardnum == NOCARD)
        return QString(":/images/cards/Box_Back.svg");
    else
        return QString(":/images/cards/%1.svg").arg(cardnum);
}
QIcon getCard(int cardnum)
{
    QIcon result(getCardStr(cardnum));
    return result;
}
QPushButton* MainWindow::getButton(int buttonNum)
{
    switch (buttonNum) {
    case 0: return ui->card1;
    case 1: return ui->card2;
    case 2: return ui->card3;
    case 3: return ui->card4;
    case 4: return ui->card5;
    }
    return NULL;
}
void MainWindow::UpdateAccount(int delta)
{
    money += delta;
    if (money < 0)
        money = 0;

    ui->account->setText(QString("$%1").arg(money));
}

void MainWindow::SetCardImage(QPushButton* button, int cardID)
{
//    QIcon icon;
//    icon.addFile(getCardStr(cardID), button->size());
    button->setIcon(QIcon(getCardStr(cardID)));
}

void MainWindow::clearCards()
{
    for (int i=0; i<5; i++)
        SetCardImage(getButton(i), NOCARD);
}

void MainWindow::MoveCardOffscreen(int card_num)
{
    getButton(card_num)->move(-1000,-1000);
}

void MainWindow::on_tableName_returnPressed()
{
    m_tableFolder_id = ui->tableName->text();
    m_jackpot_id = "";
    if (m_tableFolder_id != "")
        m_network.FindTableFiles(m_tableFolder_id);
}

void MainWindow::DrawHand()
{
    bool found;
    while(m_anim_group.animationCount())
        m_anim_group.takeAnimation(0);
    for (int i=0; i< 5; i++) {
        MoveCardOffscreen(i);
        do {
            found = false;
            hand[i]= 1+ rand() % 52;
            for (int j=0; j<i && !found;j++)
                if (hand[i] == hand[j])
                    found = true;
        } while (found);
        SetCardImage(getButton(i), hand[i]);
        m_anim_group.addAnimation(m_anim_deal_card[i]);
        used[i] = NOCARD;
    }
    PlaySound("slide5.mp3");
    m_anim_group.start();
}

void MainWindow::ReplenishHand()
{
    bool found;
    while(m_anim_group.animationCount())
        m_anim_group.takeAnimation(0);
    for (int i=0; i< 5; i++) {
        if (hand[i] == NOCARD) {
            MoveCardOffscreen(i);
            do {
                found = false;
                hand[i]= rand() % 52 + 1;
                for (int j=0; j<5 && !found;j++)
                    if (i != j && (hand[i] == hand[j] || hand[i] == used[j]))
                        found = true;
            } while (found);
            SetCardImage(getButton(i), hand[i]);
            m_anim_group.addAnimation(m_anim_deal_card[i]);
        }
    }
    this->connect(&m_anim_group, SIGNAL(finished()), SLOT(_on_animation_finished()));
    PlaySound(QString("slide%1.mp3").arg(m_anim_group.animationCount()));
    m_anim_group.start();
}

void MainWindow::_on_animation_finished()
{
    if (m_gamestate == Refilling) {
        Evaluate();
        EndOfHand();
    }
}

void MainWindow::DisplayHand()
{
    for (int i=0; i<5; i++)
        SetCardImage(getButton(i), hand[i]);
}

void MainWindow::EnableCards(bool enable)
{
    for (int i=0; i<5; i++)
        getButton(i)->setEnabled(enable);
}

int MainWindow::BetAmount()
{
    if (ui->bet0->isChecked())
        return 10;
    if (ui->bet1->isChecked() && money >= 25)
        return 25;
    if (ui->bet2->isChecked() && money >= 50)
        return 50;
    if (ui->bet3->isChecked() && money >= 100)
        return 100;
    ui->bet0->setChecked(true);
    return 10;
}

void MainWindow::goto_State(GameState state)
{
    m_gamestate = state;
    switch (m_gamestate) {
    case Login:
        ui->tableName->setEnabled(false);
        break;
    case Pick:
        bet = BetAmount();
        ui->instructions->setText("Select cards to discard then press Draw");
        ui->action->setText("Draw!");
        ui->result_msg->setText("");
        DrawHand();
        DisplayHand();
        EnableCards(true);
        break;
    case Results:
        ui->instructions->setText("");
        DisplayHand();
        //EnableCards(false);
        ui->action->setText("Play Again");
        break;
    case Start:
        ui->instructions->setText("Select a bet amount.\nPayoff shows the fixed payoff plus any additional progressive jackpot.");
        ui->action->setText("Bet!");
        ui->result_msg->setText("");
        clearCards();
        ClearCelebrate();
        break;
    case Bankrupt:
        DisplayHand();
        ui->instructions->setText("You ran out of money.\nYou must admit you are a loser,\nbut then you'll get another $1000");
        ui->action->setText("I'm a loser.\r\nHelp me.");
        break;
    }
}
void MainWindow::WonJackpot(HandRank rank)
{
    goto_State(WaitingForPayout);
    int jackpot_index = eRoyal - rank;
    saved_progressive_win = jackpots[jackpot_index] * bet / kMaxBet;
    jackpots[jackpot_index] -= saved_progressive_win;
    winners[jackpot_index] = m_username;
    WriteJackpotsAndWinners();
    UploadJackpotsFile();
}


void MainWindow::Evaluate()
{
    QString deb = QString("%1 + ").arg(hand[0]) +QString("%1+").arg(hand[1])+QString("%1+").arg(hand[2]) + QString("%1+").arg(hand[3])+ QString("%1+").arg(hand[4]);
    //ui->debug->setText(deb);
    HandRank rank = EvaluateHand(hand);
    //rank = eTwoPair; //! uncomment to test wins. You cheater.
    if (rank > eBust)
        ui->last_win->setText(QString("last win was: ") + QString(RankToText(rank)));
    int payout = bet * BasePayout(rank);
    lastJackpotWin = 0;
    saved_progressive_win = 0;
    lastJackpotRank = eBust; //aka none
    if (rank >= eStraight) {
        lastJackpotWin = payout; // start with payout and add progressive
        lastJackpotRank = rank;
        WonJackpot(rank);
    }
    else {
        if (rank == ePair)
            DoCelebrate(Small);
        else if (rank > ePair){
            DoCelebrate(Medium);
        }
        if (payout > 0) {
            ui->result_msg->setText(QString("You won $%1").arg(payout));
        }
        else {
            loss_count++;
            jackpot_kitty += bet * 3 / 2;
            ui->result_msg->setText("So close! Try again.");
            if (loss_count > 5) {
                WriteJackpotsAndWinners();
                UploadJackpotsFile();
            }
            PlaySound("too bad.mp3");
        }
        UpdateAccount(payout);
    }
}
bool MainWindow::UserGoofed()
{
    // Check to see if they pressed Draw with no cards selected to discard
    for (int i=0; i<5; i++)
        if (hand[i] == NOCARD)
            return false;
    if (EvaluateHand(hand) < eStraight) {
        QMessageBox msgBox(QMessageBox::Question, kAppName,
                           "Are you sure you don't want to discard some cards and try for a better hand?",
                           QMessageBox::Yes | QMessageBox::No, this);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.exec();
        if (msgBox.result() != QMessageBox::Yes)
            return true;
    }
    return false;
}

void MainWindow::on_action_clicked()
{
    switch (m_gamestate) {
    case Bankrupt:
        money = 1000;
        UpdateAccount(0);
        goto_State(Start);
        ui->last_win->setText("Coming out of bankruptcy");
        break;
    case Start:
        UpdateAccount(- bet);
        goto_State(Pick);
        break;
    case Pick:
        if (!UserGoofed()) {
            ReplenishHand();
            goto_State(Refilling);
        }
        break;
    case Results:
        goto_State(Start);
        break;
    }
    if (QTime::currentTime() > m_refreshTime && QTime::currentTime() <= m_refresh_limit_Time) {
        OAuth2 auth(this, &m_network);
        auth.GetTokensFromRefreshToken(m_refreshToken);
    }
}

void MainWindow::swap_card(int card)
{
    if (m_gamestate == Pick) {
        int temp=hand[card];
        hand[card]=used[card];
        used[card]=temp;
        getButton(card)->setIcon(getCard(hand[card]));
    }
}

void MainWindow::on_card1_clicked()
{
    swap_card(0);
}

void MainWindow::on_card2_clicked()
{
    swap_card(1);
}

void MainWindow::on_card3_clicked()
{
    swap_card(2);
}

void MainWindow::on_card4_clicked()
{
    swap_card(3);
}

void MainWindow::on_card5_clicked()
{
    swap_card(4);
}

void MainWindow::ClickedBet()
{
    if (m_gamestate == Start || m_gamestate == Results) {
        bet = BetAmount();
        UpdateJackpots();
    }
}

void MainWindow::on_bet0_clicked()
{
    ClickedBet();
}

void MainWindow::on_bet1_clicked()
{
    ClickedBet();
}

void MainWindow::on_bet2_clicked()
{
    ClickedBet();
}

void MainWindow::on_bet3_clicked()
{
    ClickedBet();
}

void MainWindow::ReportJackpotUpload(QString etag)
{
    int payout = lastJackpotWin + saved_progressive_win;
    m_jackpot_etag = etag;
    saved_progressive_win = 0;
    lastJackpotWin =0;
    // we upload the jackpots because of a win or enough losses
    if (payout > 0) {
        ui->result_msg->setText(QString("You won $%1").arg(payout));
        DoCelebrate(Large);
    }
    UpdateAccount(payout);
    jackpot_kitty = 0;
    lastJackpotRank = eBust;
    UpdateJackpots();
    EndOfHand();
}

void MainWindow::ReportMoneyFileID(QString money_file_id)
{
    m_money_file_id = money_file_id;
}

// The hash is just to make it a little more difficult to give yourself a lot of money
QString GetHash(QString user_id, QString moneyline)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData("Box Poker");
    hash.addData(user_id.toLatin1());
    hash.addData(moneyline.toLatin1());
    return hash.result().toHex();
}

void MainWindow::WriteMoneyFile()
{
    QFile moneyFile(GetMoneyFilename());
    if (!moneyFile.open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("Box"),
                                 tr("Unable to save data %1: %2.")
                                 .arg(moneyFileName()).arg(moneyFile.errorString()));
    }
    else {
        QString moneyline = QString("%1").arg(money);
        moneyFile.write(moneyline.toLatin1());
        moneyFile.write("\r\n");
        moneyFile.write(GetHash(m_user_id, moneyline).toLatin1());
        moneyFile.flush();
        moneyFile.close();
    }
}
void MainWindow::ReadMoneyFile()
{
    QFile moneyFile(GetMoneyFilename());
    money = 1000;
    if (moneyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&moneyFile);
        QString moneyline = in.readLine();
        QString hashline = in.readLine();
        moneyFile.close();
        if (GetHash(m_user_id, moneyline) == hashline)
            money = moneyline.toInt();
    }
    UpdateAccount(0);
    goto_State(Start);
}

void MainWindow::EndOfHand()
{
    WriteMoneyFile();
    UploadMoney();
    if (money > 10)
        goto_State(Results);
    else
        goto_State(Bankrupt);
}

void MainWindow::ReportUserInformation(QString user_id, QString username)
{
    m_user_id = user_id;
    m_username = username;
    ui->login_label->hide();
    ui->table_instructions->show();
    ui->instructions->setText("");
    ui->tableName->setEnabled(true);
}
void MainWindow::ReportEtagMismatch()
{
    // we only use etag matching for jackpot file
    // re-get the ids and etags for our files
    if (m_tableFolder_id != "")
        m_network.FindTableFiles(m_tableFolder_id);
}

void MainWindow::ReportNetworkError(QString message)
{
    ui->instructions->setText(message);
}

void MainWindow::ReportTableInfo(QString jackpot_id, QString jackpot_etag, QString moneyfile_id)
{
    if (jackpot_id != "") {
        m_jackpot_id = jackpot_id;
        m_jackpot_etag = jackpot_etag;
        m_money_file_id = moneyfile_id;
        m_network.DownloadJackpots(m_jackpot_id, m_localFolder + "/jackpots.txt");
        ui->tableName->setEnabled(false);
        ui->table_instructions->hide();
        ui->account->show();
        ui->action->show();
    }
    else {
        QMessageBox msgBox(this);
        msgBox.setText("That does not appear to be a valid table.");
        msgBox.exec();
    }

}

QString MainWindow::moneyFileName()
{
    // We use user ID because users can change their email addresses, but
    // cannot change their user ID
    return QString("player_%1.txt").arg(m_user_id);
}

QString MainWindow::GetMoneyFilename()
{
    return QString(m_localFolder + "/" + moneyFileName());
}

void MainWindow::ReportJackpotFileDownloaded()
{
    if (!UpdateJackpots()) {
        QMessageBox msgBox(this);
        msgBox.setText("That does not appear to be a valid table. Jackpots file is invalid.");
        msgBox.exec();
        ui->tableName->setEnabled(true);
        ui->table_instructions->show();
        return;
    }
    if (m_gamestate == Login) {
        SetupMoney();
    }
    else if (m_gamestate == WaitingForPayout) {
        // let's try once more to make a payout, but with the new jackpot amounts
        Evaluate();
    }
}

void MainWindow::ReportMoneyFileDownloaded()
{
    ReadMoneyFile();
}

void MainWindow::on_LoginButton_clicked()
{
    OAuth2 login(this, &m_network);
    login.startLogin();
    return;
}
void MainWindow::DoCelebrate(Celebrate amount)
{
    m_celebrate = amount;
    QPixmap pixmap;
    switch (m_celebrate)
    {
    case Small:
        pixmap = QPixmap(QString(":/images/celebrations/%1.jpg").arg(1));
        ui->celebrate_pic->setPixmap(pixmap);
        m_anim_celebrate->start();
        ui->celebrate_pic->show();
        PlaySound("small win.mp3");
        break;
    case Medium:
        pixmap = QPixmap(":/images/celebrations/ukulele.jpg");
        ui->celebrate_pic->setPixmap(pixmap);
        m_anim_celebrate->start();
        ui->celebrate_pic->show();
        PlaySound("uke.mp3");
        break;
    case Large:
        pixmap = QPixmap(":/images/celebrations/giraffe.jpg");
        ui->celebrate_pic->setPixmap(pixmap);
        m_anim_celebrate->start();
        ui->celebrate_pic->show();
        PlaySound("large win.mp3");
        break;
    }
}
void MainWindow::ClearCelebrate()
{
    switch (m_celebrate)
    {
    case Small:
        m_anim_celebrate_fade->start();
        break;
    case Medium:
        m_anim_celebrate_fade->start();
        break;
    case Large:
        m_anim_celebrate_fade->start();
        break;
    }
    m_player->stop();
    m_celebrate = None;
}

void MainWindow::on_tester_clicked()
{
    static int tester = 200;
    tester++;

    if (tester == 1) {
        QPixmap pixmap(QString(":/images/celebrations/%1.jpg").arg(1));
        ui->celebrate_pic->setPixmap(pixmap);
        //ui->celebrate_pic->setMask(pixmap.mask());
        m_anim_celebrate->start();
        ui->celebrate_pic->show();
    }
    else if (tester == 2) {
        m_anim_celebrate_fade->start();
    }
    else if (tester == 13) {
        DrawHand();
        EnableCards(true);
        m_gamestate = Pick;
    }
    else if (tester == 13) {
        DrawHand();
        EnableCards(true);
        m_gamestate = Pick;
    }
    else if (tester == 14) {
        ReplenishHand();
    }
    else if (tester == 3) {
        DoCelebrate(Small);
    }
    else if (tester == 4) {
        ClearCelebrate();
    }
    else if (tester == 5) {
        DoCelebrate(Medium);
    }
    else if (tester == 6) {
        ClearCelebrate();
    }
    else if (tester == 7) {
        DoCelebrate(Large);
    }
    else if (tester == 8) {
        ClearCelebrate();
    }
    else if (tester == 7) {
        PlaySound("slide2.mp3");
    }
    else if (tester == 8) {
        PlaySound("slide3.mp3");
    }
    else if (tester == 9) {
        PlaySound("slide4.mp3");
    }
    else if (tester == 10) {
        PlaySound("slide5.mp3");
    }
    else if (tester == 7) {
        PlaySound("swing.wav");
    }
    else if (tester == 8) {
        PlaySound("whoosh.wav");
    }
    else if (tester == 9) {
        PlaySound("too bad.mp3");
    }
}
