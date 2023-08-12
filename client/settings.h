#ifndef SETTINGS_H
#define SETTINGS_H

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

#define ALARM_CLIENT_VERSION "v1.09"

class Settings : public QDialog
{
    Q_OBJECT
public:
    Settings(QWidget *parent = Q_NULLPTR);
private slots:
    void enableGetAlarmButton();

public:

    QPushButton *getAlarmButton;
    QComboBox *hostCombo;
    QLineEdit *portLineEdit;
    QLineEdit *nameLineEdit;
    QLineEdit *locationLineEdit;
    QPushButton *nameSave;
    int sleep_avoider_period;
    bool sleep_avoider_should_be_log;
    bool should_log_events;

signals:
    void connectToServer(QString host, int port);

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // SETTINGS_H
