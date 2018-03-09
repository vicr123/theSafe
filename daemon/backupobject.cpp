#include "backupobject.h"
#include "job_adaptor.h"

BackupObject::BackupObject(QDBusObjectPath path, QObject *parent) : QObject(parent)
{
    new JobAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(path.path(), this);
    p = path;

    noUserInteraction.insert("auth.no_user_interaction", true);
}

int BackupObject::state() {
    return bupStage;
}

int BackupObject::progress() {
    return percentage;
}

int BackupObject::strategy() {
    return strat;
}

void BackupObject::start(int strategy) {
    //Start the backup
    this->strat = strategy;
    if (started) return;
    qDebug() << "Starting backup of strategy" << strategy;
    settings.beginGroup("strategy" + QString::number(strategy));

    if (settings.contains("lastError")) {
        settings.remove("lastError");
        settings.remove("lastErrorType");
    }

    bupProcess = new QProcess();

    QString backupLocation = settings.value("backupDrive").toString();

    bupStage = Starting;
    emit changed();

    //Check if the drive is mounted with udisks
    QDBusMessage resolveDevice = QDBusMessage::createMethodCall("org.freedesktop.UDisks2", "/org/freedesktop/UDisks2/Manager", "org.freedesktop.UDisks2.Manager", "ResolveDevice");
    {
        QList<QVariant> resolveDeviceArgs;
        QVariantMap devspec;
        devspec.insert("uuid", backupLocation);
        resolveDeviceArgs.append(devspec);
        resolveDeviceArgs.append(noUserInteraction);
        resolveDevice.setArguments(resolveDeviceArgs);
    }
    QDBusPendingCallWatcher* w = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(resolveDevice));
    connect(w, &QDBusPendingCallWatcher::finished, [=] {
        w->deleteLater();
        QList<QDBusObjectPath> devices;
        w->reply().arguments().first().value<QDBusArgument>() >> devices;

        if (devices.count() == 0) {
            bupStage = Error;
            emit backupError("Backup Device not found");
            settings.setValue("lastError", "Backup Device not found");
            settings.setValue("lastErrorType", 1);
            settings.setValue("lastBackup", QDateTime::currentDateTimeUtc());
            settings.sync();
            settings.endGroup();
            emit changed();
            return;
        }

        //Check if the device needs to be mounted
        QDBusMessage getMountPoints = QDBusMessage::createMethodCall("org.freedesktop.UDisks2", devices.first().path(), "org.freedesktop.DBus.Properties", "Get");
        getMountPoints.setArguments(QVariantList() << "org.freedesktop.UDisks2.Filesystem" << "MountPoints");

        QDBusPendingCallWatcher* w = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(getMountPoints));
        connect(w, &QDBusPendingCallWatcher::finished, [=] {
            w->deleteLater();

            QList<QByteArray> mountPoints;
            w->reply().arguments().first().value<QDBusVariant>().variant().value<QDBusArgument>() >> mountPoints;

            if (mountPoints.count() == 0) {
                //Mount the drive
                QDBusMessage mountDrive = QDBusMessage::createMethodCall("org.freedesktop.UDisks2", devices.first().path(), "org.freedesktop.UDisks2.Filesystem", "Mount");
                mountDrive.setArguments(QVariantList() << noUserInteraction);

                QDBusPendingCallWatcher* w = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(mountDrive));
                connect(w, &QDBusPendingCallWatcher::finished, [=] {
                    w->deleteLater();
                    init(w->reply().arguments().first().toString());
                });
            } else {
                //Initialize the backup
                init(mountPoints.first());
            }
        });
    });
}

void BackupObject::init(QString path) {
    //Initialize the backup
    QString folder = settings.value("backupDriveFolder").toString();
    QString bupPath = path + "/" + folder;
    QProcessEnvironment env = bupProcess->processEnvironment();
    env.insert("BUP_DIR", bupPath);
    env.insert("BUP_FORCE_TTY", "3");
    bupProcess->setProcessEnvironment(env);

    bupProcess->start("bup ls");
    QMetaObject::Connection* c = new QMetaObject::Connection;
    *c = PROCESS_FINISHED(bupProcess, {
        disconnect(*c);
        delete c;

        if (exitCode == 0) {
            //bup repo exists
            index();
        } else {
            //bup repo does not exist
            //Create the bup repo
            bupProcess->start("bup init");
            QMetaObject::Connection* c = new QMetaObject::Connection;
            *c = PROCESS_FINISHED(bupProcess, {
                disconnect(*c);
                delete c;
                index();
            });
        }
    });
}

void BackupObject::index() {
    QStringList excludeFolders = settings.value("excludeFolders").toString().split(",");
    QStringList exclusionStringList;
    for (QString exclude : excludeFolders) {
        exclusionStringList.append("--exclude=\"" + exclude + "\"");
    }
    exclusionString = exclusionStringList.join(" ");

    QStringList sourceFolders = settings.value("backupFolders").toString().split(",");
    QStringList sourcesStringList;
    for (QString source : sourceFolders) {
        sourcesStringList.append("\"" + source + "\"");
    }
    sourcesString = sourcesStringList.join(" ");

    bupStage = Indexing;
    emit changed();
    bupProcess->start("bup index " + exclusionString + " " + sourcesString);

    QMetaObject::Connection* c = new QMetaObject::Connection;
    *c = PROCESS_FINISHED(bupProcess, {
        disconnect(*c);
        delete c;

        if (exitCode == 0) {
            save();
        } else {
            if (exitCode == 15) {
                //Backup Cancelled
                QString error = "Backup Cancelled";
                emit backupError(error);
                settings.setValue("lastError", error);
                settings.setValue("lastErrorType", 0);
                settings.setValue("lastBackup", QDateTime::currentDateTimeUtc());
                settings.sync();
                bupStage = Error;
                emit changed();
                return;
            }

            QString error = "Nonzero return value while indexing: bup-index returned " + QString::number(exitCode);
            emit backupError(error);
            settings.setValue("lastError", error);
            settings.setValue("lastErrorType", -1);
            settings.setValue("lastBackup", QDateTime::currentDateTimeUtc());
            settings.sync();
            bupStage = Error;
            emit changed();
        }
    });
}

void BackupObject::save() {
    //Save all objects
    bupStage = BackingUp;
    emit changed();

    int compressLevel = settings.value("compression", 9).toInt();

    bupProcess->start("bup save --name=thesafe --compress=" + QString::number(compressLevel) + " " + sourcesString);
    //bupProcess->setProcessChannelMode(QProcess::MergedChannels);
    QMetaObject::Connection* c = new QMetaObject::Connection;
    *c = PROCESS_FINISHED(bupProcess, {
        disconnect(*c);
        delete c;

        if (exitCode == 0) {
            bupStage = Finished;
            settings.setValue("lastBackup", QDateTime::currentDateTimeUtc());
            settings.sync();
            emit backupComplete();
            emit changed();
        } else {
            if (exitCode == 15) {
                //Backup Cancelled
                QString error = "Backup Cancelled";
                emit backupError(error);
                settings.setValue("lastError", error);
                settings.setValue("lastErrorType", 0);
                settings.setValue("lastBackup", QDateTime::currentDateTimeUtc());
                settings.sync();
                bupStage = Error;
                emit changed();
                return;
            }

            QString error = "Nonzero return value while saving: bup-save returned " + QString::number(exitCode);
            emit backupError(error);
            settings.setValue("lastError", error);
            settings.setValue("lastErrorType", -1);
            settings.setValue("lastBackup", QDateTime::currentDateTimeUtc());
            settings.sync();
            bupStage = Error;
            emit changed();
        }
    });
    connect(bupProcess, &QProcess::readyReadStandardError, [=] {
        QString read = bupProcess->readAllStandardError();
        if (read.startsWith("Saving:")) {
            QString percentagePart = read.mid(8);
            percentagePart = percentagePart.left(percentagePart.indexOf("%"));
            percentage = (percentagePart.toFloat() * 100);
            emit progressUpdate(strat, percentage);
        }
    });
}

QDBusObjectPath BackupObject::path() {
    return p;
}

bool BackupObject::running() {
    if (state() == BackupObject::Starting || state() == BackupObject::Indexing || state() == BackupObject::BackingUp) {
        return true;
    } else {
        return false;
    }
}

void BackupObject::cancel() {
    bupProcess->terminate();
}
