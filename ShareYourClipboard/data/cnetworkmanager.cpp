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

#include "cnetworkmanager.h"

cNetworkManager::cNetworkManager(QObject *parent) : QObject(parent),m_TcpServer(TCP_PORT)
{
    m_Server = new QUdpSocket(this);
    if(!m_Server->bind(QHostAddress::AnyIPv4, UDP_PORT))
        qDebug() << "can't bind to port " << UDP_PORT;
    connect(m_Server, SIGNAL(readyRead()), this, SLOT(readyRead()));

    connect(&m_TcpServer,SIGNAL(dataReady(QByteArray&, QHostAddress)), this, SLOT(onDataReady(QByteArray&, QHostAddress)));

    qDebug() << "start";
}

QString cNetworkManager::getAddress()
{
    return m_Server->localAddress().toString();
}

void cNetworkManager::sendData(QByteArray &data, QVector<QHostAddress>& addresses)
{
    for (int i = 0; i< addresses.size(); ++i){
        m_Server->writeDatagram(data,data.size(),addresses[i],UDP_PORT);
        qDebug() << "sended to address: " << addresses[i].toString();
    }

}

void cNetworkManager::sendDataOverTcp(QByteArray &data, QHostAddress &address)
{
    m_TcpClient.connectToHost(address, TCP_PORT);
    m_TcpClient.waitForConnected();
    m_TcpClient.write(data);
}

void cNetworkManager::readyRead()
{
    qDebug() << "received";
    while (m_Server->hasPendingDatagrams()){
        QByteArray buffer;
        buffer.resize(m_Server->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;

        m_Server->readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);
        onDataReady(buffer,sender);
    }
}

void cNetworkManager::onDataReady(QByteArray &data,QHostAddress address)
{
    emit dataReceived(data,address);
}


cTcpSimpleServer::cTcpSimpleServer(int port)
{
    if (!listen(QHostAddress::Any,port)){
            qCritical() << "cTcpSimpleServer start error: " << errorString();
        }
}

void cTcpSimpleServer::incomingConnection(qintptr handle)
{
    QTcpSocket* socket = new QTcpSocket();
    socket->setSocketDescriptor(handle);

    connect(socket,SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(socket,SIGNAL(disconnected()), this, SLOT(onDisconnected()));
}

void cTcpSimpleServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

    QByteArray data = socket->readAll();
    emit dataReady(data,socket->peerAddress());
    socket->disconnectFromHost();
}

void cTcpSimpleServer::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    socket->close();
    socket->deleteLater();
}
