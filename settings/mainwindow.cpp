#include "mainwindow.h"
#include "ui_mainwindow.h"

extern QString calculateSize(quint64 size);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    strategies = new StrategyModel();
    ui->strategyList->setModel(strategies);
    connect(ui->strategyList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(strategyChanged(QModelIndex,QModelIndex)));

    ui->cancelBackupButton->setProperty("type", "destructive");

    QMenu* menuButtonMenu = new QMenu();
    menuButtonMenu->addAction(tr("Back up strategies"), [=] {
        QFileDialog* d = new QFileDialog();
        d->setAcceptMode(QFileDialog::AcceptSave);
        d->setFileMode(QFileDialog::AnyFile);
        d->setWindowTitle(tr("Back up Strategies"));
        d->setNameFilter("theSafe Strategy File (*.tsstrat)");

        QEventLoop* loop = new QEventLoop();
        connect(d, SIGNAL(finished(int)), loop, SLOT(quit()));
        connect(d, &QFileDialog::accepted, [=] {
            QSettings settings;
            QFile settingsFile(settings.fileName());
            settingsFile.copy(d->selectedFiles().first());
        });
        connect(d, SIGNAL(finished(int)), loop, SLOT(deleteLater()));
        connect(d, SIGNAL(finished(int)), d, SLOT(deleteLater()));
        d->show();
    });
    ui->menuButton->setMenu(menuButtonMenu);


    QDBusMessage pathsMessage = QDBusMessage::createMethodCall("org.thesuite.thesafe", "/org/thesuite/thesafe", "org.thesuite.thesafe", "jobs");
    QList<QDBusObjectPath> paths;
    QDBusConnection::sessionBus().call(pathsMessage).arguments().first().value<QDBusArgument>() >> paths;
    for (QDBusObjectPath p : paths) {
        addJob(p);
    }
    QDBusConnection::sessionBus().connect("org.thesuite.thesafe", "/org/thesuite/thesafe", "org.thesuite.thesafe", "newJob", this, SLOT(addJob(QDBusObjectPath)));
    reloadBackupMonitor();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_newStrategyButton_clicked()
{
    strategies->addNewStrategy();
}

void MainWindow::strategyChanged(QModelIndex current, QModelIndex previous) {
    if (current.isValid()) {
        ui->mainStack->setCurrentIndex(1);

        Strategy s = current.data(Qt::UserRole).value<Strategy>();
        ui->strategyName->setText(s.name);

        //Subscribe to job updates
        reloadBackupMonitor();
        reloadMainStack();
    } else {
        ui->mainStack->setCurrentIndex(0);
        reloadMainStack();
    }
}

void MainWindow::reloadMainStack() {
    ui->setBackupDriveButton->setDescription(get("backupDriveName", tr("Not Set")).toString());

    QStringList backupFolders = get("backupFolders", QDir::homePath()).toString().split(",", QString::SkipEmptyParts);
    ui->setBackupFoldersButton->setDescription(tr("%n folder(s)", nullptr, backupFolders.count()));

    QStringList excludeFolders = get("excludeFolders").toString().split(",", QString::SkipEmptyParts);
    ui->setExcludeFoldersButton->setDescription(tr("%n item(s)", nullptr, excludeFolders.count()));

    if (get("manual").toBool()) {
        ui->setScheduleButton->setDescription(tr("Manually"));
    } else {
        int freq = get("backupFrequency").toInt();
        switch (get("backupUnit").toInt()) {
            case 0:
                ui->setScheduleButton->setDescription(tr("Every %n minute(s)", nullptr, freq));
                break;
            case 1:
                ui->setScheduleButton->setDescription(tr("Every %n hour(s)", nullptr, freq));
                break;
            case 2:
                ui->setScheduleButton->setDescription(tr("Every %n day(s)", nullptr, freq));
                break;
            case 3:
                ui->setScheduleButton->setDescription(tr("Every %n week(s)", nullptr, freq));
                break;
            case 4:
                ui->setScheduleButton->setDescription(tr("Every %n month(s)", nullptr, freq));
                break;
        }
    }
}

void MainWindow::on_backButton_clicked()
{
    ui->mainStack->setCurrentIndex(1);
}

void MainWindow::on_setBackupDriveButton_clicked()
{
    ui->mainStack->setCurrentIndex(2);

    //Retrieve available storage devices
    QDBusMessage blockDevicesMessage = QDBusMessage::createMethodCall("org.freedesktop.UDisks2", "/org/freedesktop/UDisks2/Manager", "org.freedesktop.UDisks2.Manager", "GetBlockDevices");
    blockDevicesMessage.setArguments(QList<QVariant>() << QVariantMap());
    QDBusMessage blockDevicesReply = QDBusConnection::systemBus().call(blockDevicesMessage);
    QList<QDBusObjectPath> blockDevices;
    blockDevicesReply.arguments().first().value<QDBusArgument>() >> blockDevices;

    ui->backupDriveDevices->clear();
    for (QDBusObjectPath devicePath : blockDevices) {
        QDBusInterface device("org.freedesktop.UDisks2", devicePath.path(), "org.freedesktop.UDisks2.Block", QDBusConnection::systemBus());
        QDBusObjectPath drivePath = device.property("Drive").value<QDBusObjectPath>();
        QDBusInterface drive("org.freedesktop.UDisks2", drivePath.path(), "org.freedesktop.UDisks2.Drive", QDBusConnection::systemBus());

        if (device.property("IdUsage").toString() == "filesystem" && drive.property("Removable").toBool()) {
            QString label = device.property("IdLabel").toString();
            if (label == "") {
                label = tr("%1 Removable Disk").arg(calculateSize(device.property("Size").toLongLong()));
            }
            QListWidgetItem* i = new QListWidgetItem;
            i->setText(label);
            i->setData(Qt::UserRole, device.property("IdUUID"));
            ui->backupDriveDevices->addItem(i);

            if (device.property("IdUUID") == get("backupDrive")) {
                i->setSelected(true);
            }
        }
    }

    ui->backupDriveFolder->setText(get("backupDriveFolder", "thesafe").toString());
}

void MainWindow::set(QString key, QVariant value) {
    strategies->set(ui->strategyList->currentIndex().row(), key, value);
}

QVariant MainWindow::get(QString key, QVariant defaultValue) {
    return strategies->get(ui->strategyList->currentIndex().row(), key, defaultValue);
}

void MainWindow::on_backupDriveFolder_textChanged(const QString &arg1)
{
    set("backupDriveFolder", arg1);
}

void MainWindow::on_backupDriveDevices_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        set("backupDrive", ui->backupDriveDevices->item(currentRow)->data(Qt::UserRole).toString());
        set("backupDriveName", ui->backupDriveDevices->item(currentRow)->text());
        set("backupDriveFolder", ui->backupDriveFolder->text());
        reloadBackupMonitor();
    }
}

void MainWindow::on_mainStack_currentChanged(int arg1)
{
    switch (arg1) {
        case 1: { //Strategy Main Configuration Menu
            reloadMainStack();
            break;
        }
    }
}

void MainWindow::on_setBackupFoldersButton_clicked()
{
    ui->mainStack->setCurrentIndex(3);

    ui->backupFolders->clear();
    QStringList backupFolders = get("backupFolders", QDir::homePath()).toString().split(",", QString::SkipEmptyParts);
    ui->backupFolders->addItems(backupFolders);
}

void MainWindow::on_backButton_2_clicked()
{
    QStringList backupFolders;
    for (int i = 0; i < ui->backupFolders->count(); i++) {
        backupFolders.append(ui->backupFolders->item(i)->text());
    }
    set("backupFolders", backupFolders.join(","));

    ui->mainStack->setCurrentIndex(1);
}

void MainWindow::on_addBackupFolderButton_clicked()
{
    QFileDialog* d = new QFileDialog();
    d->setAcceptMode(QFileDialog::AcceptOpen);
    d->setFileMode(QFileDialog::Directory);
    d->setOption(QFileDialog::ShowDirsOnly, true);
    d->setWindowTitle(tr("Select Backup Folder"));

    QEventLoop* loop = new QEventLoop();
    connect(d, SIGNAL(finished(int)), loop, SLOT(quit()));
    connect(d, &QFileDialog::accepted, [=] {
        for (int i = 0; i < ui->backupFolders->count(); i++) {
            if (ui->backupFolders->item(i)->text() == d->selectedFiles().first()) {
                tToast* t = new tToast();
                t->setTitle(tr("Folder Already Added"));
                t->setText(tr("You've already set this folder to be backed up."));
                connect(t, SIGNAL(dismissed()), t, SLOT(deleteLater()));
                t->show(this);
                return;
            }
        }
        ui->backupFolders->addItems(d->selectedFiles());
    });
    connect(d, SIGNAL(finished(int)), loop, SLOT(deleteLater()));
    connect(d, SIGNAL(finished(int)), d, SLOT(deleteLater()));
    d->show();
}

void MainWindow::on_backupFolders_customContextMenuRequested(const QPoint &pos)
{
    QMenu* m = new QMenu();
    m->addSection(tr("For %n Selected Item(s)", nullptr, ui->backupFolders->selectedItems().count()));
    m->addAction(QIcon::fromTheme("edit-delete"), tr("Remove Items"), [=] {
        qDeleteAll(ui->backupFolders->selectedItems());
    });
    m->exec(ui->backupFolders->mapToGlobal(pos));
}

void MainWindow::on_backButton_3_clicked()
{
    QStringList excludeFolders;
    for (int i = 0; i < ui->excludeFolders->count(); i++) {
        excludeFolders.append(ui->excludeFolders->item(i)->text());
    }
    set("excludeFolders", excludeFolders.join(","));

    ui->mainStack->setCurrentIndex(1);
}

void MainWindow::on_setExcludeFoldersButton_clicked()
{
    ui->mainStack->setCurrentIndex(4);

    ui->excludeFolders->clear();
    QStringList excludeFolders = get("excludeFolders").toString().split(",", QString::SkipEmptyParts);
    ui->excludeFolders->addItems(excludeFolders);
}

void MainWindow::on_addExcludeFolderButton_clicked()
{
    QFileDialog* d = new QFileDialog();
    d->setAcceptMode(QFileDialog::AcceptOpen);
    d->setFileMode(QFileDialog::Directory);
    d->setWindowTitle(tr("Select Exclusion"));

    QEventLoop* loop = new QEventLoop();
    connect(d, SIGNAL(finished(int)), loop, SLOT(quit()));
    connect(d, &QFileDialog::accepted, [=] {
        for (int i = 0; i < ui->excludeFolders->count(); i++) {
            if (ui->excludeFolders->item(i)->text() == d->selectedFiles().first()) {
                tToast* t = new tToast();
                t->setTitle(tr("Folder Already Added"));
                t->setText(tr("You've already set this folder to be excluded from the backup."));
                connect(t, SIGNAL(dismissed()), t, SLOT(deleteLater()));
                t->show(this);
                return;
            }
        }
        ui->excludeFolders->addItems(d->selectedFiles());
    });
    connect(d, SIGNAL(finished(int)), loop, SLOT(deleteLater()));
    connect(d, SIGNAL(finished(int)), d, SLOT(deleteLater()));
    d->show();
}

void MainWindow::on_excludeFolders_customContextMenuRequested(const QPoint &pos)
{
    QMenu* m = new QMenu();
    m->addSection(tr("For %n Selected Item(s)", nullptr, ui->excludeFolders->selectedItems().count()));
    m->addAction(QIcon::fromTheme("edit-delete"), tr("Remove Items"), [=] {
        qDeleteAll(ui->excludeFolders->selectedItems());
    });
    m->exec(ui->excludeFolders->mapToGlobal(pos));
}

void MainWindow::on_addExcludeFileButton_clicked()
{
    QFileDialog* d = new QFileDialog();
    d->setAcceptMode(QFileDialog::AcceptOpen);
    d->setFileMode(QFileDialog::ExistingFile);
    d->setWindowTitle(tr("Select Exclusion"));

    QEventLoop* loop = new QEventLoop();
    connect(d, SIGNAL(finished(int)), loop, SLOT(quit()));
    connect(d, &QFileDialog::accepted, [=] {
        for (int i = 0; i < ui->excludeFolders->count(); i++) {
            if (ui->excludeFolders->item(i)->text() == d->selectedFiles().first()) {
                tToast* t = new tToast();
                t->setTitle(tr("File Already Added"));
                t->setText(tr("You've already set this file to be excluded from the backup."));
                connect(t, SIGNAL(dismissed()), t, SLOT(deleteLater()));
                t->show(this);
                return;
            }
        }
        ui->excludeFolders->addItems(d->selectedFiles());
    });
    connect(d, SIGNAL(finished(int)), loop, SLOT(deleteLater()));
    connect(d, SIGNAL(finished(int)), d, SLOT(deleteLater()));
    d->show();
}

void MainWindow::on_backButton_4_clicked()
{
    ui->mainStack->setCurrentIndex(1);

    //Reload the backup scheduler
    QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall("org.thesuite.thesafe", "/org/thesuite/thesafe", "org.thesuite.thesafe", "reloadSchedule"));

}

void MainWindow::on_setScheduleButton_clicked()
{
    ui->mainStack->setCurrentIndex(5);

    if (get("manual").toBool()) {
        ui->manualBackup->setChecked(true);
    } else {
        ui->scheduleNumber->setValue(get("backupFrequency").toInt());
        ui->scheduleUnit->setCurrentIndex(get("backupUnit").toInt());
        ui->scheduledBackup->setChecked(true);
    }
}

void MainWindow::on_runBackupButton_clicked()
{
    QDBusMessage m = QDBusMessage::createMethodCall("org.thesuite.thesafe", "/org/thesuite/thesafe", "org.thesuite.thesafe", "startBackup");
    m.setArguments(QVariantList() << ui->strategyList->currentIndex().row());
    QDBusConnection::sessionBus().call(m);

    reloadBackupMonitor();
}

void MainWindow::addJob(QDBusObjectPath p) {
    paths.append(p);
    QDBusConnection::sessionBus().connect("org.thesuite.thesafe", p.path(), "org.thesuite.thesafe.job", "changed", this, SLOT(reloadBackupMonitor()));
    QDBusConnection::sessionBus().connect("org.thesuite.thesafe", p.path(), "org.thesuite.thesafe.job", "progressUpdate", this, SLOT(percentageUpdate(int,int)));
}

void MainWindow::reloadBackupMonitor() {
    strategies->sync();
    int current = ui->strategyList->currentIndex().row();

    if (current == -1) {
        ui->currentBackupFrame->setVisible(false);
    } else {
        ui->currentBackupFrame->setVisible(true);
        if (get("backupDrive", "").toString() == "") {
            //Backup Drive is not configured
            ui->backupMasterLabel->setText(tr("Set Up New Strategy"));
            ui->backupStateLabel->setText(tr("To run a backup, you'll need to finish setting up this strategy."));
            ui->cancelBackupButton->setVisible(false);
            ui->runBackupButton->setVisible(true);
            ui->runBackupButton->setEnabled(false);
            ui->backupBar->setVisible(false);
            //ui->backupErrorFrame->setVisible(false);
            ui->backupErrorText->setVisible(false);
            ui->currentBackupFrame->setPalette(QApplication::palette(ui->currentBackupFrame));
        } else {
            for (QDBusObjectPath p : paths) {
                QDBusInterface i("org.thesuite.thesafe", p.path(), "org.thesuite.thesafe.job");
                if (i.property("Running").toBool() && i.property("Strategy") == current) {
                    //Backup is running
                    ui->backupMasterLabel->setText(tr("Backing Up..."));
                    ui->cancelBackupButton->setVisible(true);
                    ui->runBackupButton->setVisible(false);
                    ui->backupBar->setVisible(true);
                    //ui->backupErrorFrame->setVisible(false);
                    ui->backupErrorText->setVisible(false);
                    ui->currentBackupFrame->setPalette(QApplication::palette(ui->currentBackupFrame));

                    int state = i.property("State").toInt();
                    if (state == 1) { //Indexing
                        ui->backupStateLabel->setText(tr("Indexing files for backup"));
                        ui->backupBar->setMaximum(0);
                    } else if (state == 0) { //Starting
                        ui->backupStateLabel->setText(tr("Preparing Backup"));
                        ui->backupBar->setMaximum(0);
                    } else {
                        ui->backupStateLabel->setText(tr("Backing Up Files..."));
                    }
                    return;
                }
            }

            ui->backupMasterLabel->setText(tr("Back Up"));
            ui->cancelBackupButton->setVisible(false);
            ui->runBackupButton->setVisible(true);
            ui->runBackupButton->setEnabled(true);
            ui->backupBar->setVisible(false);

            if (get("lastError", "").toString() == "") {
                //No Backup is running
                //ui->backupErrorFrame->setVisible(false);
                QDateTime lastBackup = get("lastBackup", QDateTime::fromSecsSinceEpoch(0)).toDateTime();
                if (lastBackup == QDateTime::fromSecsSinceEpoch(0)) {
                    if (get("manual").toBool()) {
                        ui->backupStateLabel->setText(tr("To start a backup now, press \"Back Up Now\""));
                    } else {
                        ui->backupStateLabel->setText(tr("To start a backup now, press \"Back Up Now\". <b>Backup scheduling will begin once the first backup is successfully completed.</b>"));
                    }
                } else {
                    ui->backupStateLabel->setText(tr("The last backup finished successfully on %1.").arg(lastBackup.toLocalTime().toString("ddd d MMM yyyy, hh:mm")));
                }
                ui->currentBackupFrame->setPalette(QApplication::palette(ui->currentBackupFrame));
                ui->backupErrorText->setVisible(false);
            } else {
                //Last backup failed
                QPalette p = QApplication::palette(ui->currentBackupFrame);
                p.setColor(QPalette::Window, QColor(150, 0, 0));
                p.setColor(QPalette::WindowText, QColor(231, 231, 231));
                ui->currentBackupFrame->setPalette(p);

                int errorType = get("lastErrorType").toInt();
                if (errorType == 0) { //Cancelled
                    ui->backupStateLabel->setText(tr("The last backup was cancelled."));
                    ui->backupErrorText->setVisible(false);
                } else {
                    ui->backupStateLabel->setText(tr("The last backup resulted in an error. Your files were not backed up."));
                    ui->backupErrorText->setText(get("lastError").toString());
                    ui->backupErrorText->setVisible(true);
                }
                //ui->backupErrorFrame->setVisible(true);
            }
        }
    }
}

void MainWindow::percentageUpdate(int strategy, int percentage) {
    if (ui->strategyList->currentIndex().row() == strategy) {
        ui->backupBar->setMaximum(10000);
        ui->backupBar->setValue(percentage);
    }
}

void MainWindow::on_cancelBackupButton_clicked()
{
    for (QDBusObjectPath p : paths) {
        QDBusInterface i("org.thesuite.thesafe", p.path(), "org.thesuite.thesafe.job");
        if (i.property("Running").toBool() && i.property("Strategy") == ui->strategyList->currentIndex().row()) {
            //Backup is running
            i.call("cancel");
            return;
        }
    }
}

void MainWindow::on_strategyList_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex i = ui->strategyList->indexAt(pos);
    if (!i.isValid()) return;
    QMenu* m = new QMenu();
    m->addSection(tr("For \"%1\"").arg(i.data(Qt::DisplayRole).toString()));
    m->addAction(QIcon::fromTheme("edit-delete"), tr("Remove Strategy"), [=] {
        if (QMessageBox::warning(this, tr("Remove Strategy"), tr("Do you want to remove this strategy?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            strategies->removeStrategy(i.row());
        }
    });
    m->exec(ui->strategyList->mapToGlobal(pos));
}

void MainWindow::on_renameStrategyButton_clicked()
{
    QModelIndex i = ui->strategyList->currentIndex();

    bool ok;
    QString newName = QInputDialog::getText(this, tr("Strategy Name"), tr("Enter a new name for this strategy"), QLineEdit::Normal, i.data(Qt::DisplayRole).toString(), &ok);
    if (ok) {
        set("name", newName);
        ui->strategyName->setText(newName);
        strategies->sync();
    }
}

void MainWindow::on_scheduleNumber_valueChanged(int arg1)
{
    set("backupFrequency", arg1);
}

void MainWindow::on_scheduleUnit_currentIndexChanged(int index)
{
    set("backupUnit", index);
}

void MainWindow::on_manualBackup_toggled(bool checked)
{
    if (checked) {
        set("manual", true);
        ui->scheduleFrame->setVisible(false);
    }
}

void MainWindow::on_scheduledBackup_toggled(bool checked)
{
    if (checked) {
        set("manual", false);
        set("backupFrequency", ui->scheduleNumber->value());
        set("backupUnit", ui->scheduleUnit->currentIndex());
        ui->scheduleFrame->setVisible(true);
    }
}
