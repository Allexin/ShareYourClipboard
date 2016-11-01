/*
 * ShareYourClipboard - cross-platform clipboard share tool
 * Copyright (C) 2015-2016  Alexander Basov <basovav@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QDebug>

#include "ui/ctrayicon.h"
#include "ui/settingswindow.h"
#include "ui/copyprogressdialog.h"
#include "data/cclipboardmanager.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("SRFGames");
    QCoreApplication::setOrganizationDomain("sol-online.org"),
    QCoreApplication::setApplicationName("ShareYourClipboard");

#ifdef Q_OS_MAC
    QDir dir(argv[0]);
    dir.cdUp();
    QString currentDir = dir.absolutePath();
    dir.cdUp();
    dir.cd("PlugIns");
    QCoreApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif

    QApplication a(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);

#ifdef Q_OS_MAC
    QDir::setCurrent(currentDir);
#endif

    qDebug() << "init manager\n";
    cClipboardManager manager(a.clipboard());

    qDebug() << "init tray icon\n";
    cTrayIcon trIcon(&manager);
    QObject::connect(&trIcon, SIGNAL(switchState()), &manager, SLOT(switchState()));
    QObject::connect(&trIcon, SIGNAL(pasteFiles()), &manager, SLOT(pasteFiles()));
    QObject::connect(&manager, SIGNAL(onStateChanged(cClipboardManager::eClipboardState)), &trIcon, SLOT(setState(cClipboardManager::eClipboardState)));

    qDebug() << "init settings window\n";
    SettingsWindow settingsWindow(&manager);
    QObject::connect(&trIcon, SIGNAL(showSettings()), &settingsWindow, SLOT(showNormal()));
    QObject::connect(&settingsWindow, SIGNAL(preferencesChange()), &manager, SLOT(onPreferencesChanged()));

    qDebug() << "init copy dialog\n";
    CopyProgressDialog copydialog;
    QObject::connect(&manager, SIGNAL(onStartCopyProcess(QString)), &copydialog, SLOT(start(QString)));
    QObject::connect(&manager, SIGNAL(onStopCopyProcess()), &copydialog, SLOT(stop()));
    QObject::connect(&manager, SIGNAL(showMessage(QString)), &copydialog, SLOT(showMessage(QString)));
    QObject::connect(&manager, SIGNAL(onSetProgressMain(QString,int,int)), &copydialog, SLOT(setProgressMain(QString,int,int)));
    QObject::connect(&manager, SIGNAL(onSetProgressSecond(QString,int,int)), &copydialog, SLOT(setProgressSecond(QString,int,int)));
    QObject::connect(&copydialog, SIGNAL(cancel()), &manager, SLOT(cancelDownloading()));

    qDebug() << "start app loop\n";
    int result = a.exec();
    qDebug() << "application close\n";
    return result;
}
