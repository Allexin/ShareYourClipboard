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

#include "settingswindow.h"
#include "ui_settingswindow.h"

#include <QSettings>

void SettingsWindow::loadPreferences()
{
    QSettings settings;

    QString secretKey = settings.value(cClipboardManager::CONF_SECRET_KEY_ID,"").toString();
    ui->lineEditSecretKey->setText(secretKey);


    int addrCount = settings.value(cClipboardManager::CONF_ADDRESSES_COUNT_ID,0).toInt();
    if (addrCount==0){
        ui->plainTextEditAddresses->setPlainText("255.255.255.255");
    }
    else{
        QString addresses = "";
        for (int i = 0; i<addrCount; ++i)
            addresses+=(i!=0?"\n":"")+settings.value(cClipboardManager::CONF_ADDRESS_ID+QString::number(i),"").toString();
        ui->plainTextEditAddresses->setPlainText(addresses);
    }

    ui->labelLocalAddress->setText(m_ClipboardManager->getAddress());
}

void SettingsWindow::applyPreferences()
{
    QSettings settings;

    settings.setValue(cClipboardManager::CONF_SECRET_KEY_ID,ui->lineEditSecretKey->text());
    QStringList addresses = ui->plainTextEditAddresses->toPlainText().split("\n");
    settings.setValue(cClipboardManager::CONF_ADDRESSES_COUNT_ID,addresses.size());
    for (int i = 0; i<addresses.size(); ++i){
        settings.setValue(cClipboardManager::CONF_ADDRESS_ID+QString::number(i),addresses[i]);
    }
    settings.sync();

    emit preferencesChange();
}


SettingsWindow::SettingsWindow(cClipboardManager* ClipboardManager) : QMainWindow(0),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    setWindowFlags( Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint );

    connect(ui->pushButtonApply, SIGNAL (released()),this, SLOT (handleButtonApply()));
    connect(ui->pushButtonCancel, SIGNAL (released()),this, SLOT (handleButtonCancel()));
    connect(ui->pushButtonGenerateSecretKey, SIGNAL (released()),this, SLOT (handleButtonGenerateSecretKey()));

    m_ClipboardManager = ClipboardManager;
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent( event );
    loadPreferences();

    raise();
    activateWindow();
}

void SettingsWindow::handleButtonApply()
{
    applyPreferences();
    close();
}

void SettingsWindow::handleButtonCancel()
{
    close();
}

void SettingsWindow::handleButtonGenerateSecretKey()
{
    ui->lineEditSecretKey->setText(m_ClipboardManager->generateKey(32));
}


