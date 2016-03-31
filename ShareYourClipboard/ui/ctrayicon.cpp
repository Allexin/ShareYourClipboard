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

#include "ctrayicon.h"
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

cTrayIcon::cTrayIcon(cClipboardManager *ClipboardManager):QSystemTrayIcon()
{    
    m_ClipboardManager = ClipboardManager;

    connect(this,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(onTray(QSystemTrayIcon::ActivationReason)));

    connect(&m_Menu, SIGNAL(triggered(QAction*)), this, SLOT(onMenuSelection(QAction*)));
    setContextMenu(&m_Menu);
    m_Menu.addAction(tr("Disable"))->setData("SWITCH");
    m_Menu.addSeparator();
    m_Menu.addAction(tr("Paste files..."))->setData("PASTE_FILES");
    m_Menu.addSeparator();
    m_Menu.addAction(tr("Settings..."))->setData("SETTINGS");
    m_Menu.addSeparator();
    m_Menu.addAction(tr("Exit"))->setData("EXIT");

    setState(cClipboardManager::ENABLED);
    show();
}

void cTrayIcon::onTray(QSystemTrayIcon::ActivationReason reason)
{
    if (reason==QSystemTrayIcon::Trigger)
        emit switchState();
}

void cTrayIcon::setState(cClipboardManager::eClipboardState state)
{
    QAction* stateSwitch = NULL;
    QList<QAction*> actions = m_Menu.actions();
    for (int i = 0; i<actions.size(); i++){
        QString id = actions[i]->data().toString();
        if (id=="SWITCH"){
            stateSwitch = actions[i];
            break;
        }
    }

    if (stateSwitch){
        if (state==cClipboardManager::DISABLED){
            stateSwitch->setText(tr("Enable"));
        }
        else{
            stateSwitch->setText(tr("Disable"));
        }
    }
    switch(state){
        case cClipboardManager::DISABLED:setIcon(QIcon("data/icons/disabled.png"));break;
        case cClipboardManager::ENABLED:setIcon(QIcon("data/icons/waiting.png"));break;
        case cClipboardManager::RECEIVED:setIcon(QIcon("data/icons/received.png"));break;
        case cClipboardManager::SENDED:setIcon(QIcon("data/icons/sended.png"));break;
    }
}

void cTrayIcon::onMenuSelection(QAction *menuAction)
{
    QString id = menuAction->data().toString();
    if (id=="EXIT"){
        QApplication::quit();
        return;
    }

    if (id=="SETTINGS"){
        emit showSettings();
        return;
    }

    if (id=="SWITCH"){
        emit switchState();
        return;
    }

    if (id=="PASTE_FILES"){
        emit pasteFiles();
        return;
    }
}


