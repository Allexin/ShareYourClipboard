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

#ifndef CTRAYICON_H
#define CTRAYICON_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QVector>
#include <QAction>
#include "../data/cclipboardmanager.h"

class cTrayIcon : public QSystemTrayIcon
{
    Q_OBJECT
protected:
    cClipboardManager*  m_ClipboardManager;
    QMenu               m_Menu;
public:
    cTrayIcon(cClipboardManager* ClipboardManager);

signals:
    void showSettings();
    void switchState();
    void pasteFiles();
public slots:
    void onTray(QSystemTrayIcon::ActivationReason reason);
    void setState(cClipboardManager::eClipboardState state);
    void onMenuSelection(QAction* menuAction);
};

#endif // CTRAYICON_H
