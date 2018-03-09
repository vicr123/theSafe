#ifndef FOLDERSELECTMODEL_H
#define FOLDERSELECTMODEL_H

#include <QFileSystemModel>

class FolderSelectModel : public QFileSystemModel
{
        Q_OBJECT
    public:
        explicit FolderSelectModel(QObject *parent = nullptr);

        QVariant data(const QModelIndex &index, int role) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role);

        void readDescriptor(QString descriptor);
        QString descriptor();
    signals:

    public slots:

    private:
        QStringList selectedPaths;
};

#endif // FOLDERSELECTMODEL_H
