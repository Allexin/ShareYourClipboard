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
#include <QList>
#include <QUrl>
#include "cnetworkmanager.h"
#include "cfileloader.h"
#include "cfilesaver.h"
#include "../UGlobalHotkey-master/uglobalhotkeys.h"

class cMacClipboardMonitor: public QObject{
    Q_OBJECT
protected:
    QClipboard*             m_Clipboard;
    QString                 m_LastClipboardValue;
    QTimer                  m_Timer;

    QString getCurrentClipboardValue(){
        const QMimeData *mimeData = m_Clipboard->mimeData();
        if (mimeData->hasText()) {
            QString clipboard;

            QList<QUrl> urls = mimeData->urls();
            if (urls.size()>0){
                clipboard = urls[0].toString();
                for (int i = 1; i<urls.size(); i++)
                    clipboard += "\n" +urls[i].toString();
            }
            else{
                clipboard = mimeData->text();
            }
            return clipboard;
        }
        return "";
    }

public:
    explicit cMacClipboardMonitor(QClipboard* clipboard){
        m_Clipboard = clipboard;
        connect(&m_Timer,SIGNAL(timeout()),this, SLOT(onTimer()));
        m_Timer.start(500);
    }
signals:
    void clipboardChanged(QClipboard::Mode mode);
protected slots:
    void onTimer(){
        QString text = getCurrentClipboardValue();
        if (text.isEmpty())
            return;
        if (text.compare(m_LastClipboardValue)==0)
            return;
        m_LastClipboardValue = text;
        emit clipboardChanged(QClipboard::Clipboard);
    }
};

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
    static const int NETWORK_ERROR_NO= 0;
    static const int NETWORK_ERROR_CLIPBOARD_HAVE_NOT_ANY_REMOTE_FILES = 1;
    static const int NETWORK_ERROR_CANT_OPEN_REMOTE_FILE = 2;
    static const int NETWORK_ERROR_INCORRECTED_REQUESTED_FILE_PART_SIZE = 3;
    static const int NETWORK_ERROR_CANT_READ_REMOTE_FILE = 4;
protected:    
    bool sendNetworkData(QByteArray& data, QHostAddress* address);

    void sendNetworkResponseFailure(int command, int failureCode, QHostAddress address);
protected:
    cFileSaver          m_FileSaver;
    void sendNetworkRequestFilesGetListHandle(QHostAddress address);

    void processFilesList(QByteArray& data,QHostAddress& address);
    void processFileHandle(QByteArray& data,QHostAddress& address);
    void processFilePart(QByteArray& data,QHostAddress& address);

    void receivedNetworkResponse(QByteArray &data, QHostAddress address);
public:
    void closeFilesList(StringUuid listUuid, QHostAddress address);
    void closeFile(StringUuid listUuid, StringUuid fileUuid, QHostAddress address);
    bool openFile(StringUuid listUuid, QString relativeFilePath, QHostAddress address);
    bool getFilePart(StringUuid listUuid, StringUuid fileUuid, int start, int size, QHostAddress address);
    void setProgressMain(QString text, int value, int max){
        emit onSetProgressMain(text, value, max);
    }
    void setProgressSecond(QString text, int value, int max){
        emit onSetProgressSecond(text, value, max);
    }
protected:
    cFileLoader         m_FileLoader;
    void receivedNetworkFilesGetListHandle(QByteArray &data, QHostAddress address);
    void receivedNetworkFilesCloseListHandle(QByteArray &data);
    void receivedNetworkFilesGetFileHandle(QByteArray &data, QHostAddress address);
    void receivedNetworkFilesCloseFileHandle(QByteArray &data);
    void receivedNetworkFilesGetFilePart(QByteArray &data, QHostAddress address);
    void sendNetworkResponseFilesGetListHandle(QHostAddress address, sFileLoaderClipboard* clipboard);
    void sendNetworkResponseFilesGetFileHandle(QHostAddress address, StringUuid& clipboardUuid, sFileLoaderFileInfo* fileInfo, QString relativeFileName);
    void sendNetworkResponseFilesGetFilePart(QHostAddress address, StringUuid& clipboardUuid, sFileLoaderFileInfo* fileInfo, const char* fileData, int start, int size);

    void receivedNetworkRequest(QByteArray &data, QHostAddress address);
protected:
    UGlobalHotkeys      m_HotKeysManager;

    int                 m_PasteFilesHotkey;
protected:
    QVector<QHostAddress> m_Addresses;
    cNetworkManager     m_NetworkManager;
    eClipboardState     m_CurrentState;
    QHostAddress        m_SenderAddress;
    QByteArray          m_SecretKey;
    QString             m_LastClipboard;
    QClipboard*         m_Clipboard;
#ifdef Q_OS_MAC
    cMacClipboardMonitor m_ClipboardMonitor;
#endif
    void loadPreferences();    
    void sendClipboardText(QString text);
    void receivedNetworkPackage(QByteArray &package, QHostAddress address);
    void receivedNetworkData(QByteArray &data, QHostAddress address);
    void receivedNetworkClipboardText(QByteArray &data, QHostAddress address);
    void setState(cClipboardManager::eClipboardState newState);
public:


    explicit cClipboardManager(QClipboard* clipboard);

    QByteArray generateKey(int length);
    QString getAddress();
signals:
    void onStateChanged(cClipboardManager::eClipboardState newState);

    void onStartCopyProcess(QString operationName);
    void onStopCopyProcess();
    void showMessage(QString message);

    void onSetProgressMain(QString text, int value, int max);
    void onSetProgressSecond(QString text, int value, int max);
protected slots:
    void onClipboardReceived(QClipboard::Mode mode);
    void onNetworkDataReceived(QByteArray& data, QHostAddress address);
    void onDownloadingStop();
    void onHotKeys(size_t id);
public slots:
    void onPreferencesChanged();
    void switchState();
    void pasteFiles();
    void cancelDownloading();
};

#endif // CCLIPBOARDMANAGER_H
