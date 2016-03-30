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

#ifndef CCLIPBOARDMANAGER_H
#define CCLIPBOARDMANAGER_H

#include <QObject>
#include <QClipboard>
#include <QMimeData>
#include "cnetworkmanager.h"

class cClipboardManager : public QObject
{
    Q_OBJECT
public:
    static const QString CONF_SECRET_KEY_ID;
    static const QString CONF_ADDRESSES_COUNT_ID;
    static const QString CONF_ADDRESS_ID;
    enum eClipboardState{
        DISABLED,
        ENABLED,
        RECEIVED,
        SENDED
    };
protected:
    QVector<QHostAddress> m_Addresses;
    cNetworkManager     m_NetworkManager;
    eClipboardState     m_CurrentState;
    QByteArray          m_SecretKey;
    QString             m_LastClipboard;
    QClipboard*         m_Clipboard;
    void loadPreferences();
    void sendNetworkData(QByteArray& data);
    void sendClipboardText(QString text);
    void receivedNetworkPackage(QByteArray &package);
    void receivedNetworkData(QByteArray &data);
    void receivedNetworkClipboardText(QByteArray &data);
    void setState(cClipboardManager::eClipboardState newState);
public:


    explicit cClipboardManager(QClipboard* clipboard);

    QByteArray generateKey(int length);
    QString getAddress();
signals:
    void onStateChanged(cClipboardManager::eClipboardState newState);
protected slots:
    void onClipboardReceived(QClipboard::Mode mode);
    void onNetworkClipboardReceived(QByteArray data);
public slots:
    void onPreferencesChanged();
    void switchState();
};

#endif // CCLIPBOARDMANAGER_H
