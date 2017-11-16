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

#include <openspace/scene/assetsynchronizer.h>

#include <algorithm>
#include <memory>
#include <numeric>

namespace openspace {
AssetSynchronizer::AssetSynchronizer() {}

void AssetSynchronizer::startSync(std::shared_ptr<Asset> asset) {
    std::vector<std::shared_ptr<Asset>> assets = asset->allAssets();
    for (const auto& a : assets) {
        std::vector<std::shared_ptr<ResourceSynchronization>> syncs =
            a->synchronizations();

        bool startedAnySync = false;
        for (const auto& s : syncs) {
            if (!s->isResolved()) {
                startedAnySync = true;
                startAssetResourceSync(a, s);
            }
        }
        if (!startedAnySync) {
            setState(a, SynchronizationState::Resolved);
        }
    }
}

void AssetSynchronizer::cancelSync(std::shared_ptr<Asset> asset) {
    std::vector<std::shared_ptr<Asset>> assets = asset->allAssets();
    for (const auto& a : assets) {
        std::vector<std::shared_ptr<ResourceSynchronization>> syncs =
            a->synchronizations();

        bool cancelledAnySync = false;
        for (const auto& s : syncs) {
            if (s->isSyncing()) {
                cancelledAnySync = true;
                cancelAssetResourceSync(a, s);
            }
        }
        if (cancelledAnySync) {
            setState(a, SynchronizationState::Unsynced);
        }
    }
}

void AssetSynchronizer::restartSync(std::shared_ptr<Asset> asset) {
    cancelSync(asset);
    startSync(asset);
}


float AssetSynchronizer::assetProgress(Asset* asset) {
    std::vector<std::shared_ptr<Asset>> assets = asset->allAssets();

    size_t nTotalBytes = 0;
    size_t nSyncedBytes = 0;

    for (const auto& a : assets) {
        const std::vector<std::shared_ptr<ResourceSynchronization>> syncs =
            asset->synchronizations();

        for (const auto& sync : syncs) {
            if (sync->nTotalBytesIsKnown()) {
                nTotalBytes += sync->nTotalBytes();
                nSyncedBytes += sync->nSynchronizedBytes();
            } else {
                return 0;
            }
        }
    }
    if (nTotalBytes == 0) {
        return 1.f;
    }
    
    return static_cast<float>(nSyncedBytes)/static_cast<float>(nTotalBytes);
}


std::vector<AssetSynchronizer::StateChange> AssetSynchronizer::getStateChanges() {
    std::vector<StateChange> stateChangesVector;
    for (auto& s : _stateChanges) {
        stateChangesVector.push_back(std::move(s.second));
    }
    _stateChanges.clear();
    return stateChangesVector;
}
    
AssetSynchronizer::SynchronizationState AssetSynchronizer::assetState(Asset*) {
    return SynchronizationState::Unsynced;
}
    
void AssetSynchronizer::startAssetResourceSync(
    std::shared_ptr<Asset> asset,
    std::shared_ptr<ResourceSynchronization> sync)
{
    _stateChanges[asset.get()] = StateChange{asset, SynchronizationState::Synchronizing};
    _resourceToAssetMap[sync.get()].insert(asset.get());
    
    ResourceSynchronization::CallbackHandle h =
        sync->addStateChangeCallback([this, asset, sync](ResourceSynchronization::State state) {
            handleSyncStateChange(asset, sync, state);
        });
}

void AssetSynchronizer::cancelAssetResourceSync(
    std::shared_ptr<Asset> asset,
    std::shared_ptr<ResourceSynchronization> sync)
{
    // Todo: implement this
}

    
void AssetSynchronizer::setState(
    std::shared_ptr<Asset> asset,
    AssetSynchronizer::SynchronizationState state)
{
    _stateChanges[asset.get()] = StateChange{asset, state};
}
    
void AssetSynchronizer::handleSyncStateChange(
    std::shared_ptr<Asset> asset,
    std::shared_ptr<ResourceSynchronization> sync,
    ResourceSynchronization::State state)
{
    if (state == ResourceSynchronization::State::Resolved) {
        _resourceToAssetMap[sync.get()].erase(asset.get());
        if (_resourceToAssetMap[sync.get()].empty()) {
//            _stateChanges[asset.get()] =
        }
    }
}

/*
bool AssetSynchronizer::assetIsSynchronized(Asset * asset) {
    std::vector<std::shared_ptr<ResourceSynchronization>> syncs = asset->synchronizations();
    for (const auto& s : syncs) {
        if (!s->isResolved()) {
            return false;
        }
    }
    return true;
}*/

}
