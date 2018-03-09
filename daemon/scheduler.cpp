#include "scheduler.h"
#include "dbusdaemon.h"

extern DBusDaemon* daemon;

Scheduler::Scheduler(QObject *parent) : QObject(parent)
{
    QTimer::singleShot(0, this, SLOT(reload()));
}

void Scheduler::reload() {
    for (ScheduleObject* o : timers) {
        o->deleteLater();
    }
    timers.clear();

    settings.sync();
    for (int i = 0; i < settings.value("metadata/strategyCount", 0).toInt(); i++) {
        settings.beginGroup("strategy" + QString::number(i));
        if (!settings.value("manual").toBool() && settings.contains("lastBackup") && daemon->getCurrentRunningBackup(i).path() == "/") {
            ScheduleObject* o = new ScheduleObject(i);
            timers.append(o);
        }
        settings.endGroup();
    }
}

ScheduleObject::ScheduleObject(int strategy) {
    settings.beginGroup("strategy" + QString::number(strategy));
    QDateTime lastBackup = settings.value("lastBackup").toDateTime();

    QDateTime nextBackup;
    int freq = settings.value("backupFrequency").toInt();
    switch (settings.value("backupUnit").toInt()) {
        case 0: //Minutes
            nextBackup = lastBackup.addSecs(60 * freq);
            break;
        case 1: //Hours
            nextBackup = lastBackup.addSecs(60 * 60 * freq);
            break;
        case 2: //Days
            nextBackup = lastBackup.addDays(freq);
            break;
        case 3: //Weeks
            nextBackup = lastBackup.addDays(7 * freq);
            break;
        case 4: //Months
            nextBackup = lastBackup.addMonths(freq);
            break;
    }

    timer = new QTimer();
    if (nextBackup < QDateTime::currentDateTimeUtc()) {
        //Start the backup now
        daemon->startBackup(strategy);
    } else {
        timer->setInterval(nextBackup.toMSecsSinceEpoch() - QDateTime::currentMSecsSinceEpoch());
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, [=] {
            daemon->startBackup(strategy);
        });
        timer->start();
    }

    settings.endGroup();
}

ScheduleObject::~ScheduleObject() {
    this->timer->deleteLater();
}
