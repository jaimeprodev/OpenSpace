﻿/*****************************************************************************************
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

namespace openspace {
namespace luascriptfunctions {

int importAsset(lua_State* state) {
    AssetLoader *assetLoader =
        reinterpret_cast<AssetLoader*>(lua_touserdata(state, lua_upvalueindex(1)));

    int nArguments = lua_gettop(state);
    SCRIPT_CHECK_ARGUMENTS("importAsset", state, 1, nArguments);

    std::string assetName = luaL_checkstring(state, -1);

    assetLoader->importAsset(assetName);
    return 0;
}

int unimportAsset(lua_State* state) {
    AssetLoader *assetLoader =
        reinterpret_cast<AssetLoader*>(lua_touserdata(state, lua_upvalueindex(1)));

    int nArguments = lua_gettop(state);
    SCRIPT_CHECK_ARGUMENTS("unimportAsset", state, 1, nArguments);

    std::string assetName = luaL_checkstring(state, -1);

    assetLoader->unimportAsset(assetName);
    return 0;
}

} // namespace luascriptfunctions

namespace assetloader {

/**
 * Adds a Lua function to be called upon asset initialization
 * Usage: void asset.onInitialize(function<void()> initFun)
 */
int onInitialize(lua_State* state) {
    Asset *asset =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    return asset->loader()->onInitializeLua(asset);
}

/**
 * Adds a Lua function to be called upon asset deinitialization
 * Usage: void asset.onDeinitialize(function<void()> initFun)
 */
int onDeinitialize(lua_State* state) {
    Asset *asset =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    return asset->loader()->onDeinitializeLua(asset);
}

/**
* Adds a Lua function to be called when a dependency link is initialized
* Usage: void asset.onInitialize(function<void()> initFun)
*/
int onInitializeDependency(lua_State* state) {
    Asset *dependant =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    Asset *dependency =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(2)));
    return dependant->loader()->onInitializeDependencyLua(dependant, dependency);
}

/**
* Adds a Lua function to be called upon asset deinitialization
* Usage: void asset.onDeinitialize(function<void()> initFun)
*/
int onDeinitializeDependency(lua_State* state) {
    Asset *dependant =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    Asset *dependency =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(2)));
    return dependant->loader()->onDeinitializeDependencyLua(dependant, dependency);
}


/**
 * Imports required dependencies
 * Gives access to
 *   AssetTable: Exported lua values
 *   Dependency: ...
 * Usage: {AssetTable, Dependency} = asset.import(string assetIdentifier)
 */
int importRequiredDependency(lua_State* state) {
    Asset *asset =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    return asset->loader()->importRequiredDependencyLua(asset);
}

int importOptionalDependency(lua_State* state) {
    Asset *asset =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    return asset->loader()->importOptionalDependencyLua(asset);
}

int resolveLocalResource(lua_State* state) {
    Asset *asset =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    return asset->loader()->resolveLocalResourceLua(asset);
}

int resolveSyncedResource(lua_State* state) {
    Asset *asset =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    return asset->loader()->resolveSyncedResourceLua(asset);
}

int addSynchronization(lua_State* state) {
    Asset *asset =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    return asset->loader()->addSynchronizationLua(asset);
}

int noOperation(lua_State*) {
    return 0;
}

int exportAsset(lua_State* state) {
    Asset *asset =
        reinterpret_cast<Asset*>(lua_touserdata(state, lua_upvalueindex(1)));
    return asset->loader()->exportAssetLua(asset);
}

} // namespace assetloader



}
