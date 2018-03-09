#include "strategymodel.h"

StrategyModel::StrategyModel(QObject *parent)
    : QAbstractListModel(parent)
{
    settings = new QSettings();
}

int StrategyModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }

    return settings->value("metadata/strategyCount", 0).toInt();
}

int StrategyModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }

    return 1;
}

QVariant StrategyModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    int planNumber = index.row();
    settings->beginGroup("strategy" + QString::number(planNumber));

    SettingsLocker locker(settings);

    if (settings->contains("name")) {
        if (role == Qt::DisplayRole) {
            return settings->value("name").toString();
        } else if (role == Qt::UserRole) {
            Strategy s;
            s.name = settings->value("name").toString();
            return QVariant::fromValue(s);
        }
    }
    return QVariant();
}

void StrategyModel::addNewStrategy() {
    int planNumber = settings->value("metadata/strategyCount", 0).toInt();
    settings->setValue("metadata/strategyCount", planNumber + 1);

    settings->beginGroup("strategy" + QString::number(planNumber));
    settings->setValue("name", "New Strategy");
    settings->setValue("manual", true);
    settings->endGroup();

    emit dataChanged(index(0, 0), index(rowCount(), 0));
}

void StrategyModel::removeStrategy(int strategy) {
    int numberOfPlans = settings->value("metadata/strategyCount", 0).toInt();
    for (int i = strategy; i < numberOfPlans - 1; i++) {
        //For each strategy, move strategy + 1's settings into the current one
        settings->beginGroup("strategy" + QString::number(strategy + 1));
        QStringList settingsToMove = settings->allKeys();
        settings->endGroup();

        settings->beginGroup("strategy" + QString::number(strategy));
        QStringList settingsToDelete = settings->allKeys();
        for (QString key : settingsToDelete) {
            settings->remove(key);
        }
        settings->endGroup();

        for (QString setting : settingsToMove) {
            settings->beginGroup("strategy" + QString::number(strategy + 1));
            QVariant value = settings->value(setting);
            settings->endGroup();
            settings->beginGroup("strategy" + QString::number(strategy));
            settings->setValue(setting, value);
            settings->endGroup();
        }
    }

    //Now delete all of the last strategy's keys
    settings->beginGroup("strategy" + QString::number(numberOfPlans - 1));
    QStringList settingsToDelete = settings->allKeys();
    for (QString key : settingsToDelete) {
        settings->remove(key);
    }
    settings->endGroup();

    //Decrement the strategy count
    settings->setValue("metadata/strategyCount", numberOfPlans - 1);

    //Update data
    emit dataChanged(index(0, 0), index(rowCount(), 0));
}

void StrategyModel::set(int strategy, QString key, QVariant value) {
    settings->beginGroup("strategy" + QString::number(strategy));
    settings->setValue(key, value);
    settings->endGroup();
}

QVariant StrategyModel::get(int strategy, QString key, QVariant defaultValue) {
    settings->beginGroup("strategy" + QString::number(strategy));
    QVariant v = settings->value(key, defaultValue);
    settings->endGroup();
    return v;
}

void StrategyModel::sync() {
    settings->sync();
    emit dataChanged(index(0, 0), index(rowCount(), 0));
}

SettingsLocker::SettingsLocker(QSettings *s) {
    this->s = s;
}

SettingsLocker::~SettingsLocker() {
    s->endGroup();
}
