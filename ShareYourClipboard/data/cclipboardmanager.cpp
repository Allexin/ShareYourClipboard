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

#include "cclipboardmanager.h"
#include <memory>
#include <QSettings>
#include <QDebug>

class cReadStream{
protected:
    QByteArray&         m_Data;
    int                 m_Pos;
public:
    cReadStream(QByteArray& data): m_Data(data){
        m_Pos = 0;
    }

    void skip(int length){
        m_Pos+=length;
        if (m_Pos>m_Data.size())
            m_Pos = m_Data.size();
    }

    void readRaw(void* buffer, int size){
        if (m_Pos==m_Data.size())
            return;
        if (m_Pos+size>m_Data.size())
            size = m_Data.size()-m_Pos;
        memcpy(buffer, &m_Data.constData()[m_Pos],size);
        m_Pos+=size;
    }

    template<typename T>
    T read() {
        static_assert(std::is_same<T, char>::value || std::is_same<T, int>::value,
                        "only simple types allowed");
        T val = 0;
        readRaw(&val, sizeof(T));
        return val;
    }


    bool atEnd(){
        return m_Pos==m_Data.size();
    }
};

class cWriteStream{
protected:
    QByteArray&         m_Data;
public:
    cWriteStream(QByteArray& data): m_Data(data){}

    void writeRaw(const void* buffer, int size){
        int pos = m_Data.size();
        m_Data.resize(pos+size);
        memcpy(&m_Data.data()[pos],buffer,size);
    }

    template<typename T>
    void write(T value) {
        static_assert(std::is_same<T, char>::value || std::is_same<T, int>::value,
                        "only simple types allowed");
        writeRaw(&value, sizeof(T));
    }
};

int getSimpleCheckSum(QByteArray& buffer, int size = -1){
    if (size==-1)
        size=buffer.size();
    int checkSum = 0;
    for (int i = 0; i<size; ++i)
        checkSum += buffer[i];
    return checkSum;
}

const QString cClipboardManager::CONF_SECRET_KEY_ID = "SECRET_KEY";
const QString cClipboardManager::CONF_ADDRESSES_COUNT_ID = "ADDRESSED_COUNT";
const QString cClipboardManager::CONF_ADDRESS_ID = "ADDRESSED_";

void cClipboardManager::loadPreferences()
{
    QSettings settings;
    m_SecretKey = settings.value(cClipboardManager::CONF_SECRET_KEY_ID,generateKey(32)).toString().toUtf8();

    int addrCount = settings.value(cClipboardManager::CONF_ADDRESSES_COUNT_ID,0).toInt();
    if (addrCount==0){
        settings.setValue(cClipboardManager::CONF_ADDRESSES_COUNT_ID,1);
        settings.setValue(cClipboardManager::CONF_ADDRESS_ID+"0","255.255.255.255");
        settings.sync();
    }

    m_Addresses.clear();
    for (int i = 0; i<addrCount; ++i)
        m_Addresses.push_back(QHostAddress(settings.value(cClipboardManager::CONF_ADDRESS_ID+QString::number(i),"").toString()));
}

const char* NETWORK_PACKAGE_PREFIX = "SYCNP";
const int NETWORK_PACKAGE_PREFIX_SIZE = 5;

const int NETWORK_PACKAGE_VERSION = 1;
const int DISPOSABLE_KEY_SIZE = 64;

const int NETWORK_DATA_TYPE_CLIPBOARD_TEXT = 1;

void cClipboardManager::sendNetworkData(QByteArray& data)
{
    int keySize = DISPOSABLE_KEY_SIZE;
    QByteArray key = generateKey(keySize);
    int keyPos = 0;
    for (int i = 0; i<data.size(); ++i){
        data[i] = data[i] ^ key[keyPos];
        if (keyPos>=key.size())
            keyPos=0;
    }

    QByteArray package;
    cWriteStream stream(package);
    stream.write<int>(NETWORK_PACKAGE_VERSION);
    stream.writeRaw(NETWORK_PACKAGE_PREFIX,NETWORK_PACKAGE_PREFIX_SIZE);
    stream.write<int>(key.size());
    stream.writeRaw(key.constData(),key.size());
    stream.write<int>(data.size());
    stream.writeRaw(data.constData(),data.size());
    stream.write<int>(getSimpleCheckSum(package));

    keyPos = 0;
    if (m_SecretKey.size()>0)
        for (int i = 0; i<package.size(); ++i){
            package[i] = package[i] ^ m_SecretKey[keyPos];
            if (keyPos>=m_SecretKey.size())
                keyPos = 0;
        }
    m_NetworkManager.sendData(package,m_Addresses);
}

void cClipboardManager::sendClipboardText(QString text)
{
    QByteArray data;
    cWriteStream stream(data);

    stream.write<int>(NETWORK_DATA_TYPE_CLIPBOARD_TEXT);
    QByteArray buf = text.toUtf8();
    stream.write<int>(buf.size());
    stream.writeRaw(buf.constData(),buf.size());
    stream.write<int>(getSimpleCheckSum(data));
    m_LastClipboard = text;
    qDebug() << "clipboard sended: " << m_LastClipboard;
    sendNetworkData(data);
    emit onStateChanged(SENDED);
}

void cClipboardManager::receivedNetworkPackage(QByteArray& package)
{
    int keyPos = 0;
    if (m_SecretKey.size()>0)
    for (int i = 0; i<package.size(); ++i){
        package[i] = package[i] ^ m_SecretKey[keyPos];
        keyPos++;
        if (keyPos>=m_SecretKey.size())
            keyPos = 0;
    }

    if (getSimpleCheckSum(package,package.size()-4)!=*(int*)(&(package.constData()[package.size()-4])))
        return;

    cReadStream stream(package);
    int Version = stream.read<int>();
    if (Version!=NETWORK_PACKAGE_VERSION)
        return;
    char prefix[NETWORK_PACKAGE_PREFIX_SIZE+1]; //add zero for simple convert to string
    prefix[NETWORK_PACKAGE_PREFIX_SIZE] = 0;
    stream.readRaw(prefix,NETWORK_PACKAGE_PREFIX_SIZE);
    if (memcmp(prefix,NETWORK_PACKAGE_PREFIX,NETWORK_PACKAGE_PREFIX_SIZE)!=0)
        return;

    int keySize = stream.read<int>();
    if (keySize!=DISPOSABLE_KEY_SIZE)
        return;

    std::shared_ptr<char> keyPtr(new char(keySize));
    char* key = keyPtr.get();
    stream.readRaw(key,keySize);
    if (stream.atEnd())
        return;

    int contentSize = stream.read<int>();
    if (contentSize>package.size())
        return;
    keyPos = 0;
    QByteArray content;
    content.resize(contentSize);

    for (int i = 0; i<contentSize; ++i){
        char c = stream.read<char>();
        content[i] = c ^ key[keyPos];
        if (keyPos>=keySize)
            keyPos = 0;
        if (stream.atEnd())
            return;
    }


    if (getSimpleCheckSum(content,content.size()-4)!=*(int*)(&(content.constData()[content.size()-4])))
        return;
    receivedNetworkData(content);
}

void cClipboardManager::receivedNetworkData(QByteArray &data)
{
    cReadStream stream(data);
    int type = stream.read<int>();
    switch(type) {
        case NETWORK_DATA_TYPE_CLIPBOARD_TEXT:
            receivedNetworkClipboardText(data);
        return;
    }
}

void cClipboardManager::receivedNetworkClipboardText(QByteArray &data)
{
    cReadStream stream(data);
    stream.skip(sizeof(int)); //skip data type
    int textSize = stream.read<int>();
    std::shared_ptr<char> textPtr(new char(textSize+1));
    char* text = textPtr.get();
    text[textSize] = 0;
    stream.readRaw(text,textSize);

    QString string(text);
    qDebug() << "received clipboard: " << string;
    if (string!=m_LastClipboard && m_CurrentState!=DISABLED){
        m_LastClipboard = string;
        setState(RECEIVED);
        m_Clipboard->setText(string,QClipboard::Clipboard);
    }
}

void cClipboardManager::setState(cClipboardManager::eClipboardState newState)
{
    m_CurrentState = newState;
    emit onStateChanged(m_CurrentState);
}

cClipboardManager::cClipboardManager(QClipboard* clipboard) : QObject(0)
{
    connect(&m_NetworkManager,SIGNAL(dataReceived(QByteArray)), this, SLOT(onNetworkClipboardReceived(QByteArray)));
    m_Clipboard = clipboard;
    connect(clipboard, SIGNAL(changed(QClipboard::Mode)),this, SLOT(onClipboardReceived(QClipboard::Mode))) ;
    m_CurrentState = ENABLED;
}

QByteArray cClipboardManager::generateKey(int length)
{
    QByteArray key;
    key.resize(length);
    for (int i = 0; i<length; ++i){
        key[i] = 33+rand() % 92;
    }
    return key;
}

QString cClipboardManager::getAddress()
{
    return m_NetworkManager.getAddress();
}

void cClipboardManager::onClipboardReceived(QClipboard::Mode mode)
{
    if (mode!=QClipboard::Clipboard)
        return;

    const QMimeData *mimeData = m_Clipboard->mimeData();
    if (mimeData->hasText()) {
        QString clipboard = mimeData->text();
        qDebug() << "clipboard captured: " <<clipboard;
        if (clipboard!=m_LastClipboard){
            sendClipboardText(clipboard);
        }
    }
}

void cClipboardManager::onNetworkClipboardReceived(QByteArray data)
{
    receivedNetworkPackage(data);
}

void cClipboardManager::onPreferencesChanged()
{
    loadPreferences();
}

void cClipboardManager::switchState()
{
    if (m_CurrentState==DISABLED)
        setState(ENABLED);
    else
        setState(DISABLED);
}

