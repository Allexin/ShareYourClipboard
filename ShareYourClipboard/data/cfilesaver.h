#ifndef CFILESAVER_H
#define CFILESAVER_H

#include <QObject>
#include <QVector>
#include <QHostAddress>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include "utils.h"


struct sFilePart{
    int startPos;
    int size;
    QByteArray data;
    bool dataReceived;
};

class cClipboardManager;
class cFileSaver;

class cFileSaverFile: public QObject{
    friend class cFileSaverList;
    Q_OBJECT
protected:
    StringUuid          m_Handle;
    int                 m_Size;
    QString             m_RelativeFileName;
    QString             m_OverridedFileName;
    bool                m_IsExecutable;
    QVector<sFilePart>  m_FileParts;
public:
    cFileSaverFile(int size, QString relativeFileName, bool isExecutable);

    QString getRelativeFileName(){return m_RelativeFileName;}
    void overrideFileName(QString newFileName){m_OverridedFileName = newFileName;}
};

class cFileSaverList: public QObject{
    Q_OBJECT
protected:
    StringUuid          m_Handle;
    int                 m_TotalFilesCount;
    QVector<cFileSaverFile*> m_Files;
    QVector<QString>    m_Dirs;
    int                 m_TotalSize;
    QString             m_LocalPath;
    cFileSaver*         m_Parent;
    QHostAddress        m_HostAddress;

    bool                m_AlwaysSkipErrorFiles;
    bool                m_AlwaysOverwriteFiles;
    bool                m_AlwaysSkipFolderFiles;
    void processNextFilePart();
    void FilesRemove(int id);
public:
    cFileSaverList(cFileSaver* parent);
    ~cFileSaverList();
    void setHandle(StringUuid handle){m_Handle = handle;}
    void addFile(int size, QString relativeFileName, bool isExecutable);
    void addDir(QString relativeFileName);

    int dirsCount(){return m_Dirs.size();}
    QString getDirRelativePath(int index){return m_Dirs[index];}

    int filesCount(){return m_Files.size();}
    cFileSaverFile* getFile(int index){return m_Files[index];}

    void clear();
    void startDownloading(QString localPath, QHostAddress host);



    void fileOpenResult(int resultCode, StringUuid fileUuid, int fileSize, bool isExecutable, QString relativeFileName);
    void fileGetPart(int resultCode, StringUuid fileUuid, int start, int size, QByteArray& data);
};

class cFileSaver: public QObject
{
    Q_OBJECT
protected:
    cFileSaverList      m_FilesList;
    bool                m_Downloading;
    cClipboardManager*  m_Parent;
public:
    cFileSaver(cClipboardManager* parent);

    bool isDownloading(){return m_Downloading;}
    cFileSaverList* initFilesDownloading(StringUuid listHandle);
    cFileSaverList* getList(){return m_Downloading?&m_FilesList:NULL;}
    void stopDownloading();

    cClipboardManager* parent(){return m_Parent;}
signals:
    void onStop();
};

#endif // CFILESAVER_H
