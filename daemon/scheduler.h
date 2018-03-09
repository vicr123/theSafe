#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <QObject>
#include <QTimer>
#include <QSettings>
#include <QDateTime>

class ScheduleObject : public QObject {
        Q_OBJECT
    public:
        ScheduleObject(int strategy);
        ~ScheduleObject();

        QTimer* timer;
        int strategy;

    private:
        QSettings settings;
};

class Scheduler : public QObject
{
        Q_OBJECT
    public:
        explicit Scheduler(QObject *parent = nullptr);

    signals:

    public slots:
        void reload();

    private:
        QList<ScheduleObject*> timers;
        QSettings settings;
};

#endif // SCHEDULER_H
