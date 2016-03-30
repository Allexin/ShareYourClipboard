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

#ifndef CNETWORKMANAGER_H
#define CNETWORKMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QUdpSocket>

class cNetworkManager : public QObject
{
    Q_OBJECT
public:
    static const int    UDP_PORT = 26855;
protected:
    QUdpSocket*         m_Client;
    QUdpSocket*         m_Server;
public:
    explicit cNetworkManager(QObject *parent = 0);

    QString getAddress();

signals:
    void dataReceived(QByteArray data);
public slots:
    void sendData(QByteArray data, QVector<QHostAddress>& addresses);
protected slots:
    void readyRead();
};

#endif // CNETWORKMANAGER_H
