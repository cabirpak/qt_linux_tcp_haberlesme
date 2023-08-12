
#ifndef SERVER_H
#define SERVER_H

#include <QDialog>
#include <QLineEdit>
#include <QTcpSocket>
#include <QToolButton>
#include <QMutex>
#include <QNetworkReply>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QTcpServer;
class QNetworkSession;
QT_END_NAMESPACE

#include"textsaver.h"
#include <vector>
using namespace std;

class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = Q_NULLPTR);

private slots:
    void sessionOpened();
    void sendFortune();
    void sendAlarm(QString recvdAlarmText);
    void readMessage();
    //void disconnected();

private:
    QLabel *statusLabel;
    QLabel *statusLabel2;
    QList<QTcpServer *>tcpServers;
    QStringList fortunes;
    QNetworkSession *networkSession;
    vector<QTcpSocket *> clientConnections;
    TextSaver *textSaver;
    TextSaver *textSaver2;
    QMutex mutex;
    int portNumber;

    void log(QString line);
};

#endif
