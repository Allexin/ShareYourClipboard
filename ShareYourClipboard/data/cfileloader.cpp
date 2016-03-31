#include "cfileloader.h"

cFileLoader::cFileLoader(QObject *parent) : QObject(parent)
{
    connect(&m_Timer,SIGNAL(timeout()),this, SLOT(collectGarbage()));
    m_Timer.start(GARBAGE_COLLECTOR_PERIOD*1000);
}

cFileLoader::~cFileLoader()
{
    foreach (auto item, m_Clipboards)
        delete item;
}

sFileLoaderClipboard *cFileLoader::openClipboardFiles(QStringList &files)
{
    sFileLoaderClipboard* clipboard = new sFileLoaderClipboard(files);
    if (clipboard->allFiles.empty()){
        delete clipboard;
        return NULL;
    }
    m_Clipboards[clipboard->handle] = clipboard;
    return clipboard;
}

sFileLoaderFileInfo *cFileLoader::openFile(StringUuid clipboardUuid, QString relativeFileName)
{
    if (!m_Clipboards.contains(clipboardUuid))
        return NULL;

    sFileLoaderClipboard* clipboard = m_Clipboards.value(clipboardUuid);
    if (clipboard->rootFiles.contains(relativeFileName)){
        sFileLoaderFileInfo* file = clipboard->rootFiles[relativeFileName];
        if (file->open())
            return file;
        return NULL;
    }

    relativeFileName = relativeFileName.replace("\\","/");
    while (relativeFileName.indexOf("//")>-1)
        relativeFileName = relativeFileName.replace("//","/");

    QStringList tree = relativeFileName.split("/");

    if (!clipboard->rootFiles.contains(tree[0]))
        return NULL;
    sFileLoaderFileInfo* item =clipboard->rootFiles[tree[0]];
    if (!item->isDir)
        return NULL;
    for (int i  = 1; i<tree.size(); ++i){
        if (!item->chields.contains(tree[i]))
            return NULL;
        item = item->chields[tree[i]];
        if (!item->isDir && i<tree.size()-1)
            return NULL;
    }
    if (item->isDir)
        return NULL;
    if (!item->open())
        return NULL;
    return item;
}

sFileLoaderFileInfo *cFileLoader::getOpenedFile(StringUuid clipboardUuid, StringUuid fileUuid)
{
    if (!m_Clipboards.contains(clipboardUuid))
        return NULL;
    sFileLoaderClipboard* clipboard = m_Clipboards.value(clipboardUuid);
    if (!clipboard->allFiles.contains(fileUuid))
        return NULL;
    sFileLoaderFileInfo* file = clipboard->allFiles[fileUuid];
    if (file->fileOpenTimeOut<=0)
        return NULL;
    return file;
}

sFileLoaderFileInfo* cFileLoader::getFilePart(StringUuid clipboardUuid, StringUuid fileUuid, int start, int size, char *outData)
{
    sFileLoaderFileInfo * file = getOpenedFile(clipboardUuid,fileUuid);
    if (file==NULL)
        return NULL;

    return file->read(start,size,outData)?file:NULL;
}

void cFileLoader::closeFile(StringUuid clipboardUuid, StringUuid fileUuid)
{
    sFileLoaderFileInfo * file = getOpenedFile(clipboardUuid,fileUuid);
    if (file==NULL)
        return;
    file->close();
}

void cFileLoader::closeClipboard(StringUuid clipboardUuid)
{
    m_Clipboards.remove(clipboardUuid);
}

void cFileLoader::collectGarbage()
{
    for (auto it = m_Clipboards.begin(); it != m_Clipboards.end();){
        if (it.value()->clipboardOpenTimeOut-=GARBAGE_COLLECTOR_PERIOD <= 0){
            delete it.value();
            it = m_Clipboards.erase(it);
        }
        else{
            foreach (auto item, it.value()->allFiles) {
                if (item->fileOpenTimeOut>0){
                    if (item->fileOpenTimeOut-GARBAGE_COLLECTOR_PERIOD <=0){
                        item->close();
                    }
                    else
                        item->fileOpenTimeOut-=GARBAGE_COLLECTOR_PERIOD;
                }
            }
            ++it;
        }
    }

}


sFileLoaderClipboard::sFileLoaderClipboard(QStringList files):QObject(0)
{
    clipboardOpenTimeOut = LIFE_TIME;
    handle = QUuid::createUuid().toString();
    for (int i = 0; i<files.size(); i++){
        QFileInfo info(files[i]);
        if (!rootFiles.contains(info.fileName()))
            if (!info.isSymLink() && info.isReadable())
                rootFiles[info.fileName()] = new sFileLoaderFileInfo(this, info);
    }

    totalSize = 0;
    foreach(auto item, allFiles)
        if(!item->isDir)
            totalSize+=item->fileSize;
}

bool sFileLoaderFileInfo::open()
{
    if (fileOpenTimeOut>0){
        fileOpenTimeOut = LIFE_TIME;
        return true;
    }

    if (!file.open(QIODevice::ReadOnly))
        return false;
    if (file.isSequential() || !file.isReadable()){
        file.close();
        return false;
    }

    fileOpenTimeOut = LIFE_TIME;
    fileSize = file.size();
    return true;
}

void sFileLoaderFileInfo::close()
{
    if (fileOpenTimeOut<=0)
        return;
    file.close();
    fileOpenTimeOut = 0;
}

bool sFileLoaderFileInfo::read(int start, int size, char* outData)
{
    if (fileOpenTimeOut<=0)
        return false;
    if (start+size>file.size())
        return false;

    file.seek(start);
    return (file.read(outData,size)==size);
}

sFileLoaderFileInfo::sFileLoaderFileInfo(sFileLoaderClipboard *parent, QFileInfo &FileInfo):QObject(NULL)
{
    handle = QUuid::createUuid().toString();
    parent->allFiles[handle] = this;
    fileOpenTimeOut = 0;
    isDir = FileInfo.isDir();
    isExecutable = FileInfo.isExecutable();
    fileName = FileInfo.fileName();
    if (!isDir){
        file.setFileName(FileInfo.absoluteFilePath());
        fileSize = FileInfo.size();
    }
    else{
        fileSize = 0;
        QDir dir(FileInfo.absoluteFilePath());
        QFileInfoList files = dir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
        for (int i = 0; i<files.size(); ++i)
            chields[files[i].absoluteFilePath()] = new sFileLoaderFileInfo(parent,files[i]);
    }
}
