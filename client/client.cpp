#include <QtWidgets>
#include <QtNetwork>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCloseEvent>
#include <QMediaPlayer>

#include "alarm.h"
#include "client.h"
#include <fstream>
using namespace std;


void Client::renameLogFile()
{
    log("closing this log file and reoping a new one... bye...");

    mutex.lock();

    QDateTime currTime = QDateTime::currentDateTime();

    QFile toRename("logger.txt");
    if(toRename.exists())
        toRename.rename(currTime.toString("logger_dd_MMM_yyyy__HH_mm_ss.log"));

    mutex.unlock();
}

Client::Client(QWidget *parent)
    : QDialog(parent)
    //, settingsButton(new QToolButton())
    , tcpSocket(new QTcpSocket(this))
    , networkSession(Q_NULLPTR)
    , settings(new Settings)
    , player(new QMediaPlayer)
    , timerForPingPong(nullptr)
    , receivedAlarmWindow(nullptr)
    , timerForReconnecting(nullptr)
{
    //TRAY START
    closing = false;
    closingTcpSocket = false;
    timerForTruncatingLog = new QTimer(this);

    logRefreshTimerCounter = 120;

    connect(timerForTruncatingLog, &QTimer::timeout, this, [=](){
        logRefreshTimerCounter--;
        if(logRefreshTimerCounter%10 == 0){
            log(QString("log trunctaion will be occurred after %1 minutes.").arg(logRefreshTimerCounter));
        }
        if(logRefreshTimerCounter <= 0){
            logRefreshTimerCounter = 120;
            renameLogFile();
        }
    }
        , Qt::QueuedConnection);//separate thread
    timerForTruncatingLog->start(60000); // logs truncated periodically by 3 hours.

    renameLogFile();

    auto exitAction = new QAction(tr("&Exit"), this);
    connect(exitAction, &QAction::triggered, [this]()
    {
        closing = true;
        log("app will be closed.");
        qApp->exit(0);
        //close();
    });

    auto settingsAction = new QAction(tr("&Settings"), this);
    connect(settingsAction, &QAction::triggered, [this]()
    {
        settings->show();
    });

    auto trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(exitAction);
    trayIconMenu->addAction(settingsAction);

    auto sysTrayIcon = new QSystemTrayIcon(this);
    sysTrayIcon->setContextMenu(trayIconMenu);
    sysTrayIcon->setIcon(QIcon("images/panicbutton.png"));
    sysTrayIcon->show();

    connect(sysTrayIcon, &QSystemTrayIcon::activated, [this](auto reason)
    {
        if(reason == QSystemTrayIcon::Trigger)
        {
            if(isVisible())
            {
                hide();
            }
            else
            {
                show();
                activateWindow();
            }
        }
    });
    //TRAY END



    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    statusLabel = new QLabel(tr("log dosyasina bakiniz."));

    //settingsButton->setText("Settings");
    connect(settings, &Settings::connectToServer, this, &Client::connectToServer);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->setOrientation(Qt::Vertical);
    //buttonBox->addButton(settingsButton, QDialogButtonBox::ActionRole);
    //buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    QGridLayout *mainLayout = Q_NULLPTR;
    if (QGuiApplication::styleHints()->showIsFullScreen() || QGuiApplication::styleHints()->showIsMaximized()) {
        QVBoxLayout *outerVerticalLayout = new QVBoxLayout(this);
        outerVerticalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
        QHBoxLayout *outerHorizontalLayout = new QHBoxLayout;
        outerHorizontalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
        QGroupBox *groupBox = new QGroupBox(QGuiApplication::applicationDisplayName());
        mainLayout = new QGridLayout(groupBox);
        outerHorizontalLayout->addWidget(groupBox);
        outerHorizontalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
        outerVerticalLayout->addLayout(outerHorizontalLayout);
        outerVerticalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    } else {
        mainLayout = new QGridLayout(this);
    }
    mainLayout->addWidget(statusLabel, 3, 0, 1, 2);
    mainLayout->addWidget(buttonBox, 4, 0, 1, 2);

    setWindowTitle(QGuiApplication::applicationDisplayName());

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
        connect(networkSession, &QNetworkSession::opened, this, &Client::sessionOpened);
        connect(networkSession, &QNetworkSession::closed, this, [=](){
            statusLabel->setText("Connection lost!");
        });

        statusLabel->setText(tr("Opening network session."));
        networkSession->open();
    }

    connectToServer(settings->hostCombo->currentText(), settings->portLineEdit->text().toInt());

    QPixmap pixmap( 32, 32 );
    pixmap.fill( Qt::transparent );
    setWindowIcon(QIcon(pixmap));

    setWindowTitle("");//"Panic");

    /*QFile file("stylesheet.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    //setStyleSheet(styleSheet);

    file.close();*/

    log(QString("client started. version: %1").arg(ALARM_CLIENT_VERSION));
    log("client constructed.");
    log(QString("sleep_avoider_period: %1, sleep_avoider_should_be_log: %2").arg(settings->sleep_avoider_period).arg(settings->sleep_avoider_should_be_log));

}

#if 0
void Client::connectToServer()
{
    //getAlarmButton->setEnabled(false);
    tcpSocket->abort();

    tcpSocket->connectToHost(hostCombo->currentText(),
                             portLineEdit->text().toInt());
    /*unsigned char data[16]{0x81, 0x01, 0x6, 0x4, 0xff};
    tcpSocket->write((const char*)data, 5);*/

}
#endif
void Client::connectToServer(QString host, int port)
{
    log("in connectToServer method.");

    closingTcpSocket = true;
    //tcpSocket->close();
    //delete tcpSocket;
    log("tcp socket deleted.");

    tcpSocket = new QTcpSocket(this);

    inDataStream.setDevice(tcpSocket);
    inDataStream.setVersion(QDataStream::Qt_4_0);

    connect(tcpSocket, &QIODevice::readyRead, this, &Client::readAlarm);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Client::displayError);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &Client::disconnected);

    closingTcpSocket = false;

    tcpSocket->connectToHost(host, port);
    log(QString("tcp socket called connectToHost. %1:%2").arg(host).arg(port));
}

void Client::log(QString line)
{
    if(!settings->should_log_events)
        return;
    mutex.lock();

    QDateTime currTime = QDateTime::currentDateTime();

    ofstream logfile;
    logfile.open("logger.txt", std::ios_base::app);
    //logfile << ">>> StartMessage | ";
    logfile << currTime.toString("dd-MMM-yyyy HH:mm:ss | ").toStdString();
    logfile << line.toStdString() << endl;
    logfile.close();
    mutex.unlock();
}

void Client::acceptAlarm()
{
    QPushButton* acceptButton = (QPushButton*)sender();
    Alarm* tWindow = (Alarm*)acceptButton->userData(Qt::UserRole);
    tWindow->close();

    player->stop();
    log(QString("accept clicked: accepted|%1|%2\n").arg(settings->nameLineEdit->text()).arg(settings->locationLineEdit->text()));
    inDataStream.startTransaction();
    inDataStream << QString("accepted|%1|%2").arg(settings->nameLineEdit->text()).arg(settings->locationLineEdit->text());
    if (!inDataStream.commitTransaction()){
        log("write error occurred within commit transaction method!");
        return;
    }

    hasAcceptClicked = true;

}

void Client::readAlarm()
{
    ///return;
    while(tcpSocket->bytesAvailable()){

        //inDataStream.startTransaction();

        QString nextAlarm;
        //inDataStream >> nextAlarm;

        //if (!inDataStream.commitTransaction()){
        //    log("read error occurred within commit transaction method!");
        //    return;
        //}

        QByteArray ba = tcpSocket->readAll();
        nextAlarm = QString::fromStdString(ba.toStdString());

        qDebug() << nextAlarm;

        if(nextAlarm == "pong"){
            if(settings->sleep_avoider_should_be_log){
                log("pong recvd.");
            }


            return;
        }


        if(nextAlarm.compare("This message comes from Alarm Server.") == 0){

            /*
             * Bu client programi, kerneldebuggerserver'a ilk baglandiginda bu mesaji alir.
             * Boylece burada ilklendirme islemlerini yapariz.
            */

            log(" connected to the server\n");
            const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
            for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
                if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
                     log(QString("my ip addresses of network interfaces: %1").arg(address.toString()));
            }

            //sleep avoider messages are sending periodically.
            if(!timerForPingPong)
                timerForPingPong = new QTimer(this);
            else{
                timerForPingPong->stop();
                delete timerForPingPong;
                timerForPingPong = new QTimer(this);
            }
            connect(timerForPingPong, &QTimer::timeout, this, [=](){
                if(settings->sleep_avoider_should_be_log){
                    log("sleep avoider message is sent to the server...");
                }
                inDataStream.startTransaction();
                inDataStream << QString("ping");
                inDataStream.commitTransaction();
            }
                , Qt::QueuedConnection);//separate thread
            timerForPingPong->start(settings->sleep_avoider_period*1000);
        }
        else //if(nextAlarm.compare("This message comes from Alarm Server.") != 0)
        {
            log(QString(" msg recvd: %1\n").arg(nextAlarm));

            QString messageType;
            int pipeLocation = nextAlarm.indexOf("|");
            if(pipeLocation > 0){
                messageType = nextAlarm.mid(0, pipeLocation);
            }
            nextAlarm = nextAlarm.mid(pipeLocation+1);

            QString fromName;
            pipeLocation = nextAlarm.indexOf("|");
            if(pipeLocation > 0){
                fromName = nextAlarm.mid(0, pipeLocation);
            }
            nextAlarm = nextAlarm.mid(pipeLocation+1);

            //the alarm comes from me? if so, don't play alarm sound.
            log(QString("message comes from: %1").arg(fromName));
            bool messageFromMe = false;
            if(fromName.compare(QString("%1_%2").arg(settings->nameLineEdit->text()).arg(settings->locationLineEdit->text())) == 0){
                messageFromMe = true;
            }

            qDebug() << "messageType:" << messageType;

            if(true) {//messageType.compare("alarm") == 0){

                /*if(receivedAlarmWindow){
                    // we don't support multiple alarms at the same time for now.

                    //receivedAlarmWindow->hide();
                    receivedAlarmWindow->deleteLater();
                    receivedAlarmWindow = nullptr;
                }*/

                if(!receivedAlarmWindow) {
                    receivedAlarmWindow = new Alarm;
                    receivedAlarmWindow->setType(ALARM_TYPE);
                    receivedAlarmWindow->setWindowFlags(receivedAlarmWindow->windowFlags() & ~Qt::WindowContextHelpButtonHint);
                    receivedAlarmWindow->setWindowFlags(receivedAlarmWindow->windowFlags() | Qt::WindowStaysOnTopHint);
                    receivedAlarmWindow->setAttribute(Qt::WA_DeleteOnClose, false);
                    //receivedAlarmWindow->setGeometry(300, 300, 500, 500);

                    QPixmap pixmap( 32, 32 );
                    pixmap.fill( Qt::transparent );
                    receivedAlarmWindow->setWindowIcon(QIcon(pixmap));

                    QVBoxLayout* layout = new QVBoxLayout;
                    tb = new QTextBrowser();
                    //tb->setHtml(QString("%1 <BR/> <img src=images/panicbutton.png> </img>").arg(nextAlarm));
                    //tb->setFixedHeight(500);
                    tb->setFixedWidth(500);
                    tb->setFixedHeight(250);//(tb->document()->size().toSize().height()+3);
                    layout->addWidget(tb);

                    QPushButton* muteButton = new QPushButton(tr("Mute"));
                    muteButton->setObjectName("muteButton");
                    muteButton->setEnabled(true);
                    layout->addWidget(muteButton);
                    connect(muteButton, &QAbstractButton::clicked, this, [=](){
                        player->stop();
                    });

                    receivedAlarmWindow->setWindowTitle("");//"Alarm");

                    receivedAlarmWindow->setLayout(layout);

                    receivedAlarmWindow->setObjectsToDelete(layout, tb, muteButton);

                    receivedAlarmWindow->show();
                }

                tb->setHtml(QString("%1").arg(tb->toHtml() + nextAlarm));

                player->setMedia(QUrl::fromLocalFile("alarm.mp3"));
                player->setVolume(100);
                player->play();

            }

        }
    }
    //statusLabel->setText(currentAlarm);
    statusLabel->setText(tr("Connected."));



    settings->getAlarmButton->setEnabled(true);
}

void Client::displayError(QAbstractSocket::SocketError socketError)
{

    statusLabel->setText("Not connected!");
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        log("remote host closed.");
        break;
    case QAbstractSocket::HostNotFoundError:
        /*QMessageBox::information(this, tr("Alarm Client"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));*/
        log("host not found.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        /*QMessageBox::information(this, tr("Alarm Client"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the Alarm server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."))*/;
        log("connection refused.");
        break;
    default:
        /*QMessageBox::information(this, tr("Alarm Client"),
                                 tr("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()))*/;
        log(QString("following error occurred: %1").arg(tcpSocket->errorString()));
    }

    settings->getAlarmButton->setEnabled(true);

    if(!closingTcpSocket){
        tryToReconnect();
        //connectToServer(settings->hostCombo->currentText(), settings->portLineEdit->text().toInt());
    }
}

void Client::disconnected()
{
    if(closing)
        return;
    timerForPingPong->stop();

    statusLabel->setText("Not connected!");
    log("The server connection is disconnected, and will try to reconnect...");

    if(!closingTcpSocket){
        tryToReconnect();
        //connectToServer(settings->hostCombo->currentText(), settings->portLineEdit->text().toInt());
    }
}

void Client::tryToReconnect()
{
    log("trying to reconnect within 1000 ms.");
    //try reconnecting continuously with 1000 millisecond intervals
    if(timerForReconnecting){
        timerForReconnecting->stop();
        timerForReconnecting->deleteLater();
        timerForReconnecting = nullptr;
    }
    timerForReconnecting = new QTimer(this);
    connect(timerForReconnecting, &QTimer::timeout, this, [=](){
        connectToServer(settings->hostCombo->currentText(), settings->portLineEdit->text().toInt());
    }
        , Qt::QueuedConnection);//separate thread
    timerForReconnecting->setSingleShot(true);
    timerForReconnecting->start(1000);

    hasAlarmClicked = false;
    hasAcceptClicked = false;
}

void Client::enableGetAlarmButton()
{
    /*log("connect and accept buttons are enabled.");
    settings->getAlarmButton->setEnabled((!networkSession || networkSession->isOpen()) &&
                                 !settings->hostCombo->currentText().isEmpty() &&
                                 !settings->portLineEdit->text().isEmpty());*/
    /*acceptButton->setEnabled((!networkSession || networkSession->isOpen()) &&
                                 !settings->hostCombo->currentText().isEmpty() &&
                                 !settings->portLineEdit->text().isEmpty());*/

}

void Client::sessionOpened()
{
    log("session opened.");
    // Save the used configuration
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

    enableGetAlarmButton();
}

void Client::sendAlarm()
{

    //in << QString("alarm|John Taylor");

    QByteArray block;
    QDataStream ds(&block, QIODevice::WriteOnly);
    ds.setVersion(QDataStream::Qt_4_0);
    QString alarmString = QString("alarm|%1|%2").arg(settings->nameLineEdit->text(), settings->locationLineEdit->text());
    ds << alarmString;
    tcpSocket->write(block);

    qDebug() << "here" << QString("alarm|%1|%2").arg(settings->nameLineEdit->text(), settings->locationLineEdit->text());
    log(QString("alarm sent: %1").arg(alarmString));


    hasAlarmClicked = true;
}

void Client::changeEvent(QEvent *e)
{

    switch (e->type())
    {
        case QEvent::WindowStateChange:
            {
                if (this->windowState() & Qt::WindowMinimized)
                {
                    /*if (Preferences::instance().minimizeToTray())
                    {
                        QTimer::singleShot(250, this, SLOT(hide()));
                    }*/
                }

                break;
            }
        default:
            break;
    }

    QDialog::changeEvent(e);
}

void Client::closeEvent(QCloseEvent *event)
{
    if(closing)
    {
        log("app closed.");
        event->accept();
    }
    else
    {
        log("app sent to tray bar.");
        this->hide();
        event->ignore();
    }
}

