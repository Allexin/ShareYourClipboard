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

    QString readUtf8(){
        int length = read<int>();
        if (length>m_Data.size())
            return "";
        std::shared_ptr<char> stringPtr(new char[length], std::default_delete<char[]>());
        char* string = stringPtr.get();
        readRaw(string,length);
        return QString::fromUtf8(string,length);
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
        static_assert(std::is_same<T, char>::value || std::is_same<T, int>::value || std::is_same<T, int64_t>::value,
                        "only simple types allowed");
        writeRaw(&value, sizeof(T));
    }

    void writeUtf8(QString str){
        QByteArray string = str.toUtf8();
        write<int>(string.size());
        writeRaw(string.constData(),string.size());
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
    m_SecretKey = settings.value(cClipboardManager::CONF_SECRET_KEY_ID,"").toString().toUtf8();

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
const int NETWORK_DATA_TYPE_REQUEST = 2;
const int NETWORK_DATA_TYPE_RESPONSE = 3;


const int NETWORK_DATA_CLIPBOARD_FILES_GET_LIST_HANDLE     = 1001;
const int NETWORK_DATA_CLIPBOARD_FILES_CLOSE_LIST_HANDLE   = 1002;

const int NETWORK_DATA_CLIPBOARD_FILES_GET_FILE_HANDLE     = 1011;
const int NETWORK_DATA_CLIPBOARD_FILES_CLOSE_FILE_HANDLE   = 1012;
const int NETWORK_DATA_CLIPBOARD_FILES_GET_FILE_PART       = 1013;


void cClipboardManager::sendNetworkResponseFailure(int command, int failureCode, QHostAddress address)
{
    QByteArray data;
    cWriteStream stream(data);
    stream.write<int>(NETWORK_DATA_TYPE_RESPONSE);
    stream.write<int>(command);
    stream.write<int>(failureCode);

    sendNetworkData(data, &address);
}

void cClipboardManager::sendNetworkRequestFilesGetListHandle(QHostAddress address)
{
    QByteArray data;
    cWriteStream stream(data);
    stream.write<int>(NETWORK_DATA_TYPE_REQUEST);
    stream.write<int>(NETWORK_DATA_CLIPBOARD_FILES_GET_LIST_HANDLE);
    sendNetworkData(data,&address);
    qDebug() << " try get files from " << address.toString();
}

void cClipboardManager::receivedNetworkResponse(QByteArray &data, QHostAddress address)
{
    cReadStream stream(data);
    stream.skip(sizeof(int));//skip @response@
    int command = stream.read<int>();
    int result = stream.read<int>();
    if (result!=0){
        qDebug() << "response for command " << command << " is error " << result;
        return;
    }

    qDebug() << data;
    //TODO - process success packages
}

void cClipboardManager::receivedNetworkFilesGetListHandle(QByteArray &data, QHostAddress address)
{
    cReadStream stream(data);
    stream.skip(sizeof(int));//skip request
    int command = stream.read<int>();

    qDebug() << "receivedNetworkFilesGetListHandle";

    if (m_CurrentState!=SENDED){
        sendNetworkResponseFailure(command,NETWORK_ERROR_CLIPBOARD_HAVE_NOT_ANY_REMOTE_FILES,address);
        return;
    }

    QStringList items = m_LastClipboard.split("\n");
    QStringList files;
    for (int i = 0; i<items.size(); i++){
        QString str = items[i].trimmed();
        if (str.mid(0,8)=="file:///"){
#ifdef Q_OS_WIN
            files.append(str.mid(8));
#else
            files.append(str.mid(7));
#endif
        }
    }

    if (files.size()==0){
        sendNetworkResponseFailure(command,NETWORK_ERROR_CLIPBOARD_HAVE_NOT_ANY_REMOTE_FILES,address);
        return;
    }

    sFileLoaderClipboard* clipboard = m_FileLoader.openClipboardFiles(files);
    if (clipboard==NULL){
        sendNetworkResponseFailure(command,NETWORK_ERROR_CLIPBOARD_HAVE_NOT_ANY_REMOTE_FILES,address);
        return;
    }

    sendNetworkResponseFilesGetListHandle(address,clipboard);

}

void cClipboardManager::receivedNetworkFilesCloseListHandle(QByteArray &data)
{
    cReadStream stream(data);
    stream.skip(sizeof(int));//skip request
    int command = stream.read<int>();

    qDebug() << "receivedNetworkFilesCloseListHandle";

    StringUuid clipboardUuid = stream.readUtf8();
    m_FileLoader.closeClipboard(clipboardUuid);
}

void cClipboardManager::receivedNetworkFilesGetFileHandle(QByteArray &data, QHostAddress address)
{
    cReadStream stream(data);
    stream.skip(sizeof(int));//skip request
    int command = stream.read<int>();\

    StringUuid clipboardUuid = stream.readUtf8();
    QString relativeFileName = stream.readUtf8();

    qDebug() << "receivedNetworkFilesGetFileHandle " << relativeFileName;

    sFileLoaderFileInfo* file =  m_FileLoader.openFile(clipboardUuid,relativeFileName);
    if (!file){
        sendNetworkResponseFailure(command,NETWORK_ERROR_CANT_OPEN_REMOTE_FILE,address);
        return;
    }

    sendNetworkResponseFilesGetFileHandle(address,clipboardUuid,file,relativeFileName);
}

void cClipboardManager::receivedNetworkFilesCloseFileHandle(QByteArray &data)
{
    cReadStream stream(data);
    stream.skip(sizeof(int));//skip request
    int command = stream.read<int>();

    qDebug() << "receivedNetworkFilesCloseFileHandle";

    StringUuid clipboardUuid = stream.readUtf8();
    StringUuid fileUuid = stream.readUtf8();
    m_FileLoader.closeFile(clipboardUuid,fileUuid);
}

void cClipboardManager::receivedNetworkFilesGetFilePart(QByteArray &data, QHostAddress address)
{
    cReadStream stream(data);
    stream.skip(sizeof(int));//skip request
    int command = stream.read<int>();

    StringUuid clipboardUuid = stream.readUtf8();
    StringUuid fileUuid = stream.readUtf8();
    int filePartStart = stream.read<int>();
    int filePartSize = stream.read<int>();

    qDebug() << "receivedNetworkFilesGetFilePart " <<filePartStart;

    if (filePartSize>MAX_FILE_PART_SIZE){
        sendNetworkResponseFailure(command,NETWORK_ERROR_TOO_BIG_REQUESTED_FILE_PART,address);
        return;
    }

    std::shared_ptr<char> dataPtr(new char[filePartSize], std::default_delete<char[]>());
    char* fileData = dataPtr.get();
    sFileLoaderFileInfo* readedFrom = m_FileLoader.getFilePart(clipboardUuid,fileUuid,filePartStart,filePartSize,fileData);
    if (!readedFrom){
        sendNetworkResponseFailure(command,NETWORK_ERROR_CANT_READ_REMOTE_FILE,address);
        return;
    }
    sendNetworkResponseFilesGetFilePart(address,clipboardUuid,readedFrom,fileData,filePartStart,filePartSize);
}

void writeItemToStream(cWriteStream& stream, sFileLoaderFileInfo* file, const QString& path){
    stream.writeUtf8(path+file->fileName);
    stream.write<int64_t>(file->fileSize);
    stream.write<int>(file->isDir?1:0);
    stream.write<int>(file->isExecutable?1:0);
    if (file->isDir){
        stream.write<int>(file->chields.count());
        foreach(auto item, file->chields){
            writeItemToStream(stream,item,path+file->fileName+"/");
        }
    }
    else
        stream.write<int>(0);
}

void cClipboardManager::sendNetworkResponseFilesGetListHandle(QHostAddress address, sFileLoaderClipboard *clipboard)
{
    QByteArray data;
    cWriteStream stream(data);

    stream.write<int>(NETWORK_DATA_TYPE_RESPONSE);
    stream.write<int>(NETWORK_DATA_CLIPBOARD_FILES_GET_LIST_HANDLE);
    stream.write<int>(NETWORK_ERROR_NO);

    stream.writeUtf8(clipboard->handle);
    stream.write<int64_t>(clipboard->totalSize);
    stream.write<int>(clipboard->rootFiles.count());
    foreach(auto item, clipboard->rootFiles){
        writeItemToStream(stream,item,"");
    }
    sendNetworkData(data,&address);
}

void cClipboardManager::sendNetworkResponseFilesGetFileHandle(QHostAddress address, StringUuid &clipboardUuid, sFileLoaderFileInfo *fileInfo, QString relativeFileName)
{
    QByteArray data;
    cWriteStream stream(data);

    stream.write<int>(NETWORK_DATA_TYPE_RESPONSE);
    stream.write<int>(NETWORK_DATA_CLIPBOARD_FILES_GET_FILE_HANDLE);
    stream.write<int>(NETWORK_ERROR_NO);

    stream.writeUtf8(clipboardUuid);
    stream.writeUtf8(fileInfo->handle);
    stream.writeUtf8(relativeFileName);
    stream.write<int>(fileInfo->isExecutable?1:0);
    stream.write<int>(fileInfo->fileSize);
    sendNetworkData(data,&address);
}

void cClipboardManager::sendNetworkResponseFilesGetFilePart(QHostAddress address, StringUuid &clipboardUuid, sFileLoaderFileInfo *fileInfo, const char *fileData, int start, int size)
{
    QByteArray data;
    cWriteStream stream(data);

    stream.write<int>(NETWORK_DATA_TYPE_RESPONSE);
    stream.write<int>(NETWORK_DATA_CLIPBOARD_FILES_GET_FILE_PART);
    stream.write<int>(NETWORK_ERROR_NO);

    stream.writeUtf8(clipboardUuid);
    stream.writeUtf8(fileInfo->handle);
    stream.write<int>(start);
    stream.write<int>(size);
    stream.writeRaw(fileData,size);
    sendNetworkData(data,&address);
}

void cClipboardManager::sendNetworkData(QByteArray& data, QHostAddress* address)
{
    cWriteStream checkSumStream(data);
    checkSumStream.write<int>(getSimpleCheckSum(data));

    int keySize = DISPOSABLE_KEY_SIZE;
    QByteArray key = generateKey(keySize);
    int keyPos = 0;
    for (int i = 0; i<data.size(); ++i){
        data[i] = data[i] ^ key[keyPos];
        keyPos++;
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
            package[i] = package[i] ^ m_SecretKey.constData()[keyPos];
            keyPos++;
            if (keyPos>=m_SecretKey.size())
                keyPos = 0;
        }
    if (address)
        m_NetworkManager.sendDataOverTcp(package,*address);
    else
        m_NetworkManager.sendData(package,m_Addresses);
}

void cClipboardManager::sendClipboardText(QString text)
{
    QByteArray data;
    cWriteStream stream(data);

    stream.write<int>(NETWORK_DATA_TYPE_CLIPBOARD_TEXT);
    stream.writeUtf8(text);    
    m_LastClipboard = text;
    qDebug() << "clipboard sended: " << m_LastClipboard;
    sendNetworkData(data,NULL);
    setState(SENDED);
}

void cClipboardManager::receivedNetworkPackage(QByteArray& package, QHostAddress address)
{
    int keyPos = 0;
    if (m_SecretKey.size()>0)
    for (int i = 0; i<package.size(); ++i){
        package[i] = package[i] ^ m_SecretKey[keyPos];
        keyPos++;
        if (keyPos>=m_SecretKey.size())
            keyPos = 0;
    }

    int checkSum = getSimpleCheckSum(package,package.size()-4);
    int receivedCheckSum = *((int*)(&(package.constData()[package.size()-4])));
    if (checkSum!=receivedCheckSum)
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

    std::shared_ptr<char> keyPtr(new char[keySize], std::default_delete<char[]>());
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
        keyPos++;
        if (keyPos>=keySize)
            keyPos = 0;
        if (stream.atEnd())
            return;
    }


    if (getSimpleCheckSum(content,content.size()-4)!=*(int*)(&(content.constData()[content.size()-4])))
        return;
    receivedNetworkData(content,address);
}

void cClipboardManager::receivedNetworkData(QByteArray &data, QHostAddress address)
{
    cReadStream stream(data);
    int type = stream.read<int>();
    switch(type) {
        case NETWORK_DATA_TYPE_CLIPBOARD_TEXT:
            receivedNetworkClipboardText(data,address);
        return;
        case NETWORK_DATA_TYPE_REQUEST:
            receivedNetworkRequest(data,address);
        return;
        case NETWORK_DATA_TYPE_RESPONSE:
            receivedNetworkResponse(data,address);
        return;
    }
}

void cClipboardManager::receivedNetworkRequest(QByteArray &data, QHostAddress address)
{
    cReadStream stream(data);
    stream.skip(sizeof(int));//skip type
    int request = stream.read<int>();
    switch(request){
    case NETWORK_DATA_CLIPBOARD_FILES_GET_LIST_HANDLE  :
        receivedNetworkFilesGetListHandle(data,address);
        return;
    case NETWORK_DATA_CLIPBOARD_FILES_CLOSE_LIST_HANDLE:
        receivedNetworkFilesCloseListHandle(data);
        return;
    case NETWORK_DATA_CLIPBOARD_FILES_GET_FILE_HANDLE   :
        receivedNetworkFilesGetFileHandle(data,address);
        return;
    case NETWORK_DATA_CLIPBOARD_FILES_CLOSE_FILE_HANDLE :
        receivedNetworkFilesCloseFileHandle(data);
        return;
    case NETWORK_DATA_CLIPBOARD_FILES_GET_FILE_PART     :
        receivedNetworkFilesGetFilePart(data,address);
        return;
    }
}

void cClipboardManager::receivedNetworkClipboardText(QByteArray &data, QHostAddress address)
{
    cReadStream stream(data);
    stream.skip(sizeof(int)); //skip data type
    QString string = stream.readUtf8();
    qDebug() << "received clipboard: " << string;
    if (string!=m_LastClipboard && m_CurrentState!=DISABLED){
        m_LastClipboard = string;
        m_SenderAddress = address;
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
    connect(&m_NetworkManager,SIGNAL(dataReceived(QByteArray&,QHostAddress)), this, SLOT(onNetworkDataReceived(QByteArray&,QHostAddress)));
    m_Clipboard = clipboard;
    connect(clipboard, SIGNAL(changed(QClipboard::Mode)),this, SLOT(onClipboardReceived(QClipboard::Mode))) ;
    m_CurrentState = ENABLED;

    loadPreferences();
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

void cClipboardManager::onNetworkDataReceived(QByteArray &data, QHostAddress address)
{
    receivedNetworkPackage(data,address);
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

void cClipboardManager::pasteFiles()
{
    if (m_CurrentState==RECEIVED)
        sendNetworkRequestFilesGetListHandle(m_SenderAddress);
}

