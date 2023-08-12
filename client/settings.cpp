#include "settings.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include <QtNetwork>
#include <QGuiApplication>
#include <QMessageBox>
#include <QCloseEvent>

#include <iostream>
#include <fstream>
using namespace std;

Settings::Settings(QWidget *parent):
      QDialog(parent)
    , hostCombo(new QComboBox)
    , portLineEdit(new QLineEdit)
    , nameLineEdit(new QLineEdit)
    , locationLineEdit(new QLineEdit)
    , nameSave(new QPushButton("Save configuration"))
    , getAlarmButton(new QPushButton(tr("Connect to server")))
{
    QSettings sett("settings.ini",
                 QSettings::IniFormat);
    /*sett.beginGroup("config");
    const QStringList childKeys = sett.childKeys();
    foreach (const QString &childKey, childKeys)
        QMessageBox::information(this,"", sett.value(childKey).toString());
        //qDebug() << sett.value(childKey);
    sett.endGroup();*/


    nameLineEdit->setText("Write your name here...");
    locationLineEdit->setText("Write your location here...");

    hostCombo->setEditable(true);
    // find out name of this machine
    QString name = QHostInfo::localHostName();
    if (!name.isEmpty()) {
        hostCombo->addItem(name);
        QString domain = QHostInfo::localDomainName();
        if (!domain.isEmpty())
            hostCombo->addItem(name + QChar('.') + domain);
    }
    if (name != QLatin1String("localhost"))
        hostCombo->addItem(QString("localhost"));
    // find out IP addresses of this machine
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // add non-localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (!ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }
    // add localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }
    hostCombo->setCurrentText(sett.value("config/host").toString());

    portLineEdit->setValidator(new QIntValidator(1, 65535, this));

    QLabel *hostLabel = new QLabel(tr("&Server name:"));
    hostLabel->setBuddy(hostCombo);
    QLabel *portLabel = new QLabel(tr("S&erver port:"));
    portLabel->setBuddy(portLineEdit);

    portLineEdit->setText(sett.value("config/port").toString());

    nameLineEdit->setText(sett.value("config/name").toString());
    locationLineEdit->setText(sett.value("config/location").toString());

    sleep_avoider_should_be_log = sett.value("config/sleep_avoider_should_be_log", false).toBool();
    sleep_avoider_period = sett.value("config/sleep_avoider_period", 60).toInt();
    should_log_events = sett.value("config/should_log_events", true).toBool();

    //QDialogButtonBox *buttonBox = new QDialogButtonBox;
    //buttonBox->addButton(getAlarmButton, QDialogButtonBox::ActionRole);


    QLabel *nameLabel = new QLabel(tr("Name:"));
    QLabel *locationLabel = new QLabel(tr("Location:"));

    connect(hostCombo, &QComboBox::editTextChanged,
            this, &Settings::enableGetAlarmButton);
    connect(portLineEdit, &QLineEdit::textChanged,
            this, &Settings::enableGetAlarmButton);
    connect(getAlarmButton, &QAbstractButton::clicked,
            this, [=](){

        emit connectToServer(hostCombo->currentText(),
                                   portLineEdit->text().toInt());
    });


    getAlarmButton->setDefault(true);
    getAlarmButton->setEnabled(false);

    QGridLayout *mainLayout = Q_NULLPTR;
    QGroupBox *groupBox = new QGroupBox(QGuiApplication::applicationDisplayName());
    mainLayout = new QGridLayout(groupBox);

    mainLayout->addWidget(hostLabel, 0, 0);
    mainLayout->addWidget(hostCombo, 0, 1);
    mainLayout->addWidget(portLabel, 1, 0);
    mainLayout->addWidget(portLineEdit, 1, 1);
    mainLayout->addWidget(nameLabel, 2, 0);
    mainLayout->addWidget(nameLineEdit, 2, 1);
    mainLayout->addWidget(locationLabel, 3, 0);
    mainLayout->addWidget(locationLineEdit, 3, 1);
    mainLayout->addWidget(nameSave, 4, 0);
    mainLayout->addWidget(getAlarmButton, 5, 0);
    mainLayout->addWidget(new QLabel(ALARM_CLIENT_VERSION), 6, 0);


    /*hostCombo->setCurrentText("127.0.0.1");
    portLineEdit->setText("8888");
*/

    portLineEdit->setFocus();

    connect(nameSave, &QPushButton::clicked, this, [=](){

        QSettings sett("settings.ini",
                     QSettings::IniFormat);
        sett.setValue("config/host", hostCombo->currentText());
        sett.setValue("config/port", portLineEdit->text());
        sett.setValue("config/name", nameLineEdit->text());
        sett.setValue("config/location", locationLineEdit->text());

    });

    /*if(QFile::exists("name.txt")){
        ifstream nameFile;
        nameFile.open("name.txt", std::ios::in);
        string sName;
        nameFile >> sName;
        nameLineEdit->setText(QString::fromStdString(sName));
    }*/

    getAlarmButton->setEnabled(false);

    setLayout(mainLayout);
}

void Settings::enableGetAlarmButton()
{
    getAlarmButton->setEnabled(true);
}

void Settings::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
}

