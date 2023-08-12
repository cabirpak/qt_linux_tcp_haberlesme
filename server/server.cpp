#include <QtWidgets>
#include <QtNetwork>

#include <stdlib.h>

#include "server.h"
#include "textsaver.h"
#include <algorithm>
#include <fstream>
using namespace std;

#define KERNELDEBUGGER_SERVER_VERSION "V0.01"

Server::Server(QWidget *parent)
    : QDialog(parent)
    , statusLabel(new QLabel)
    , statusLabel2(new QLabel)
    , networkSession(0)
    , portNumber(53421)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    statusLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
                QNetworkConfiguration::Discovered) {
            config = manager.defaultConfiguration();
        }

        networkSession = new QNetworkSession(config, this);
        connect(networkSession, &QNetworkSession::opened, this, &Server::sessionOpened);

        statusLabel->setText(tr("Opening network session."));
        networkSession->open();
    } else {
        sessionOpened();
    }

    QPushButton *quitButton = new QPushButton(tr("Quit"));
    quitButton->setAutoDefault(false);
    connect(quitButton, &QAbstractButton::clicked, this, &QWidget::close);

    QPushButton *saveTextButton = new QPushButton(tr("send a debugger message"));
    quitButton->setAutoDefault(false);
    textSaver = new TextSaver("alarmtext.html", nullptr);

    connect(saveTextButton, &QAbstractButton::clicked, this, [=](){
        //textSaver->show();
        sendAlarm("deneme");
    });
    QHBoxLayout *buttonLayout = new QHBoxLayout;

    //messageEdit = new QLineEdit(tr("Alarm Message..."));

    for(int i=0; i<tcpServers.count(); i++)
        connect(tcpServers[i], &QTcpServer::newConnection, this, &Server::sendFortune);


    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    //buttonLayout->addWidget(messageEdit);
    buttonLayout->addWidget(saveTextButton);
    buttonLayout->addStretch(1);

    QVBoxLayout *mainLayout = Q_NULLPTR;
    if (QGuiApplication::styleHints()->showIsFullScreen() || QGuiApplication::styleHints()->showIsMaximized()) {
        QVBoxLayout *outerVerticalLayout = new QVBoxLayout(this);
        outerVerticalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
        QHBoxLayout *outerHorizontalLayout = new QHBoxLayout;
        outerHorizontalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
        QGroupBox *groupBox = new QGroupBox(QGuiApplication::applicationDisplayName());
        mainLayout = new QVBoxLayout(groupBox);
        outerHorizontalLayout->addWidget(groupBox);
        outerHorizontalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
        outerVerticalLayout->addLayout(outerHorizontalLayout);
        outerVerticalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    } else {
        mainLayout = new QVBoxLayout(this);
    }

    mainLayout->addWidget(new QLabel(KERNELDEBUGGER_SERVER_VERSION));
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(statusLabel2);
    mainLayout->addLayout(buttonLayout);

    setWindowTitle(QGuiApplication::applicationDisplayName());
    log(QString("server constructed. version: %1").arg(KERNELDEBUGGER_SERVER_VERSION));
}

void Server::log(QString line)
{
    mutex.lock();
    QDateTime currTime = QDateTime::currentDateTime();

    ofstream logfile;
    logfile.open("logger.txt", std::ios_base::app);
    logfile << ">>> StartMessage | ";
    logfile << currTime.toString("dd-MMM-yyyy HH:mm:ss ").toStdString();
    logfile << line.toStdString() << endl;
    logfile.close();
    mutex.unlock();
}

void Server::sessionOpened()
{
    log("session opened.");

    if(!QFile::exists("portnumber.txt")){
        ofstream ofilePortNumber("portnumber.txt");
        ofilePortNumber << portNumber;
        ofilePortNumber.close();
        log("portnumber.txt file does not exist, setting it to default (53421).");
    }else{
        ifstream ifilePourtNumber("portnumber.txt");
        ifilePourtNumber >> portNumber;
        ifilePourtNumber.close();
        log(QString("port number set to %1").arg(portNumber));
    }

    // Save the used configuration
    if (networkSession) {
        QNetworkConfiguration config = networkSession->configuration();
        QString id;
        if (config.type() == QNetworkConfiguration::UserChoice)
            id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
        else
            id = config.identifier();

        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
        settings.endGroup();
    }

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    int serverPort = 0;
    log(QString("there are %1 interfaces on server. now, trying to listen them.").arg(ipAddressesList.size()));
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (/*ipAddressesList.at(i) != QHostAddress::LocalHost &&*/
                ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();

            tcpServers.append(new QTcpServer(this));
            int serverIndex = tcpServers.count()-1;

            // if we did not find one, use IPv4 localhost
            //if (ipAddress.isEmpty())
            //ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

            if (!tcpServers[serverIndex]->listen(QHostAddress(ipAddress), portNumber)) {
                /*QMessageBox::critical(this, tr("Fortune Server"),
                                      tr("Unable to start the server: %1 %2.")
                                      .arg(ipAddress)
                                      .arg(tcpServers[serverIndex]->errorString()));*/
                tcpServers.removeAt(serverIndex);
                log(QString("listen failed for %1:%2").arg(ipAddress).arg(portNumber));
                //close();
                //return;
            }
            else{
                serverIndex = tcpServers.count()-1;

                if(serverIndex == 0)
                    statusLabel2->setText(statusLabel2->text().append("Interface IPs: ") + ipAddress);
                else
                    statusLabel2->setText(statusLabel2->text().append(", ") + ipAddress);

                serverPort = tcpServers[serverIndex]->serverPort();
                log(QString("listen success for %1:%2").arg(ipAddress).arg(serverPort));
            }
            //break;
        }
        else{
            log("ignoring interface with other than IPv4.");
        }
    }
    statusLabel->setText(tr("The server is running on port: %1\n"
                            "Run the Clients now.").arg(serverPort));
}

void Server::sendFortune()
{
    //mutex.lock();
    QTcpServer* theServer = dynamic_cast<QTcpServer*>(sender());
    qDebug() << "new connection" << endl;
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);

    out << QString("This message comes from Alarm Server.");

    QTcpSocket* lastConnectedClient = theServer->nextPendingConnection();

    log(QString("new connection: %1(%2):%3").arg(lastConnectedClient->peerAddress().toString()).
                arg(lastConnectedClient->peerName()).arg(lastConnectedClient->peerPort()));

    clientConnections.push_back(lastConnectedClient);
    connect(lastConnectedClient, &QAbstractSocket::disconnected,
            this, [=](){

        log(QString("disconnected: %1(%2):%3").arg(lastConnectedClient->peerAddress().toString()).
            arg(lastConnectedClient->peerName()).arg(lastConnectedClient->peerPort()));

        vector<QTcpSocket*>::iterator iter = std::find(clientConnections.begin(), clientConnections.end(), sender()) ;
        if(iter != clientConnections.end()){
            clientConnections.erase(iter);
            qDebug() << "erased client due it disconnected...";
        }
    });
    connect(lastConnectedClient, &QAbstractSocket::readyRead,
            this, &Server::readMessage);

    lastConnectedClient->write(block);
    //clientConnection[clientCount-1]->disconnectFromHost();
    //mutex.unlock();
}

void Server::sendAlarm(QString receivedAlarmText)
{
    for(QTcpSocket* iter : clientConnections){
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);

        QStringList words;
        QString msgToSent;

        msgToSent = QString("kerneldebugger|drm.c:222");

        out << msgToSent;

        iter->write(block);

        log(QString("alarm sent to %1(%2):%3\n").arg(iter->peerAddress().toString()).arg(iter->peerName()).arg(iter->peerPort()));
    }
}

void Server::readMessage()
{
    QDataStream in;
    QString receivedAlarmText;
    QTcpSocket* client = dynamic_cast<QTcpSocket*>(sender());
    in.setDevice(client);
    in.setVersion(QDataStream::Qt_4_0);
    in >> receivedAlarmText;
    if(receivedAlarmText == "ping"){
        in << QString("pong");
    }
    else
        sendAlarm(receivedAlarmText);
}

/*void Server::disconnected()
{
    QTcpSocket* socket = dynamic_cast<QTcpSocket*>(sender());

}*/
