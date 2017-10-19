/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2017                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include "syncwidget.h"

#include "infowidget.h"

#include <openspace/openspace.h>

#include <ghoul/ghoul.h>
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/filesystem/file.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/lua/ghoul_lua.h>
#include <ghoul/lua/luastate.h>
#include <ghoul/misc/dictionary.h>
#include <ghoul/misc/onscopeexit.h>

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

#include <libtorrent/entry.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_info.hpp>

#include <fstream>
#include <mutex>

namespace {
    const std::string _loggerCat = "SyncWidget";

    const std::string _configurationFile = "Launcher.config";

    const int nColumns = 3;

    const int DownloadApplicationVersion = 1;

    const std::string FileDownloadKey = "FileDownload";
    const std::string FileRequestKey = "FileRequest";
    const std::string TorrentFilesKey = "TorrentFiles";

    const std::string UrlKey = "URL";
    const std::string FileKey = "File";
    const std::string DestinationKey = "Destination";
    const std::string IdentifierKey = "Identifier";
    const std::string VersionKey = "Version";

    const bool OverwriteFiles = false;
    const bool CleanInfoWidgets = true;
}

SyncWidget::SyncWidget(QWidget* parent, Qt::WindowFlags f) 
    : QWidget(parent, f)
    , _sceneLayout(nullptr)
    , _session(new libtorrent::session)
{
    setObjectName("SyncWidget");
    setFixedSize(500, 500);

    QBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    {
        QGroupBox* sceneBox = new QGroupBox;
        _sceneLayout = new QGridLayout;
        sceneBox->setLayout(_sceneLayout);
        layout->addWidget(sceneBox);
    }
    {
        QPushButton* syncButton = new QPushButton("Synchronize Data");
        syncButton->setObjectName("SyncButton");
        QObject::connect(
            syncButton, SIGNAL(clicked(bool)),
            this, SLOT(syncButtonPressed())
        );

        layout->addWidget(syncButton);
    }

    {
        QScrollArea* area = new QScrollArea;
        area->setWidgetResizable(true);

        QWidget* w = new QWidget;
        w->setObjectName("DownloadArea");
        area->setWidget(w);

        _downloadLayout = new QVBoxLayout(w);
        _downloadLayout->setMargin(0);
        _downloadLayout->setSpacing(0);
        _downloadLayout->addStretch(100);

        layout->addWidget(area);
    }
    QPushButton* close = new QPushButton("Close");
    layout->addWidget(close, Qt::AlignRight);
    QObject::connect(
        close, SIGNAL(clicked(bool)),
        this, SLOT(close())
    );

    setLayout(layout);

    ghoul::initialize();
    _downloadManager = std::make_unique<openspace::DownloadManager>(
        "http://data.openspaceproject.com/request", DownloadApplicationVersion);

    libtorrent::error_code ec;
    _session->listen_on(std::make_pair(20280, 20290), ec);

    libtorrent::settings_pack settings;
    settings.set_str(libtorrent::settings_pack::user_agent, "OpenSpace/" +
        std::to_string(openspace::OPENSPACE_VERSION_MAJOR) + "." +
        std::to_string(openspace::OPENSPACE_VERSION_MINOR) + "." +
        std::to_string(openspace::OPENSPACE_VERSION_PATCH));
    settings.set_bool(libtorrent::settings_pack::allow_multiple_connections_per_ip, true);
    settings.set_bool(libtorrent::settings_pack::ignore_limits_on_local_network, true);
    settings.set_int(libtorrent::settings_pack::connection_speed, 20);
    settings.set_int(libtorrent::settings_pack::active_downloads, -1);
    settings.set_int(libtorrent::settings_pack::active_seeds, -1);
    settings.set_int(libtorrent::settings_pack::active_limit, 30);
    settings.set_int(libtorrent::settings_pack::dht_announce_interval, 60);
    _session->apply_settings(settings);

    if (ec) {
        LFATAL("Failed to open socket: " << ec.message());
        return;
    }
    _session->start_upnp();

    std::ifstream file(_configurationFile);
    if (!file.fail()) {
        union {
            uint32_t value;
            std::array<char, 4> data;
        } size;

        file.read(size.data.data(), sizeof(uint32_t));
        std::vector<char> buffer(size.value);
        file.read(buffer.data(), size.value);
        file.close();

        libtorrent::entry e = libtorrent::bdecode(buffer.begin(), buffer.end());
        _session->start_dht(e);
    }
    else 
        _session->start_dht();

    _session->add_dht_router({ "router.utorrent.com", 6881 });
    _session->add_dht_router({ "dht.transmissionbt.com", 6881 });
    _session->add_dht_router({ "router.bittorrent.com", 6881 });
    _session->add_dht_router({ "router.bitcomet.com", 6881 });



    QTimer* timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(handleTimer()));
    timer->start(100);

    _mutex.clear();

    _scriptEngine.initialize();
}

SyncWidget::~SyncWidget() {
    libtorrent::entry dht = _session->dht_state();

    std::vector<char> buffer;
    libtorrent::bencode(std::back_inserter(buffer), dht);

    std::ofstream f(_configurationFile);

    union {
        uint32_t value;
        std::array<char, 4> data;
    } size;
    size.value = buffer.size();
    f.write(size.data.data(), sizeof(uint32_t));
    f.write(buffer.data(), buffer.size());

    _downloadManager.reset();
    ghoul::deinitialize();
    delete _session;
}

void SyncWidget::closeEvent(QCloseEvent* event) {
    std::vector<libtorrent::torrent_handle> handles = _session->get_torrents();
    for (libtorrent::torrent_handle h : handles) {
        h.flush_cache();
        _session->remove_torrent(h);
    }
}

void SyncWidget::setSceneFiles(QMap<QString, QString> sceneFiles) {
    _sceneFiles = std::move(sceneFiles);
    QStringList keys = _sceneFiles.keys();
    for (int i = 0; i < keys.size(); ++i) {
        const QString& sceneName = keys[i];

        QCheckBox* checkbox = new QCheckBox(sceneName);
        QString defaultName = "default";
        if (sceneName == defaultName){
            checkbox->setChecked(true);
        }
        _sceneLayout->addWidget(checkbox, i / nColumns, i % nColumns);
    }
}

void SyncWidget::clear() {
    for (std::shared_ptr<openspace::DownloadManager::FileFuture> f : _futures)
        f->abortDownload = true;

    using libtorrent::torrent_handle;
    for (QMap<torrent_handle, InfoWidget*>::iterator i = _torrentInfoWidgetMap.begin();
         i != _torrentInfoWidgetMap.end();
         ++i)
    {
        delete i.value();
    }
    _torrentInfoWidgetMap.clear();
    _session->abort();


    _directFiles.clear();
    _fileRequests.clear();
    _torrentFiles.clear();
}

void SyncWidget::handleDirectFiles() {
    LDEBUG("Direct Files");
    for (const DirectFile& f : _directFiles) {
        LDEBUG(f.url.toStdString() << " -> " << f.destination.toStdString());

        std::shared_ptr<openspace::DownloadManager::FileFuture> future = _downloadManager->downloadFile(
            f.url.toStdString(),
            absPath(f.destination.toStdString()),
            OverwriteFiles
        );
        if (future) {
            InfoWidget* w = new InfoWidget(f.destination);
            _downloadLayout->insertWidget(_downloadLayout->count() - 1, w);

            _futures.push_back(future);
            _futureInfoWidgetMap[future] = w;
        }
    }
}

void SyncWidget::handleFileRequest() {
    LDEBUG("File Requests");
        
    for (const FileRequest& f : _fileRequests) {
        LDEBUG(f.identifier.toStdString() << " (" << f.version << ") -> " << f.destination.toStdString()); 

        ghoul::filesystem::Directory d = FileSys.currentDirectory();
        OnExit([&]() { FileSys.setCurrentDirectory(d); });
        FileSys.setCurrentDirectory(f.baseDir.toStdString());

        std::string identifier =  f.identifier.toStdString();
        std::string path = absPath(f.destination.toStdString());
        int version = f.version;

        std::string requestId = path + "$" + identifier;


        std::lock_guard<std::mutex> g(_filesDownloadingMutex);
        if (_filesDownloading.find(requestId) != _filesDownloading.end()) {
            continue; // The file is already being downloaded.
        }
        _filesDownloading.insert(requestId);

        _downloadManager->downloadRequestFilesAsync(
            identifier,
            path,
            version,
            OverwriteFiles,
            [this, requestId](const std::vector<std::shared_ptr<openspace::DownloadManager::FileFuture>>& futures) {
                handleFileFutureAddition(futures);
                std::lock_guard<std::mutex> g(_filesDownloadingMutex);
                _filesDownloading.erase(requestId);
            }
        );
    }
}

void SyncWidget::handleTorrentFiles() {
    LDEBUG("Torrent Files");
    for (const TorrentFile& f : _torrentFiles) {
        LDEBUG(f.file.toStdString() << " -> " << f.destination.toStdString());

        ghoul::filesystem::Directory d = FileSys.currentDirectory();
//        std::string thisDirectory = absPath("${SCENE}/" + f.module.toStdString() + "/");
        FileSys.setCurrentDirectory(f.baseDir.toStdString());
//        FileSys.setCurrentDirectory(thisDirectory);

        QString file = QString::fromStdString(absPath(f.file.toStdString()));

        if (!QFileInfo(file).exists()) {
            LERROR(file.toStdString() << " does not exist");
            continue;
        }

        libtorrent::error_code ec;
        libtorrent::add_torrent_params p;

        //if (f.destination.isEmpty())
            //p.save_path = absPath(fullPath(f.module, ".").toStdString());
        //else
            //p.save_path = 
        p.save_path = absPath(f.destination.toStdString());

        p.ti = std::make_shared<libtorrent::torrent_info>(file.toStdString(), ec);
        p.name = f.file.toStdString();
        p.storage_mode = libtorrent::storage_mode_allocate;
        if (ec) {
            LERROR(f.file.toStdString() << ": " << ec.message());
            continue;
        }
        libtorrent::torrent_handle h = _session->add_torrent(p, ec);
        if (ec) {
            LERROR(f.file.toStdString() << ": " << ec.message());
            continue;
        }

        if (_torrentInfoWidgetMap.find(h) == _torrentInfoWidgetMap.end()) {
            QString fileString = f.file;
            QString t = QString(".torrent");
            fileString.replace(fileString.indexOf(t), t.size(), "");

            fileString = f.module + "/" + fileString;

            InfoWidget* w = new InfoWidget(fileString, h.status().total_wanted);
            _downloadLayout->insertWidget(_downloadLayout->count() - 1, w);
            _torrentInfoWidgetMap[h] = w;
        }

        FileSys.setCurrentDirectory(d);
    }
}

void SyncWidget::syncButtonPressed() {
    clear();

    ghoul::lua::LuaState state;
    _scriptEngine.initializeLuaState(state);

    for (const QString& scene : selectedScenes()) {
        LDEBUG(scene.toStdString());
        ghoul::Dictionary sceneDictionary;

        try {
            ghoul::lua::loadDictionaryFromFile(
                scene.toStdString(),
                sceneDictionary,
                state
            );
        } catch (const ghoul::lua::LuaRuntimeException& e) {
            LERROR(e.message);
            return;
        }

        ghoul::Dictionary modules;
        bool success = sceneDictionary.getValue<ghoul::Dictionary>("Assets", modules);
        if (!success) {
            LERROR("Could not find 'Modules'");
            return;
        }

        QDir sceneDir(scene);
        sceneDir.cdUp();
        QList<ModuleInformation> modulesList;
        for (int i = 1; i <= modules.size(); ++i) {
            std::string module = modules.value<std::string>(std::to_string(i));
            std::string shortModule = module;
            
            std::string::size_type pos = module.find_last_of(FileSys.PathSeparator);
            if (pos != std::string::npos) {
                shortModule = module.substr(pos+1);
            }
            
            QString m = QString::fromStdString(module);
            
            QString dataFile = sceneDir.absoluteFilePath(
                QString::fromStdString(module) + "/" + QString::fromStdString(shortModule) + ".data"
            );
            
            if (QFileInfo(dataFile).exists()) {
                modulesList.append({
                    QString::fromStdString(module),
                    dataFile,
                    sceneDir.absolutePath() + "/" + QString::fromStdString(module)
                });
            }
            else {
                QDir metaModuleDir = sceneDir;
                metaModuleDir.cd(QString::fromStdString(module));
                
                QDirIterator it(metaModuleDir.absolutePath(), QStringList() << "*.data", QDir::Files, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    QString v = it.next();
                    QDir d(v);
                    d.cdUp();
                    
                    modulesList.append({
                        d.dirName(),
                        v,
                        d.absolutePath()
                    });
                }
            }
        }
        modulesList.append({
            "common",
            sceneDir.absolutePath() + "/common/common.data",
            sceneDir.absolutePath() + "/common"
        });

        for (const ModuleInformation& module : modulesList) {
            QString dataFile = module.moduleDatafile;
//            QString dataFile = sceneDir.absoluteFilePath(module + "/" + module + ".data");

            if (QFileInfo(dataFile).exists()) {
                ghoul::Dictionary dataDictionary;
                ghoul::lua::loadDictionaryFromFile(dataFile.toStdString(), dataDictionary);

                ghoul::Dictionary directDownloadFiles;
                ghoul::Dictionary fileRequests;
                ghoul::Dictionary torrentFiles;

                bool found = dataDictionary.getValue<ghoul::Dictionary>(FileDownloadKey, directDownloadFiles);
                if (found) {
                    for (int i = 1; i <= directDownloadFiles.size(); ++i) {
                       if (!directDownloadFiles.hasKeyAndValue<ghoul::Dictionary>(std::to_string(i))) {
                           LERROR(dataFile.toStdString() << ": " << FileDownloadKey << " is not a dictionary");
                           continue;
                        }
                        ghoul::Dictionary d = directDownloadFiles.value<ghoul::Dictionary>(std::to_string(i));
                        if (!d.hasKeyAndValue<std::string>(UrlKey)) {
                            LERROR(dataFile.toStdString() << ": No " << UrlKey);
                            continue;
                        }
                        std::string url = d.value<std::string>(UrlKey);

                        std::string dest = "";
                        if (d.hasKeyAndValue<std::string>(DestinationKey))
                            dest = d.value<std::string>(DestinationKey);

                        _directFiles.append({
                            module.moduleName,
                            QString::fromStdString(url),
                            "${SYNC}/" + module.moduleName + "/" + QString::fromStdString(dest),
                            module.modulePath
                        });
                    }
                }

                found = dataDictionary.getValue<ghoul::Dictionary>(FileRequestKey, fileRequests);
                if (found) {
                    for (int i = 1; i <= fileRequests.size(); ++i) {
                        ghoul::Dictionary d = fileRequests.value<ghoul::Dictionary>(std::to_string(i));

                        if (!d.hasKeyAndValue<std::string>(IdentifierKey)) {
                            LERROR(dataFile.toStdString() << ": No " << IdentifierKey);
                            continue;
                        }
                        std::string url = d.value<std::string>(IdentifierKey);

                        std::string dest = "";
                        if (d.hasKeyAndValue<std::string>(DestinationKey))
                            dest = d.value<std::string>(DestinationKey);

                        if (!d.hasKeyAndValue<double>(VersionKey)) {
                            LERROR(dataFile.toStdString() << ": No " << VersionKey);
                            continue;
                        }
                        int version = static_cast<int>(d.value<double>(VersionKey));

                        _fileRequests.append({
                            module.moduleName,
                            QString::fromStdString(url),
                            "${SYNC}/" + module.moduleName + "/" + QString::fromStdString(dest),
                            module.modulePath,
                            version
                        });
                    }
                }

                found = dataDictionary.getValue<ghoul::Dictionary>(TorrentFilesKey, torrentFiles);
                if (found) {
                    for (int i = 1; i <= torrentFiles.size(); ++i) {
                        ghoul::Dictionary d = torrentFiles.value<ghoul::Dictionary>(std::to_string(i));

                        if (!d.hasKeyAndValue<std::string>(FileKey)) {
                            LERROR(dataFile.toStdString() << ": No " << FileKey);
                            continue;
                        }
                        std::string file = d.value<std::string>(FileKey);
                        
                        std::string dest;
                        if (d.hasKeyAndValue<std::string>(DestinationKey))
                            dest = d.value<std::string>(DestinationKey);
                        else
                            dest = "";

                        _torrentFiles.append({
                            module.moduleName,
                            QString::fromStdString(file),
                            "${SYNC}/" + module.moduleName + "/" + QString::fromStdString(dest),
                            module.modulePath
                        });
                    }
                }
            }
        }
    }

    // Remove duplicates for file requests
    std::map<QString, FileRequest> uniqueFileRequests;
    for (const FileRequest& f : _fileRequests) {
        uniqueFileRequests.emplace(std::make_pair(f.baseDir + "/" + f.destination, f));
    }
    _fileRequests.clear();
    for (const auto& f : uniqueFileRequests) {
        _fileRequests.append(f.second);
    }

    // Remove duplicates for direct files
    std::map<QString, DirectFile> uniqueDirectFiles;
    for (const DirectFile& f : _directFiles) {
        uniqueDirectFiles.emplace(std::make_pair(f.destination, f));
    }
    _directFiles.clear();
    for (const auto& f : uniqueDirectFiles) {
        _directFiles.append(f.second);
    }

    // Remove duplicates for torrents
    std::map<QString, TorrentFile> uniqueTorrentFiles;
    for (const TorrentFile& f : _torrentFiles) {
        uniqueTorrentFiles.emplace(std::make_pair(f.destination, f));
    }
    _directFiles.clear();
    for (const auto& f : uniqueTorrentFiles) {
        _torrentFiles.append(f.second);
    }

    handleDirectFiles();
    handleFileRequest();
    handleTorrentFiles();
}

QStringList SyncWidget::selectedScenes() const {
    QStringList result;
    int nChildren = _sceneLayout->count();
    for (int i = 0; i < nChildren; ++i) {
        QWidget* w = _sceneLayout->itemAt(i)->widget();
        QCheckBox* c = static_cast<QCheckBox*>(w);
        if (c->isChecked()) {
            QString t = c->text();
            result.append(_sceneFiles[t]);
        }
    }
    std::string scenes;
    for (QString s : result)
        scenes += s.toStdString() + "; ";
    LDEBUG("Downloading scenes: " << scenes);
    return result;
}

void SyncWidget::handleTimer() {
    using namespace libtorrent;
    using FileFuture = openspace::DownloadManager::FileFuture;

    std::vector<std::shared_ptr<FileFuture>> toRemove;
    for (std::shared_ptr<FileFuture> f : _futures) {
        InfoWidget* w = _futureInfoWidgetMap[f];

        if (CleanInfoWidgets && (f->isFinished || f->isAborted)) {
            toRemove.push_back(f);
            _downloadLayout->removeWidget(w);
            _futureInfoWidgetMap.erase(f);
            delete w;
        }
        else
            w->update(f);
    }

    for (std::shared_ptr<FileFuture> f : toRemove) {
        _futures.erase(std::remove(_futures.begin(), _futures.end(), f), _futures.end()); 
    }

    while (_mutex.test_and_set()) {}
    for (std::shared_ptr<FileFuture> f : _futuresToAdd) {
        InfoWidget* w = new InfoWidget(QString::fromStdString(f->filePath), -1);
        _downloadLayout->insertWidget(_downloadLayout->count() - 1, w);

        _futureInfoWidgetMap[f] = w;
        _futures.push_back(f);
    }
    _futuresToAdd.clear();
    _mutex.clear();


    std::vector<torrent_handle> handles = _session->get_torrents();
    for (torrent_handle h : handles) {
        torrent_status s = h.status();
        InfoWidget* w = _torrentInfoWidgetMap[h];

        if (w)
            w->update(s);

        if (CleanInfoWidgets && (s.state == torrent_status::finished || s.state == torrent_status::seeding)) {
            _torrentInfoWidgetMap.remove(h);
            delete w;
        }
    }

    // Only close every torrent if all torrents are finished
    bool allSeeding = true;
    for (torrent_handle h : handles) {
        torrent_status s = h.status();
        allSeeding &= (s.state == torrent_status::seeding);
    }

    if (allSeeding) {
        for (torrent_handle h : handles)
            _session->remove_torrent(h);
    }




    //_session->post_torrent_updates();
    //libtorrent::session_settings settings = _session->settings();

    //qDebug() << "Session";
    //qDebug() << "nPeers: " << _session->status().num_peers;
    //qDebug() << "DHT: " << _session->is_dht_running();
    //qDebug() << "Incoming TCP" << settings.enable_incoming_tcp;
    //qDebug() << "Outgoing TCP" << settings.enable_outgoing_tcp;
    //qDebug() << "Incoming UTP" << settings.enable_incoming_utp;
    //qDebug() << "Outgoing UTP" << settings.enable_outgoing_utp;
    //qDebug() << "===";

    //qDebug() << "Alerts";
    //std::deque<alert*> alerts;
    //_session->pop_alerts(&alerts);
    //for (alert* a : alerts) {
    //    qDebug() << QString::fromStdString(a->message());

    //    //if (a->category() == alert::status_notification) {
    //    //    state_update_alert* sua = static_cast<state_update_alert*>(a);
    //    //    for (torrent_status s )
    //    //}
    //}
    //qDebug() << "===";


    //    qDebug() << "Name: " << QString::fromStdString(h.name());
    //    //torrent_status s = h.status();

    //    qDebug() << "Error: " << QString::fromStdString(s.error);

    //    qDebug() << "Total Wanted: " << s.total_wanted;
    //    qDebug() << "Total Wanted Done: " << s.total_wanted_done;
    //    qDebug() << "Has Incoming: " << s.has_incoming;
    //    qDebug() << "Connect Candidates: " << s.connect_candidates;
    //    qDebug() << "Last Seen Complete: " << s.last_seen_complete;
    //    qDebug() << "List Peers: " << s.list_peers;
    //    qDebug() << "List Seeds: " << s.list_seeds;
    //    qDebug() << "Num Pieces: " << s.num_pieces;
    //    qDebug() << "Download Rate: " << s.download_rate;
    //    qDebug() << "List Seeds: " << s.list_seeds;
    //    qDebug() << "Paused: " << s.paused;
    //    qDebug() << "Progress: " << s.progress;

    //    qDebug() << "";
}

void SyncWidget::handleFileFutureAddition(
    const std::vector<std::shared_ptr<openspace::DownloadManager::FileFuture>>& futures)
{
    while (_mutex.test_and_set()) {}
    _futuresToAdd.insert(_futuresToAdd.end(), futures.begin(), futures.end());
    _mutex.clear();
}

