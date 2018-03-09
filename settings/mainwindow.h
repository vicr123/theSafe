#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QListView>
#include <QStackedWidget>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusArgument>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QCommandLinkButton>
#include <QTreeView>
#include <QLabel>
#include <QFileDialog>
#include <QEventLoop>
#include <QMenu>
#include <QToolButton>
#include <QProgressBar>
#include <ttoast.h>
#include <QDateTime>
#include <QMessageBox>
#include <QInputDialog>
#include <QSpinBox>
#include <QComboBox>
#include <QRadioButton>
#include "folderselectmodel.h"
#include "strategymodel.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    private slots:
        void on_newStrategyButton_clicked();

        void strategyChanged(QModelIndex current, QModelIndex previous);

        void on_backButton_clicked();

        void on_setBackupDriveButton_clicked();

        void set(QString key, QVariant value);

        QVariant get(QString key, QVariant defaultValue = QVariant());

        void on_backupDriveFolder_textChanged(const QString &arg1);

        void on_backupDriveDevices_currentRowChanged(int currentRow);

        void on_mainStack_currentChanged(int arg1);

        void on_setBackupFoldersButton_clicked();

        void on_backButton_2_clicked();

        void on_addBackupFolderButton_clicked();

        void on_backupFolders_customContextMenuRequested(const QPoint &pos);

        void on_backButton_3_clicked();

        void on_setExcludeFoldersButton_clicked();

        void on_addExcludeFolderButton_clicked();

        void on_excludeFolders_customContextMenuRequested(const QPoint &pos);

        void on_addExcludeFileButton_clicked();

        void on_backButton_4_clicked();

        void on_setScheduleButton_clicked();

        void on_runBackupButton_clicked();

        void addJob(QDBusObjectPath p);

        void reloadBackupMonitor();

        void reloadMainStack();

        void percentageUpdate(int strategy, int percentage);

        void on_cancelBackupButton_clicked();

        void on_strategyList_customContextMenuRequested(const QPoint &pos);

        void on_renameStrategyButton_clicked();

        void on_scheduleNumber_valueChanged(int arg1);

        void on_scheduleUnit_currentIndexChanged(int index);

        void on_manualBackup_toggled(bool checked);

        void on_scheduledBackup_toggled(bool checked);

    private:
        Ui::MainWindow *ui;

        StrategyModel* strategies;
        FolderSelectModel* fsModel;
        QList<QDBusObjectPath> paths;
};

#endif // MAINWINDOW_H
