
#ifndef CLIENT_H
#define CLIENT_H

#include "alarm.h"
#include "settings.h"

#include <QDialog>
#include <QTcpSocket>
#include <QDataStream>
#include <QToolButton>
#include <QTextBrowser>
#include <QMediaPlayer>
#include <QMutex>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTcpSocket;
class QNetworkSession;
QT_END_NAMESPACE

class Client : public QDialog
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = Q_NULLPTR);
    void renameLogFile();

public slots:
    void acceptAlarm();
    void readAlarm();
    void displayError(QAbstractSocket::SocketError socketError);
    void disconnected();
    void enableGetAlarmButton();
    void sessionOpened();
    void sendAlarm();
    void connectToServer(QString host, int port);

private:
    QLabel *statusLabel;
    //QToolButton *settingsButton;
    Settings* settings;

    QTcpSocket *tcpSocket;
    QDataStream inDataStream;

    QNetworkSession *networkSession;
    bool closing;
    bool closingTcpSocket;
    Alarm *receivedAlarmWindow;
    QTextBrowser* tb;
    QMediaPlayer *player;
    bool hasAlarmClicked;
    bool hasAcceptClicked;
    QMutex mutex;
    QTimer* timerForPingPong;
    QTimer* timerForReconnecting;
    QTimer* timerForTruncatingLog;
    int logRefreshTimerCounter;

    QMap<QPushButton*, Alarm*> pushButtonOwners;

    void log(QString line);

protected:
    void changeEvent(QEvent* e);
    void closeEvent(QCloseEvent *event) override;

    void tryToReconnect();
};

#endif
