#ifndef DBUSDAEMON_H
#define DBUSDAEMON_H

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusConnection>
#include "backupobject.h"
#include "scheduler.h"

class DBusDaemon : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.thesuite.thesafe")

    public:
        explicit DBusDaemon(QObject *parent = nullptr);

    signals:
        Q_SCRIPTABLE void newJob(QDBusObjectPath job);

    public Q_SLOTS:
        Q_SCRIPTABLE void reloadSchedule();
        Q_SCRIPTABLE QDBusObjectPath startBackup(int strategyNumber);
        Q_SCRIPTABLE QDBusObjectPath getCurrentRunningBackup(int strategyNumber);
        Q_SCRIPTABLE QList<QDBusObjectPath> jobs();

    private:
        QList<BackupObject*> backupObjects;
        Scheduler* scheduler;
};

#endif // DBUSDAEMON_H
