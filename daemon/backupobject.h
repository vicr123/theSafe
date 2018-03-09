#ifndef BACKUPOBJECT_H
#define BACKUPOBJECT_H

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QProcess>
#include <QSettings>

#define PROCESS_FINISHED(process, function) connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) \
    function\
);

class BackupObject : public QObject
{
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.thesuite.thesafe.job")
        Q_SCRIPTABLE Q_PROPERTY(int State READ state)
        Q_SCRIPTABLE Q_PROPERTY(bool Running READ running)
        Q_SCRIPTABLE Q_PROPERTY(int Strategy READ strategy)

    public:
        enum Stages {
            Error = -1,
            Starting,
            Indexing,
            BackingUp,
            Finished
        };

        explicit BackupObject(QDBusObjectPath path, QObject *parent = nullptr);
        void start(int strategy);
        QDBusObjectPath path();

    signals:
        Q_SCRIPTABLE void backupComplete();
        Q_SCRIPTABLE void backupError(QString description);
        Q_SCRIPTABLE void changed();
        Q_SCRIPTABLE void progressUpdate(int strategy, int progress);

    public Q_SLOTS:
        Q_SCRIPTABLE int state();
        Q_SCRIPTABLE bool running();
        Q_SCRIPTABLE int progress();
        Q_SCRIPTABLE int strategy();
        Q_SCRIPTABLE void cancel();

    private slots:
        void init(QString devicePath);
        void index();
        void save();

    private:
        QProcess* bupProcess;
        Stages bupStage;
        QSettings settings;
        QVariantMap noUserInteraction;
        QString exclusionString;
        QString sourcesString;
        bool started = false;
        int strat;
        int percentage = 0;
        QDBusObjectPath p;
};

#endif // BACKUPOBJECT_H
