#ifndef TEXTSAVER_H
#define TEXTSAVER_H

#include <QTextEdit>
#include<QWidget>
class TextSaver : public QWidget
{
    Q_OBJECT
public:
    explicit TextSaver(QString filename, QWidget *parent = nullptr);
    QString getText();
    QString filename;
signals:

public slots:

private:
    QTextEdit *te;
};

#endif // TEXTSAVER_H
