#ifndef CFILELOADER_H
#define CFILELOADER_H

#include <QObject>
#include <QUuid>
#include <QMap>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QTimer>
#include <QDir>

class sFileLoaderClipboard;

typedef QString StringUuid;

class sFileLoaderFileInfo: public QObject{
    Q_OBJECT
protected:
    const int LIFE_TIME = 60;//at least 1 minute
public:
    StringUuid handle;
    int fileOpenTimeOut; //0 - file is closed
    QFile file;
    QString fileName;
    int fileSize;
    bool isDir;
    bool isExecutable;
    QMap<QString,sFileLoaderFileInfo*> chields;

    bool open();
    void close();
    bool read(int start, int size, char *outData);

    sFileLoaderFileInfo(sFileLoaderClipboard* parent,QFileInfo& FileInfo);
    ~sFileLoaderFileInfo(){
        foreach (auto item, chields) {
            delete item;
        }
    }
};

class sFileLoaderClipboard: public QObject{
    Q_OBJECT
protected:
    const int LIFE_TIME = 2880;//2 days
public:
    int                 clipboardOpenTimeOut; //0 - time to forget this clipboard
    StringUuid          handle;
    int                 totalSize;
    QMap<QString,sFileLoaderFileInfo*> rootFiles;
    QMap<StringUuid,sFileLoaderFileInfo*> allFiles;

    sFileLoaderClipboard(QStringList files);
    ~sFileLoaderClipboard(){
        foreach (auto item, rootFiles) {
            delete item;
        }
    }
};

class cFileLoader : public QObject
{
    Q_OBJECT
protected:
    const int GARBAGE_COLLECTOR_PERIOD = 60;//every minute
protected:
    QMap<StringUuid,sFileLoaderClipboard*> m_Clipboards;
    QTimer              m_Timer;
    sFileLoaderFileInfo* getOpenedFile(StringUuid clipboardUuid, StringUuid fileUuid);
public:
    explicit cFileLoader(QObject *parent = 0);
    ~cFileLoader();

    sFileLoaderClipboard* openClipboardFiles(QStringList& files);
    sFileLoaderFileInfo* openFile(StringUuid clipboardUuid, QString relativeFileName);
    sFileLoaderFileInfo* getFilePart(StringUuid clipboardUuid, StringUuid fileUuid, int start, int size, char *outData);
    void closeFile(StringUuid clipboardUuid, StringUuid fileUuid);
    void closeClipboard(StringUuid clipboardUuid);
signals:

protected slots:
    void collectGarbage();
public slots:
};

#endif // CFILESENDER_H

