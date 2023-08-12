#ifndef ALARM_H
#define ALARM_H

#include <QCloseEvent>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

#define ALARM_TYPE 0
#define ACCEPT_TYPE 1

class Alarm : public QWidget
{
    Q_OBJECT
public:
    Alarm();
    ~Alarm();
    void setType(int _type){
        type = _type;
    }
    void setObjectsToDelete(QVBoxLayout* _layout,
        QTextBrowser* _tb,
        QPushButton* _muteBtn = nullptr)
    {
        layout = _layout;
        tb = _tb;
        muteBtn = _muteBtn;
    }

protected slots:
    void closeEvent(QCloseEvent *event);

private:
    int type;
    QVBoxLayout* layout;
    QTextBrowser* tb;
    QPushButton* muteBtn;
};

#endif // ALARM_H
