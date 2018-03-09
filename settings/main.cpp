#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName("theSuite");
    a.setOrganizationDomain("");
    a.setApplicationName("theSafe");

    QTranslator* qtTranslator = new QTranslator;
    qtTranslator->load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(qtTranslator);

    QTranslator* translator = new QTranslator;
    translator->load(QLocale::system().name(), "/usr/share/thesafe/translations/");
    a.installTranslator(translator);

    MainWindow w;
    w.show();

    return a.exec();
}

QString calculateSize(quint64 size) {
    QString ret;
    if (size > 1073741824) {
        ret = QString::number(((float) size / 1024 / 1024 / 1024), 'f', 2).append(" GiB");
    } else if (size > 1048576) {
        ret = QString::number(((float) size / 1024 / 1024), 'f', 2).append(" MiB");
    } else if (size > 1024) {
        ret = QString::number(((float) size / 1024), 'f', 2).append(" KiB");
    } else {
        ret = QString::number((float) size, 'f', 2).append(" B");
    }

    return ret;
}
