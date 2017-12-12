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

#include <openspace/scene/assetloader.h>

#include <openspace/scene/asset.h>
#include <openspace/scripting/script_helper.h>

#include <ghoul/lua/ghoul_lua.h>
#include <ghoul/lua/luastate.h>
#include <ghoul/lua/lua_helper.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/misc/onscopeexit.h>
#include <ghoul/filesystem/filesystem.h>

#include "assetloader_lua.inl"

namespace {
    const char* AssetGlobalVariableName = "asset";

    const char* RequireFunctionName = "require";
    const char* RequestFunctionName = "request";
    const char* ExportFunctionName = "export";

    const char* SyncedResourceFunctionName = "syncedResource";
    const char* LocalResourceFunctionName = "localResource";

    const char* OnInitializeFunctionName = "onInitialize";
    const char* OnDeinitializeFunctionName = "onDeinitialize";

    const char* ExportsTableName = "_exports";
    const char* AssetTableName = "_asset";
    const char* DependantsTableName = "_dependants";

    const char* _loggerCat = "AssetLoader";

    const char* AssetFileSuffix = "asset";

    bool isRelative(std::string path) {
        if (path.size() > 2) {
            if (path[0] == '.' && path[1] == '/') return true;
        }
        if (path.size() > 3) {
            if (path[0] == '.' && path[1] == '.' && path[2] == '/') return true;
        }
        return false;
    };
}

namespace openspace {

AssetLoader::AssetLoader(
    ghoul::lua::LuaState& luaState,
    SynchronizationWatcher* syncWatcher,
    std::string assetRootDirectory
)
    : _luaState(&luaState)
    , _rootAsset(std::make_shared<Asset>(this, syncWatcher))
    , _synchronizationWatcher(syncWatcher)
    , _assetRootDirectory(assetRootDirectory)
{
    setCurrentAsset(_rootAsset);

    // Create _assets table.
    lua_newtable(*_luaState);
    _assetsTableRef = luaL_ref(*_luaState, LUA_REGISTRYINDEX);
}

AssetLoader::~AssetLoader() {
    _currentAsset = nullptr;
    _rootAsset = nullptr;
}

void AssetLoader::trackAsset(std::shared_ptr<Asset> asset) {
    _loadedAssets.emplace(asset->id(), asset);
}

void AssetLoader::untrackAsset(Asset* asset) {
    auto it = _loadedAssets.find(asset->id());
    if (it != _loadedAssets.end()) {
        _loadedAssets.erase(it);
    }
}

void AssetLoader::setUpAssetLuaTable(Asset* asset) {
    /*
    Set up lua table:
    AssetMeta
    |- Exports (table<name, exported data>)
    |- Asset
    |  |- localResource
    |  |- syncedResource
    |  |- require
    |  |- request
    |  |- export
    |  |- onInitialize
    |  |- onDeinitialize
    |- Dependants (table<dependant, Dependency dep>)

    where Dependency is a table:
    Dependency
    |- onInitialize
    |- onDeinitialize
    */

    // Push the global assets table to the lua stack.
    lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, _assetsTableRef);
    int globalTableIndex = lua_gettop(*_luaState);


    // Create meta table for the current asset.
    lua_newtable(*_luaState);
    int assetMetaTableIndex = lua_gettop(*_luaState);

    // Register empty exports table on current asset.
    // (string => exported object)
    lua_newtable(*_luaState);
    lua_setfield(*_luaState, assetMetaTableIndex, ExportsTableName);

    // Create asset table
    lua_newtable(*_luaState);
    int assetTableIndex = lua_gettop(*_luaState);

    // Register local resource function
    // string localResource(string path)
    lua_pushlightuserdata(*_luaState, asset);
    lua_pushcclosure(*_luaState, &assetloader::localResource, 1);
    lua_setfield(*_luaState, assetTableIndex, LocalResourceFunctionName);

    // Register synced resource function
    // string syncedResource(string path)
    lua_pushlightuserdata(*_luaState, asset);
    lua_pushcclosure(*_luaState, &assetloader::syncedResource, 1);
    lua_setfield(*_luaState, assetTableIndex, SyncedResourceFunctionName);

    // Register require function
    // Asset, Dependency require(string path)
    lua_pushlightuserdata(*_luaState, asset);
    lua_pushcclosure(*_luaState, &assetloader::require, 1);
    lua_setfield(*_luaState, assetTableIndex, RequireFunctionName);

    // Register request function
    // Dependency request(string path)
    lua_pushlightuserdata(*_luaState, asset);
    lua_pushcclosure(*_luaState, &assetloader::request, 1);
    lua_setfield(*_luaState, assetTableIndex, RequestFunctionName);


    // Register export-dependency function
    // export(string key, any value)
    lua_pushlightuserdata(*_luaState, asset);
    lua_pushcclosure(*_luaState, &assetloader::exportAsset, 1);
    lua_setfield(*_luaState, assetTableIndex, ExportFunctionName);

    // Register onInitialize function
    // void onInitialize(function<void()> initializationFunction)
    lua_pushlightuserdata(*_luaState, asset);
    lua_pushcclosure(*_luaState, &assetloader::onInitialize, 1);
    lua_setfield(*_luaState, assetTableIndex, OnInitializeFunctionName);

    // Register onDeinitialize function
    // void onDeinitialize(function<void()> deinitializationFunction)
    lua_pushlightuserdata(*_luaState, asset);
    lua_pushcclosure(*_luaState, &assetloader::onDeinitialize, 1);
    lua_setfield(*_luaState, assetTableIndex, OnDeinitializeFunctionName);

    // Attach asset table to asset meta table
    lua_setfield(*_luaState, assetMetaTableIndex, AssetTableName);

    // Register empty dependant table on asset metatable.
    // (importer => dependant object)
    lua_newtable(*_luaState);
    lua_setfield(*_luaState, assetMetaTableIndex, DependantsTableName);

    // Extend global asset table (pushed to the lua stack earlier) with this asset meta table 
    lua_setfield(*_luaState, globalTableIndex, asset->id().c_str());
}

void AssetLoader::tearDownAssetLuaTable(Asset* asset) {
    // Push the global assets table to the lua stack.
    lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, _assetsTableRef);
    int globalTableIndex = lua_gettop(*_luaState);

    lua_pushnil(*_luaState);

    // Clear entry from global asset table (pushed to the lua stack earlier)
    lua_setfield(*_luaState, globalTableIndex, asset->id().c_str());
}

bool AssetLoader::loadAsset(std::shared_ptr<Asset> asset) {
    std::shared_ptr<Asset> parentAsset = _currentAsset;
    setUpAssetLuaTable(asset.get());
    setCurrentAsset(asset);
    ghoul::OnScopeExit e([this, parentAsset] {
        setCurrentAsset(parentAsset);
    });
    
    if (!FileSys.fileExists(asset->assetFilePath())) {
        LERROR("Could not load asset '" << asset->assetFilePath() << "': File does not exist.");
        tearDownAssetLuaTable(asset.get());
        return false;
    }

    try {
        ghoul::lua::runScriptFile(*_luaState, asset->assetFilePath());
    } catch (const ghoul::lua::LuaRuntimeException& e) {
        LERROR("Could not load asset '" << asset->assetFilePath() << "': " << e.message);
        tearDownAssetLuaTable(asset.get());
        return false;
    }

    return true;
}

std::string AssetLoader::generateAssetPath(const std::string& baseDirectory,
                                           const std::string& assetPath) const
{
    ghoul::filesystem::Directory directory = isRelative(assetPath) ?
        baseDirectory :
        _assetRootDirectory;
   
    return ghoul::filesystem::File(directory.path() +
        ghoul::filesystem::FileSystem::PathSeparator +
        assetPath +
        "." +
        AssetFileSuffix);
}

std::shared_ptr<Asset> AssetLoader::getAsset(std::string name) {
    ghoul::filesystem::Directory directory = currentDirectory();
    std::string path = generateAssetPath(directory, name);

    // Check if asset is already loaded.
    const auto it = _loadedAssets.find(path);

    if (it != _loadedAssets.end()) {
        std::shared_ptr<Asset> a = it->second.lock();
        if (a != nullptr) {
            return a;
        }
    }

    std::shared_ptr<Asset> a = std::make_shared<Asset>(this, _synchronizationWatcher, path);
    trackAsset(a);
    return a;
}

int AssetLoader::onInitializeLua(Asset* asset) {
    int nArguments = lua_gettop(*_luaState);
    SCRIPT_CHECK_ARGUMENTS("onInitialize", *_luaState, 1, nArguments);
    int referenceIndex = luaL_ref(*_luaState, LUA_REGISTRYINDEX);
    _onInitializationFunctionRefs[asset].push_back(referenceIndex);
    return 0;
}

int AssetLoader::onDeinitializeLua(Asset* asset) {
    int nArguments = lua_gettop(*_luaState);
    SCRIPT_CHECK_ARGUMENTS("onDeinitialize", *_luaState, 1, nArguments);
    int referenceIndex = luaL_ref(*_luaState, LUA_REGISTRYINDEX);
    _onDeinitializationFunctionRefs[asset].push_back(referenceIndex);
    return 0;
}

int AssetLoader::onInitializeDependencyLua(Asset* dependant, Asset* dependency) {
    int nArguments = lua_gettop(*_luaState);
    SCRIPT_CHECK_ARGUMENTS("onInitialize", *_luaState, 1, nArguments);
    int referenceIndex = luaL_ref(*_luaState, LUA_REGISTRYINDEX);
    _onDependencyInitializationFunctionRefs[dependant][dependency].push_back(referenceIndex);
    return 0;
}

int AssetLoader::onDeinitializeDependencyLua(Asset* dependant, Asset* dependency) {
    int nArguments = lua_gettop(*_luaState);
    SCRIPT_CHECK_ARGUMENTS("onDeinitialize", *_luaState, 1, nArguments);
    int referenceIndex = luaL_ref(*_luaState, LUA_REGISTRYINDEX);
    _onDependencyDeinitializationFunctionRefs[dependant][dependency].push_back(referenceIndex);
    return 0;
}

std::shared_ptr<Asset> AssetLoader::require(const std::string& name) {
    std::shared_ptr<Asset> asset = getAsset(name);
    std::shared_ptr<Asset> dependant = _currentAsset;
    dependant->require(asset);
    return asset;
}

std::shared_ptr<Asset> AssetLoader::request(const std::string& name) {
    std::shared_ptr<Asset> asset = getAsset(name);
    std::shared_ptr<Asset> parent = _currentAsset;
    parent->request(asset);
    assetRequested(parent, asset);
    return asset;
}

void AssetLoader::unrequest(const std::string& name) {
    std::shared_ptr<Asset> asset = has(name);
    std::shared_ptr<Asset> parent = _currentAsset;
    parent->unrequest(asset);
    assetUnrequested(parent, asset);
}

ghoul::filesystem::Directory AssetLoader::currentDirectory() const {
    if (_currentAsset->hasAssetFile()) {
        return _currentAsset->assetDirectory();
    } else {
        return _assetRootDirectory;
    }
}

std::shared_ptr<Asset> AssetLoader::add(const std::string& identifier) {
    setCurrentAsset(_rootAsset);
    return request(identifier);
}


void AssetLoader::remove(const std::string& identifier) {
    setCurrentAsset(_rootAsset);
    unrequest(identifier);
}

std::shared_ptr<Asset> AssetLoader::has(const std::string& name) const {
    ghoul::filesystem::Directory directory = currentDirectory();
    std::string path = generateAssetPath(directory, name);

    const auto it = _loadedAssets.find(path);
    if (it == _loadedAssets.end()) {
        return nullptr;
    }
    return it->second.lock();
}

ghoul::lua::LuaState* AssetLoader::luaState() {
    return _luaState;
}

std::shared_ptr<Asset> AssetLoader::rootAsset() const {
    return _rootAsset;
}

const std::string& AssetLoader::assetRootDirectory() const {
    return _assetRootDirectory;
}

void AssetLoader::callOnInitialize(Asset* asset) {
    for (int init : _onInitializationFunctionRefs[asset]) {
        lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, init);
        if (lua_pcall(*_luaState, 0, 0, 0) != LUA_OK) {
            throw ghoul::lua::LuaRuntimeException(
                "When initializing " + asset->assetFilePath() + ": " + luaL_checkstring(*_luaState, -1)
            );
        }
    }
}

void AssetLoader::callOnDeinitialize(Asset * asset) {
    std::vector<int>& funs = _onDeinitializationFunctionRefs[asset];
    for (auto it = funs.rbegin(); it != funs.rend(); it++) {
        lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, *it);
        if (lua_pcall(*_luaState, 0, 0, 0) != LUA_OK) {
            throw ghoul::lua::LuaRuntimeException(
                "When deinitializing " + asset->assetFilePath() + ": " + luaL_checkstring(*_luaState, -1)
            );
        }
    }
}

void AssetLoader::callOnDependencyInitialize(Asset* asset, Asset* dependant) {
    for (int init : _onDependencyInitializationFunctionRefs[dependant][asset]) {
        lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, init);
        if (lua_pcall(*_luaState, 0, 0, 0) != LUA_OK) {
            throw ghoul::lua::LuaRuntimeException(
                "When initializing dependency " + dependant->assetFilePath() + " -> " + 
                asset->assetFilePath() + ": " + luaL_checkstring(*_luaState, -1)
            );
        }
    }
}

void AssetLoader::callOnDependencyDeinitialize(Asset* asset, Asset* dependant) {
    std::vector<int>& funs = _onDependencyDeinitializationFunctionRefs[dependant][asset];
    for (auto it = funs.rbegin(); it != funs.rend(); it++) {
        lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, *it);
        if (lua_pcall(*_luaState, 0, 0, 0) != LUA_OK) {
            throw ghoul::lua::LuaRuntimeException(
                "When deinitializing dependency " + dependant->assetFilePath() + " -> " +
                asset->assetFilePath() + ": " + luaL_checkstring(*_luaState, -1)
            );
        }
    }
}

int AssetLoader::localResourceLua(Asset* asset) {
    int nArguments = lua_gettop(*_luaState);
    SCRIPT_CHECK_ARGUMENTS("localResource", *_luaState, 1, nArguments);

    std::string resourceName = luaL_checkstring(*_luaState, -1);
    std::string resolved = asset->resolveLocalResource(resourceName);

    lua_pushstring(*_luaState, resolved.c_str());
    return 1;
}

int AssetLoader::syncedResourceLua(Asset* asset) {
    int nArguments = lua_gettop(*_luaState);
    SCRIPT_CHECK_ARGUMENTS("syncedResource", *_luaState, 1, nArguments);

    ghoul::Dictionary d;
    ghoul::lua::luaDictionaryFromState(*_luaState, d);

    std::shared_ptr<ResourceSynchronization> sync =
        ResourceSynchronization::createFromDictionary(d);

    std::string absolutePath = sync->directory();

    asset->addSynchronization(sync);

    lua_pushstring(*_luaState, absolutePath.c_str());
    return 1;
}

void AssetLoader::setCurrentAsset(std::shared_ptr<Asset> asset) {
    _currentAsset = asset;
    // Set `asset` lua global to point to the current asset table

    if (asset == _rootAsset) {
        lua_pushnil(*_luaState);
        lua_setglobal(*_luaState, AssetGlobalVariableName);
        return;
    }

    lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, _assetsTableRef);
    lua_getfield(*_luaState, -1, asset->id().c_str());
    lua_getfield(*_luaState, -1, AssetTableName);
    lua_setglobal(*_luaState, AssetGlobalVariableName);
}

int AssetLoader::requireLua(Asset* dependant) {
    int nArguments = lua_gettop(*_luaState);
    SCRIPT_CHECK_ARGUMENTS("require", *_luaState, 1, nArguments);

    std::string assetName = luaL_checkstring(*_luaState, 1);

    std::shared_ptr<Asset> dependency = require(assetName);

    if (!dependency) {
        return luaL_error(*_luaState, "Asset '%s' not found", assetName.c_str());
    }

    addLuaDependencyTable(dependant, dependency.get());

    // Get the exports table
    lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, _assetsTableRef);
    lua_getfield(*_luaState, -1, dependency->id().c_str());
    lua_getfield(*_luaState, -1, ExportsTableName);
    int exportsTableIndex = lua_gettop(*_luaState);

    // Get the dependency table
    lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, _assetsTableRef);
    lua_getfield(*_luaState, -1, dependency->id().c_str());
    lua_getfield(*_luaState, -1, DependantsTableName);
    lua_getfield(*_luaState, -1, dependant->id().c_str());
    int dependencyTableIndex = lua_gettop(*_luaState);

    lua_pushvalue(*_luaState, exportsTableIndex);
    lua_pushvalue(*_luaState, dependencyTableIndex);
    return 2;
}

int AssetLoader::requestLua(Asset* parent) {
    int nArguments = lua_gettop(*_luaState);
    SCRIPT_CHECK_ARGUMENTS("request", *_luaState, 1, nArguments);
    
    std::string assetName = luaL_checkstring(*_luaState, 1);
    
    std::shared_ptr<Asset> child = request(assetName);

    addLuaDependencyTable(parent, child.get());
        
    // Get the dependency table
    lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, _assetsTableRef);
    lua_getfield(*_luaState, -1, child->id().c_str());
    lua_getfield(*_luaState, -1, DependantsTableName);
    lua_getfield(*_luaState, -1, parent->id().c_str());
    int dependencyTableIndex = lua_gettop(*_luaState);

    lua_pushvalue(*_luaState, dependencyTableIndex);
    return 1;
}

int AssetLoader::exportAssetLua(Asset* asset) {
    int nArguments = lua_gettop(*_luaState);
    SCRIPT_CHECK_ARGUMENTS("exportAsset", *_luaState, 2, nArguments);

    std::string exportName = luaL_checkstring(*_luaState, 1);

    lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, _assetsTableRef);
    lua_getfield(*_luaState, -1, asset->id().c_str());
    lua_getfield(*_luaState, -1, ExportsTableName);
    int exportsTableIndex = lua_gettop(*_luaState);

    // push the second argument
    lua_pushvalue(*_luaState, 2);
    lua_setfield(*_luaState, exportsTableIndex, exportName.c_str());
    return 0;
}

void AssetLoader::addLuaDependencyTable(Asset* dependant, Asset* dependency) {
    const std::string dependantId = dependant->id();
    const std::string dependencyId = dependency->id();

    lua_rawgeti(*_luaState, LUA_REGISTRYINDEX, _assetsTableRef);
    lua_getfield(*_luaState, -1, dependencyId.c_str());
    const int dependencyIndex = lua_gettop(*_luaState);

    // Extract the imported asset's dependants table
    lua_getfield(*_luaState, -1, DependantsTableName);
    const int dependantsTableIndex = lua_gettop(*_luaState);

    // Set up Dependency object
    lua_newtable(*_luaState);
    const int currentDependantTableIndex = lua_gettop(*_luaState);

    // Register onDependencyInitialize function
    // void onInitialize(function<void()> initializationFunction)
    lua_pushlightuserdata(*_luaState, dependant);
    lua_pushlightuserdata(*_luaState, dependency);
    lua_pushcclosure(*_luaState, &assetloader::onInitializeDependency, 2);
    lua_setfield(*_luaState, currentDependantTableIndex, OnInitializeFunctionName);

    // Register onDependencyDeinitialize function
    // void onDeinitialize(function<void()> deinitializationFunction)
    lua_pushlightuserdata(*_luaState, dependant);
    lua_pushlightuserdata(*_luaState, dependency);
    lua_pushcclosure(*_luaState, &assetloader::onDeinitializeDependency, 2);
    lua_setfield(*_luaState, currentDependantTableIndex, OnDeinitializeFunctionName);

    // duplicate the table reference on the stack, so it remains after assignment.
    lua_pushvalue(*_luaState, -1);

    // Register the dependant table on the imported asset's dependants table.
    lua_setfield(*_luaState, dependantsTableIndex, dependantId.c_str());
}

void AssetLoader::addAssetListener(AssetListener* listener) {
    const auto it = std::find(
        _assetListeners.begin(),
        _assetListeners.end(),
        listener
    );

    if (it == _assetListeners.end()) {
        _assetListeners.push_back(listener);
    }
}

void AssetLoader::removeAssetListener(AssetListener* listener) {
    _assetListeners.erase(std::remove(
        _assetListeners.begin(),
        _assetListeners.end(),
        listener
    ));
}

void AssetLoader::assetStateChanged(std::shared_ptr<Asset> asset, Asset::State state) {
    for (auto& listener : _assetListeners) {
        listener->assetStateChanged(asset, state);
    }
}

void AssetLoader::assetRequested(std::shared_ptr<Asset> parent, std::shared_ptr<Asset> child) {
    for (auto& listener : _assetListeners) {
        listener->assetRequested(parent, child);
    }
}

void AssetLoader::assetUnrequested(std::shared_ptr<Asset> parent, std::shared_ptr<Asset> child) {
    for (auto& listener : _assetListeners) {
        listener->assetUnrequested(parent, child);
    }
}


}
