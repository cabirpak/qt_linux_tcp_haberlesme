#include "alarm.h"

#include <QCloseEvent>
#include <QDebug>

Alarm::Alarm(): type(0)
{
}

Alarm::~Alarm()
{
    qDebug() << "destructor called";
    delete layout;
    delete tb;

    if(type == ACCEPT_TYPE){
        //nothing
    }
    else if(type == ALARM_TYPE){
        delete muteBtn;
    }
}

void Alarm::closeEvent(QCloseEvent *event)
{
    event->ignore();
    this-> hide();

    //if(type == ACCEPT_TYPE)

    deleteLater();

    //qDebug() << "Alarm::closeEvent: the object deleted from memory";

    /*
        GREAT TODO HERE:
        So many alarm widgets may hidden and not be deleted from the memory. This should be handled as
        adding them to a list, such as a "to be deleted" list.
        Why I don't delete them and only hide them, the reason is that: If I delete the alarm widget while the main
        panic widget is in the tray bar, the application closes itself.

        UPDATE FOR THIS TODO: the deleteLater call avoids memory leaks.
    */
}
