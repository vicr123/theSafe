#include "folderselectmodel.h"

FolderSelectModel::FolderSelectModel(QObject *parent) : QFileSystemModel(parent)
{
    this->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
}

QVariant FolderSelectModel::data(const QModelIndex &index, int role) const {
    if (role == Qt::CheckStateRole) {
        if (selectedPaths.contains(filePath(index))) {
            return Qt::Checked;
        } else {
            return Qt::Unchecked;
        }
    }
    return QFileSystemModel::data(index, role);
}

Qt::ItemFlags FolderSelectModel::flags(const QModelIndex &index) const {
    if (index.column() == 0) {
        return QFileSystemModel::flags(index) | Qt::ItemIsUserCheckable;
    } else {
        return QFileSystemModel::flags(index);
    }
}

bool FolderSelectModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role == Qt::CheckStateRole) {
        QString file = filePath(index);
        if (value == Qt::Checked && !selectedPaths.contains(file)) {
            selectedPaths.append(file);
        } else if (selectedPaths.contains(file)) {
            selectedPaths.removeAll(file);
        }

        fetchMore(index);
        for (int i = 0; i < rowCount(index); i++) {
            setData(this->index(i, 0, index), value, role);
        }
        return true;
    }
    return QFileSystemModel::setData(index, value, role);
}

void FolderSelectModel::readDescriptor(QString descriptor) {

}

QString FolderSelectModel::descriptor() {
    return selectedPaths.join(",");
}
