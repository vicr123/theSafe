#include <QCoreApplication>
#include <QSettings>
#include "dbusdaemon.h"

DBusDaemon* daemon;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    a.setOrganizationName("theSuite");
    a.setOrganizationDomain("");
    a.setApplicationName("theSafe");

    QDBusConnection::sessionBus().registerService("org.thesuite.thesafe");

    QSettings settings;

    daemon = new DBusDaemon;

    return a.exec();
}
