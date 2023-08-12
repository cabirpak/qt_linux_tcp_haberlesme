
#include <QApplication>
#include "client.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QGuiApplication::setApplicationDisplayName(Client::tr("KernelDebugger Client"));

    QFile file("stylesheet.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());

    qApp->setStyleSheet(styleSheet);
    file.close();

    Client client;
    client.setObjectName("panicWidget");
    client.show();
    return app.exec();
}
