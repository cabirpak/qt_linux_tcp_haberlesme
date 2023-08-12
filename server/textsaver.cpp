#include "textsaver.h"

#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

TextSaver::TextSaver(QString _filename, QWidget *parent) : QWidget(parent)
  , filename(_filename)
{
    QVBoxLayout *vbl = new QVBoxLayout(this);
    setLayout(vbl);
    te = new QTextEdit;
    vbl->addWidget(te);

    QPushButton *save = new QPushButton("Save");
    vbl->addWidget(save);

    QFile f(filename);
    if(!f.open(QIODevice::ReadOnly)){
        f.close();
        QMessageBox::information(this, QString("%1 not opened! It will created at first save.").arg(filename), QString("%1 not opened! It will created at first save.").arg(filename));
        QFile f2(filename);
        if(!f2.open(QIODevice::WriteOnly)){
            QMessageBox::information(this, QString("%1 not created!").arg(filename), QString("%1 not created!").arg(filename));
            return;
        }
        f2.write("Test message", QString("Test message").length());
        f2.close();


        if(!f.open(QIODevice::ReadOnly)){
            QMessageBox::information(this, QString("%1 not opened!").arg(filename), QString("%1 not opened!").arg(filename));
            return;
        }
    }

    char alarmtext[1000];
    f.read(alarmtext, 999);
    te->setPlainText(alarmtext);
    f.close();

    connect(save, &QPushButton::clicked, this, [=](){
        QFile f(filename);
        if(!f.open(QIODevice::WriteOnly)){
            QMessageBox::information(this, QString("%1 cannot be opened!").arg(filename), QString("%1 cannot be opened, check the user rights.").arg(filename));
            return;
        }
        f.write(te->toPlainText().toStdString().c_str(), te->toPlainText().length());
        f.close();
    });
}

QString TextSaver::getText()
{
    return te->toPlainText();
    /*QFile f(filename);
    if(!f.open(QIODevice::ReadOnly)){
        f.close();
        return "";
    }
    char alarmtext[1000];
    f.read(alarmtext, 999);
    alarmtext[strlen(alarmtext)-2] = 0;
    return QString::fromLocal8Bit(alarmtext);*/
}
