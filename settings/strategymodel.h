#ifndef STRATEGYMODEL_H
#define STRATEGYMODEL_H

#include <QAbstractItemModel>
#include <QSettings>

struct SettingsLocker {
    SettingsLocker(QSettings* s);
    ~SettingsLocker();

    QSettings* s;
};

struct Strategy {
    QString name;
};
Q_DECLARE_METATYPE(Strategy)

class StrategyModel : public QAbstractListModel
{
        Q_OBJECT

    public:
        explicit StrategyModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    public slots:
        void addNewStrategy();
        void removeStrategy(int strategy);
        void set(int strategy, QString key, QVariant value);
        QVariant get(int strategy, QString key, QVariant defaultValue = QVariant());
        void sync();

    private:
        QSettings* settings;
};

#endif // STRATEGYMODEL_H
