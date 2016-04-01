#include "cfilesaver.h"
#include "cclipboardmanager.h"

cFileSaver::cFileSaver(cClipboardManager* parent):m_Parent(parent),m_FilesList(this)
{
    m_Downloading = false;
}

cFileSaverList *cFileSaver::initFilesDownloading(StringUuid listHandle)
{
    if (m_Downloading)
        return NULL;
    m_Downloading = true;
    m_FilesList.clear();
    m_FilesList.setHandle(listHandle);
    return &m_FilesList;
}

void cFileSaver::stopDownloading()
{
    m_Downloading = false;
    m_FilesList.clear();
    emit onStop();
}


cFileSaverFile::cFileSaverFile(int size, QString relativeFileName, bool isExecutable)
{
    m_Handle = "";
    m_Size = size;
    m_RelativeFileName = relativeFileName;
    m_IsExecutable = isExecutable;
    m_FileParts.clear();
}




void cFileSaverList::clear()
{
    for (int i = 0; i<m_Files.size(); ++i)
        delete m_Files[i];
    m_Files.clear();;
    m_Dirs.clear();
    m_TotalSize = 0;
    if (m_Handle!="")
        m_Parent->parent()->closeFilesList(m_Handle,m_HostAddress);
    m_Handle = "";
}

void cFileSaverList::startDownloading(QString localPath, QHostAddress host)
{
    m_HostAddress = host;
    localPath = localPath.replace("\\","/");
    while (localPath.indexOf("//")>-1)
        localPath = localPath.replace("//","/");

    if (m_Files.size()==1 && m_Dirs.size()==0){
        QFileInfo info(localPath);
        m_LocalPath = info.absolutePath();
        m_Files[0]->overrideFileName(info.fileName());
    }
    else
        m_LocalPath = localPath;

    if (m_LocalPath[m_LocalPath.size()-1]!='/')
        m_LocalPath += "/";

    bool AlwaysSkipNotAccessableFolders = false;
    m_AlwaysSkipErrorFiles = false;
    m_AlwaysOverwriteFiles = false;
    m_AlwaysSkipFolderFiles = false;


    QDir local(m_LocalPath);
    if (!local.exists()){
        QMessageBox::information(0,"",tr("Can't access destination folder"));
        m_Parent->stopDownloading();
        return;
    }

    for (int i = 0; i<m_Dirs.size(); i++){
        QFileInfo file(m_LocalPath+m_Dirs[i]);
        if (file.isFile() && file.exists()){
            if (AlwaysSkipNotAccessableFolders)
                continue;
            QMessageBox::StandardButton answer = QMessageBox::critical(0,"file exists",tr("file %1 already exists.\ncan't create folder %1. \nSkip?").arg(file.absoluteFilePath()),QMessageBox::YesAll | QMessageBox::Yes | QMessageBox::Abort);
            if (answer==QMessageBox::Abort){
                m_Parent->stopDownloading();
                return;
            }
            if (answer==QMessageBox::YesAll){
                AlwaysSkipNotAccessableFolders = true;
                continue;
            }
            continue;
        }

        local.mkpath(m_Dirs[i]);
    }

    processNextFilePart();
}

void cFileSaverList::fileOpenResult(int resultCode, StringUuid fileUuid, int fileSize, bool isExecutable, QString relativeFileName)
{
    int id = -1;
    for (int i = 0; i<m_Files.size(); ++i){
        if (m_Files[i]->m_RelativeFileName==relativeFileName){
            id = i;
            break;
        }
    }

    if (id==-1){
        qCritical() << "unknown file received "<< relativeFileName;
        return;
    }

    if (resultCode!=0){
        if (m_AlwaysSkipErrorFiles){
            FilesRemove(id);
            processNextFilePart();
            return;
        }

        QMessageBox::StandardButton answer = QMessageBox::critical(0,"remote file access failed",tr("can't access remote file %1\nSkip?").arg(relativeFileName),QMessageBox::YesAll | QMessageBox::Yes | QMessageBox::Retry | QMessageBox::Abort);
        if (answer==QMessageBox::Abort){
            m_Parent->stopDownloading();
            return;
        }
        if (answer==QMessageBox::Retry){
            processNextFilePart();
            return;
        }
        if (answer==QMessageBox::YesAll){
            m_AlwaysSkipErrorFiles = true;
        }
        FilesRemove(id);
        processNextFilePart();
        return;
    }

    cFileSaverFile* file = m_Files[0];
    file->m_Handle = fileUuid;
    file->m_Size = fileSize;
    file->m_IsExecutable = isExecutable;
    if (fileSize==0){
        QByteArray dummy;
        fileGetPart(0,fileUuid,0,0,dummy);
        return;
    }

    file->m_FileParts.resize(fileSize / MAX_FILE_PART_SIZE + 1);
    if (file->m_FileParts.size()==1){

        file->m_FileParts[0].data.clear();;
        file->m_FileParts[0].dataReceived = false;
        file->m_FileParts[0].size = fileSize;
        file->m_FileParts[0].startPos = 0;
    }
    else{
        int start = 0;
        for (int i = 0; i<file->m_FileParts.size(); ++i){
            file->m_FileParts[i].data.clear();;
            file->m_FileParts[i].dataReceived = false;
            file->m_FileParts[0].startPos = start;
            int partSize;
            if (start+MAX_FILE_PART_SIZE<fileSize){
                partSize = MAX_FILE_PART_SIZE;
            }
            else{
                partSize = fileSize-start;
            }
            file->m_FileParts[0].size = partSize;
            start+=partSize;
        }
    }
    processNextFilePart();
}

void cFileSaverList::fileGetPart(int resultCode, StringUuid fileUuid, int start, int size, QByteArray &data)
{
    int id = -1;
    for (int i = 0; i<m_Files.size(); ++i){
        if (m_Files[i]->m_Handle==fileUuid){
            id = i;
            break;
        }
    }

    if (id==-1){
        qCritical() << "unknown file received "<< fileUuid;
        return;
    }

    int part = -1;
    for (int i = 0; i<m_Files[id]->m_FileParts.size(); ++i){
        if (m_Files[id]->m_FileParts[i].startPos == start && m_Files[id]->m_FileParts[i].size == size){
            part = i;
            break;
        }
    }

    if (resultCode!=0 || part==-1 && data.size()!=size){
        if (m_AlwaysSkipErrorFiles){
            FilesRemove(id);
            processNextFilePart();
            return;
        }

        QMessageBox::StandardButton answer = QMessageBox::critical(0,"remote file access failed",tr("can't access remote file %1\nSkip?").arg(m_Files[id]->m_RelativeFileName),QMessageBox::YesAll | QMessageBox::Yes | QMessageBox::Retry | QMessageBox::Abort);
        if (answer==QMessageBox::Abort){
            m_Parent->stopDownloading();
            return;
        }
        if (answer==QMessageBox::Retry){
            m_Files[id]->m_Handle = "";
            processNextFilePart();
            return;
        }
        if (answer==QMessageBox::YesAll){
            m_AlwaysSkipErrorFiles = true;
        }
        FilesRemove(id);
        processNextFilePart();
        return;
    }

    cFileSaverFile* file = m_Files[id];
    file->m_FileParts[part].data.append(data.constData(),size);
    file->m_FileParts[part].dataReceived = true;
    for (int i = 0; i<file->m_FileParts.size(); ++i){
        if (!file->m_FileParts[i].dataReceived){
            processNextFilePart();
            return;
        }
    }

    QString FileName = m_LocalPath;
    if (file->m_OverridedFileName.isEmpty())
        FileName+=file->m_RelativeFileName;
    else
        FileName+=file->m_OverridedFileName;

    QFileInfo info(FileName);
    if (info.isDir()){
        if (m_AlwaysSkipFolderFiles){
            FilesRemove(id);
            processNextFilePart();
            return;
        }
        QMessageBox::StandardButton answer = QMessageBox::critical(0,"folder exists",tr("folder %1 already exists.\ncan't paste file over folder %1. \nSkip?").arg(FileName),QMessageBox::YesAll | QMessageBox::Yes | QMessageBox::Abort);
        if (answer==QMessageBox::Abort){
            m_Parent->stopDownloading();
            return;
        }
        if (answer==QMessageBox::YesAll){
            m_AlwaysSkipFolderFiles = true;
        }
        FilesRemove(id);
        processNextFilePart();
    }

    if (info.exists()){
        if (m_AlwaysOverwriteFiles){

        }
        else{
            QMessageBox::StandardButton answer = QMessageBox::critical(0,"file exists",tr("file %1 already exists.\nOverwrite?").arg(FileName),QMessageBox::YesAll | QMessageBox::Yes | QMessageBox::Ignore |QMessageBox::Abort);
            if (answer==QMessageBox::Abort){
                m_Parent->stopDownloading();
                return;
            }
            if (answer==QMessageBox::Ignore){
                FilesRemove(id);
                processNextFilePart();
                return;
            }
            if (answer==QMessageBox::YesAll){
                m_AlwaysOverwriteFiles = true;
            }
        }
        if (!QFile::remove(FileName)){
            if (QMessageBox::critical(0,"no access to file",tr("can't overwrite file %1").arg(FileName),QMessageBox::Ignore | QMessageBox::Abort)==QMessageBox::Abort){
                m_Parent->stopDownloading();
            }
            else{
                FilesRemove(id);
                processNextFilePart();
            }
            return;
        }
    }

    bool trySaveFile = true;
    while (trySaveFile){
        QFile outFile(FileName);
        if (!outFile.open(QIODevice::WriteOnly)){
            QFile::remove(FileName);
            QMessageBox::StandardButton answer = QMessageBox::critical(0,"file write error",tr("can't write file %1").arg(FileName),QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Abort);
            if (answer==QMessageBox::Retry){
                continue;
            }

            if (answer==QMessageBox::Abort){
                m_Parent->stopDownloading();
                return;
            }

            if (answer==QMessageBox::Ignore){
                FilesRemove(id);
                processNextFilePart();
                return;
            }
        }

        trySaveFile = false;
        for (int i = 0; i<file->m_FileParts.size(); ++i){
            if (!outFile.write(file->m_FileParts[i].data.constData(),file->m_FileParts[i].size)){
                outFile.close();
                QFile::remove(FileName);
                QMessageBox::StandardButton answer = QMessageBox::critical(0,"file write error",tr("can't write file %1").arg(FileName),QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Abort);
                if (answer==QMessageBox::Retry){
                    trySaveFile = true;
                    break;
                }

                if (answer==QMessageBox::Abort){
                    m_Parent->stopDownloading();
                    return;
                }

                if (answer==QMessageBox::Ignore){
                    FilesRemove(id);
                    processNextFilePart();
                    return;
                }
            }
        }

        QFileDevice::Permissions permissions = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::WriteGroup;
        if (file->m_IsExecutable)
            permissions = permissions | QFileDevice::ExeOwner | QFileDevice::ExeGroup;
        outFile.setPermissions(permissions);
        outFile.close();
    }
    FilesRemove(id);
    processNextFilePart();
}


void cFileSaverList::processNextFilePart()
{
    if (m_Files.size()==0){ //done
        m_Parent->stopDownloading();
        return;
    }

    cFileSaverFile* file = m_Files[0];
    if (file->m_Handle.isEmpty()){
        if (!m_Parent->parent()->openFile(m_Handle,file->m_RelativeFileName,m_HostAddress)){
            QMessageBox::StandardButton answer = QMessageBox::critical(0,"remote access failed",tr("can't open remote file %1").arg(file->m_RelativeFileName),QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Abort);
            if (answer==QMessageBox::Retry){
                processNextFilePart();
                return;
            }

            if (answer==QMessageBox::Ignore){
                FilesRemove(0);
                processNextFilePart();
                return;
            }

            if (answer==QMessageBox::Abort){
                m_Parent->stopDownloading();
                return;
            }
        }

        return;
    }

    for (int i = 0; i<file->m_FileParts.size(); ++i ){
        if (!file->m_FileParts[i].dataReceived){
            m_Parent->parent()->getFilePart(m_Handle,file->m_Handle,file->m_FileParts[i].startPos,file->m_FileParts[i].size,m_HostAddress);
            return;
        }
    };

    QMessageBox::critical(0,"unknown file error",tr("unknwon error while loading file %1").arg(file->m_RelativeFileName));
    //unknown error
    FilesRemove(0);
    processNextFilePart();
}

void cFileSaverList::FilesRemove(int id)
{
    delete m_Files[id];
    m_Files.remove(id);
}

cFileSaverList::cFileSaverList(cFileSaver *parent):m_Parent(parent)
{
}

cFileSaverList::~cFileSaverList()
{
    clear();
}

void cFileSaverList::addFile(int size, QString relativeFileName, bool isExecutable)
{
    cFileSaverFile* file = new cFileSaverFile(size,relativeFileName,isExecutable);
    m_TotalSize += size;
    m_Files.push_back(file);
}

void cFileSaverList::addDir(QString relativeFileName)
{
    m_Dirs.append(relativeFileName);
}
