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

#include <modules/sync/syncmodule.h>

#include <modules/sync/syncs/httpsynchronization.h>
#include <modules/sync/syncs/torrentsynchronization.h>
#include <modules/sync/tasks/syncassettask.h>

#include <openspace/documentation/documentation.h>
#include <openspace/engine/configurationmanager.h>
#include <openspace/engine/openspaceengine.h>
#include <openspace/rendering/renderable.h>
#include <openspace/rendering/screenspacerenderable.h>
#include <openspace/util/factorymanager.h>
#include <openspace/util/resourcesynchronization.h>

#include <ghoul/filesystem/filesystem.h>
#include <ghoul/misc/assert.h>
#include <ghoul/misc/dictionary.h>

namespace {
    constexpr const char* KeyHttpSynchronizationRepositories =
        "HttpSynchronizationRepositories";
    constexpr const char* KeySynchronizationRoot = "SynchronizationRoot";

    const char* _loggerCat = "SyncModule";
}

namespace openspace {

SyncModule::SyncModule() : OpenSpaceModule(Name) {}

void SyncModule::internalInitialize(const ghoul::Dictionary& configuration) {
    if (configuration.hasKey(KeyHttpSynchronizationRepositories)) {
        ghoul::Dictionary dictionary = configuration.value<ghoul::Dictionary>(
            KeyHttpSynchronizationRepositories);

        for (const std::string& key : dictionary.keys()) {
            _httpSynchronizationRepositories.push_back(
                dictionary.value<std::string>(key)
            );
        }
    }

    if (configuration.hasKey(KeySynchronizationRoot)) {
        _synchronizationRoot = configuration.value<std::string>(KeySynchronizationRoot);
    } else {
        LWARNING(
            "No synchronization root specified."
            "Resource synchronization will be disabled."
        );
        //_synchronizationEnabled = false;
        // TODO: Make it possible to disable synchronization manyally.
        // Group root and enabled into a sync config object that can be passed to syncs.
    }

    auto fSynchronization = FactoryManager::ref().factory<ResourceSynchronization>();
    ghoul_assert(fSynchronization, "ResourceSynchronization factory was not created");

    fSynchronization->registerClass(
        "HttpSynchronization",
        [this](bool, const ghoul::Dictionary& dictionary) {
            return new HttpSynchronization(
                dictionary,
                _synchronizationRoot,
                _httpSynchronizationRepositories
            );
        }
    );

    fSynchronization->registerClass(
        "TorrentSynchronization",
        [this](bool, const ghoul::Dictionary& dictionary) {
            return new TorrentSynchronization(
                dictionary,
                _synchronizationRoot,
                &_torrentClient
            );
        }
    );

    auto fTask = FactoryManager::ref().factory<Task>();
    ghoul_assert(fTask, "No task factory existed");
    fTask->registerClass<SyncAssetTask>("SyncAssetTask");

    _torrentClient.initialize();
}

std::vector<documentation::Documentation> SyncModule::documentations() const {
    return {
        HttpSynchronization::Documentation(),
        TorrentSynchronization::Documentation()
    };
}

std::string SyncModule::synchronizationRoot() const {
    return _synchronizationRoot;
}

void SyncModule::addHttpSynchronizationRepository(const std::string& repository) {
    _httpSynchronizationRepositories.push_back(repository);
}

std::vector<std::string> SyncModule::httpSynchronizationRepositories() const {
    return _httpSynchronizationRepositories;
}

TorrentClient* SyncModule::torrentClient() {
    return &_torrentClient;
}

} // namespace openspace
