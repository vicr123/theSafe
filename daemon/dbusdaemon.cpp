#include "dbusdaemon.h"
#include "thesafe_adaptor.h"

DBusDaemon::DBusDaemon(QObject *parent) : QObject(parent)
{
    new ThesafeAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/org/thesuite/thesafe", this);

    scheduler = new Scheduler;
}

QDBusObjectPath DBusDaemon::startBackup(int strategyNumber) {
    if (getCurrentRunningBackup(strategyNumber) == QDBusObjectPath("/")) {
        QDBusObjectPath returnPath("/org/thesuite/thesafe/jobs/" + QString::number(backupObjects.count()));
        BackupObject* backup = new BackupObject(returnPath);
        backupObjects.append(backup);
        connect(backup, SIGNAL(backupError(QString)), scheduler, SLOT(reload()));
        connect(backup, SIGNAL(backupComplete()), scheduler, SLOT(reload()));
        backup->start(strategyNumber);

        emit newJob(returnPath);
        return returnPath;
    } else {
        //Backup is already running
        return QDBusObjectPath("/");
    }
}

QDBusObjectPath DBusDaemon::getCurrentRunningBackup(int strategyNumber) {
    for (BackupObject* b : backupObjects) {
        if (b->strategy() == strategyNumber && b->running()) {
            return b->path();
        }
    }

    return QDBusObjectPath("/");
}

QList<QDBusObjectPath> DBusDaemon::jobs() {
    QList<QDBusObjectPath> jobs;
    for (BackupObject* b : backupObjects) {
        jobs.append(b->path());
    }
    return jobs;
}

void DBusDaemon::reloadSchedule() {
    scheduler->reload();
}
