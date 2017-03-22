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

#include <regex>

namespace openspace {

namespace {

void executePropertySet(properties::Property* prop, lua_State* L) {
    prop->setLuaValue(L);
    //ensure properties are synced over parallel connection
    std::string value;
    prop->getStringValue(value);
    /*OsEng.parallelConnection().scriptMessage(
    prop->fullyQualifiedIdentifier(),
    value
    );*/
}

template <class T>
properties::PropertyOwner* findPropertyOwnerWithMatchingGroupTag(T* prop,
const std::string tagToMatch)     {
    properties::PropertyOwner* tagMatchOwner = nullptr;
    properties::PropertyOwner* owner = prop->owner();
    
    if (owner != nullptr) {
        std::vector<std::string>* tags = (std::vector<std::string>*)owner->getTags();
        for (std::string currTag : *tags) {
            if (tagToMatch.compare(currTag) == 0) {
                tagMatchOwner = owner;
                break;
            }
        }
        
        //Call recursively until we find an owner with matching tag or the top of the
        // ownership list
        if (tagMatchOwner == nullptr)
        tagMatchOwner = findPropertyOwnerWithMatchingGroupTag(owner, tagToMatch);
    }
    return tagMatchOwner;
}

void applyRegularExpression(lua_State* L, std::regex regex,
                            std::vector<properties::Property*> properties, int type,
                            std::string groupName = "")                               {
    using ghoul::lua::errorLocation;
    using ghoul::lua::luaTypeToString;
    bool isGroupMode = (groupName.compare("") != 0) ? true : false;
    
    for (properties::Property* prop : properties) {
        // Check the regular expression for all properties
        std::string id = prop->fullyQualifiedIdentifier();

        if (std::regex_match(id, regex)) {
            // If the fully qualified id matches the regular expression, we queue the
            // value change if the types agree
            if (isGroupMode) {
                properties::PropertyOwner* matchingTaggedOwner = findPropertyOwnerWithMatchingGroupTag(prop,
                groupName);
                if (matchingTaggedOwner == nullptr)
                    continue;
            }
            
            if (type != prop->typeLua()) {
                LERRORC("property_setValue",
                        errorLocation(L) << "Property '" <<
                        prop->fullyQualifiedIdentifier() <<
                        "' does not accept input of type '" << luaTypeToString(type) <<
                        "'. Requested type: '" << luaTypeToString(prop->typeLua()) << "'"
                );
            } else {
               executePropertySet(prop, L);
            }
        }
    }
}

std::string extractGroupNameFromUri(std::string command) {
    return command.substr(0, command.find_first_of("."));
}

std::string replaceUriGroupNameWith(std::string uri, std::string ownerName) {
    size_t pos = uri.find_first_of(".");
    return ownerName + "." + uri.substr(pos);
}

std::string extractUriWithoutGroupName(std::string uri) {
    size_t pos = uri.find_first_of(".");
    return uri.substr(pos);
}
}


namespace luascriptfunctions {

/**
 * \ingroup LuaScripts
 * setPropertyValueSingle(string, *):
 * Sets the property identified by the URI in the first argument to the value passed to
 * the second argument. The type of the second argument is arbitrary, but it must agree
 * with the type the denoted Property expects
 */
int property_setValueSingle(lua_State* L) {
    using ghoul::lua::errorLocation;
    using ghoul::lua::luaTypeToString;

    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("property_setValueSingle", L, 2, nArguments);

    std::string uri = luaL_checkstring(L, -2);
    const int type = lua_type(L, -1);

    properties::Property* prop = property(uri);
    if (!prop) {
        LERRORC("property_setValue", errorLocation(L) << "Property with URI '"
            << uri << "' was not found");
        return 0;
    }


    if (type != prop->typeLua()) {
        LERRORC("property_setValue", errorLocation(L) << "Property '" << uri <<
            "' does not accept input of type '" << luaTypeToString(type) <<
            "'. Requested type: '" << luaTypeToString(prop->typeLua()) << "'");
        return 0;
    }
    else {
        prop->setLuaValue(L);
        //ensure properties are synced over parallel connection
        std::string value;
        prop->getStringValue(value);
        //OsEng.parallelConnection().scriptMessage(prop->fullyQualifiedIdentifier(), value);
    }

    return 0;
}

/**
 * \ingroup LuaScripts
 * setPropertyValueRegex(string, *):
 * Sets all properties that pass the regular expression in the first argument. The second
 * argument can be any type, but it has to match the type of the properties that matched
 * the regular expression. The regular expression has to be of the ECMAScript grammar.
*/
int property_setValueRegex(lua_State* L) {
    using ghoul::lua::errorLocation;
    using ghoul::lua::luaTypeToString;

    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("property_setValueRegex<", L, 2, nArguments);

    std::string regex = luaL_checkstring(L, -2);
    try {
        applyRegularExpression(
            L,
            std::regex(regex, std::regex_constants::optimize),
            allProperties(),
            lua_type(L, -1)
        );
    }
    catch (const std::regex_error& e) {
        LERRORC("property_setValueRegex", "Malformed regular expression: '"
            << regex << "'");
    }

    return 0;
}

/**
* \ingroup LuaScripts
* setPropertyValue(string, *):
* Sets all properties identified by the URI (with potential wildcards) in the first
* argument. The second argument can be any type, but it has to match the type that the
* property (or properties) expect.
*/

int property_setValue(lua_State* L) {
    using ghoul::lua::errorLocation;
    using ghoul::lua::luaTypeToString;

    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("property_setValue", L, 2, nArguments);

    std::string regex = luaL_checkstring(L, -2);

    // Replace all wildcards *  with the correct regex (.*)
    size_t startPos = regex.find("*");
    while (startPos != std::string::npos) {
        regex.replace(startPos, 1, "(.*)");
        startPos += 4;

        startPos = regex.find("*", startPos);
    }


    applyRegularExpression(
        L,
        std::regex(regex/*, std::regex_constants::optimize*/),
        allProperties(),
        lua_type(L, -1)
    );

    return 0;
}

/**
 * \ingroup LuaScripts
 * setPropertyGroupSingle(string, *):
 * Sets all properties that belong to a tagged group AND match the URI (requires exact
 * match) in the first argument (group tag name is given in place of property owner
 * name). The second argument can be any type, but it has to match the type that the
 * property expects.
 */
int property_setGroupSingle(lua_State* L) {
    using ghoul::lua::errorLocation;
    using ghoul::lua::luaTypeToString;

    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("property_setGroupSingle", L, 2, nArguments);

    std::string uri = luaL_checkstring(L, -2);
    const int type = lua_type(L, -1);
    std::string tagToMatch = extractGroupNameFromUri(uri);
    std::string pathRemainderToMatch = extractUriWithoutGroupName(uri);
 
    for (properties::Property* prop : allProperties()) {
        std::string propFullId = prop->fullyQualifiedIdentifier();
        //Look for a match in the uri with the group name (first term) removed
        int propMatchLength = propFullId.length() - pathRemainderToMatch.length();

        if (propMatchLength >= 0) {
            std::string thisPropMatchId = propFullId.substr(propMatchLength);
            //If remainder of uri matches (with group name removed),
            if (pathRemainderToMatch.compare(thisPropMatchId) == 0)  {
                properties::PropertyOwner* matchingTaggedOwner
                    = findPropertyOwnerWithMatchingGroupTag(prop, tagToMatch);
                if (matchingTaggedOwner != nullptr) {
                    if (type != prop->typeLua()) {
                        LERRORC("property_setValue", errorLocation(L) << "Property '"
                            << prop->guiName() << "' does not accept input of type '"
                            << luaTypeToString(type) << "'. Requested type: '"
                            << luaTypeToString(prop->typeLua()) << "'");
                        return 0;
                    }
                    else {
                        prop->setLuaValue(L);
                        //ensure properties are synced over parallel connection
                        std::string value;
                        prop->getStringValue(value);
                        //OsEng.parallelConnection().scriptMessage(
                        //    prop->fullyQualifiedIdentifier(), value);
                        break;
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * \ingroup LuaScripts
 * setPropertyGroup(string, *):
 * Sets all properties that belong to a tagged group AND match the URI (with optional
 * wildcards) in the first argument (group tag name is given in place of property owner
 * name). The second argument can be any type, but it has to match the type that the
 * property expects.
 */

int property_setGroup(lua_State* L) {
    using ghoul::lua::errorLocation;
    using ghoul::lua::luaTypeToString;

    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("property_setGroup", L, 2, nArguments);

    std::string regex = luaL_checkstring(L, -2);
    std::string groupName = extractGroupNameFromUri(regex);

    // Replace all wildcards *  with the correct regex (.*)
    size_t startPos = regex.find("*");
    while (startPos != std::string::npos) {
        regex.replace(startPos, 1, "(.*)");
        startPos += 4;
        startPos = regex.find("*", startPos);
    }
    //Remove group name from start of regex and replace with '.*'
    regex = replaceUriGroupNameWith(regex, ".*");

    applyRegularExpression(
        L,
        std::regex(regex/*, std::regex_constants::optimize*/),
        allProperties(),
        lua_type(L, -1),
        groupName
    );

    return 0;
}

/**
 * \ingroup LuaScripts
 * setPropertyGroupRegex(string, *):
 * Sets all properties that belong to a tagged group AND match the URI (allows regular
 * expression syntax) in the first argument (group tag name is given in place of property
 * owner name). The second argument can be any type, but it has to match the type that 
 * the property expects.
*/
int property_setGroupRegex(lua_State* L) {
    using ghoul::lua::errorLocation;
    using ghoul::lua::luaTypeToString;

    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("property_setGroupRegex<", L, 2, nArguments);

    std::string regex = luaL_checkstring(L, -2);
    std::string groupName = extractGroupNameFromUri(regex);
    //Remove group name from start of regex and replace with '.*'
    regex = replaceUriGroupNameWith(regex, ".*");

    try {
        applyRegularExpression(
            L,
            std::regex(regex, std::regex_constants::optimize),
            allProperties(),
            lua_type(L, -1),
            groupName
        );
    }
    catch (const std::regex_error& e) {
        LERRORC("property_setValueRegex", "Malformed regular expression: '"
            << regex << "'");
    }

    return 0;
}

/**
 * \ingroup LuaScripts
 * getPropertyValue(string):
 * Returns the value of the property identified by the passed URI as a Lua object that can
 * be passed to the setPropertyValue method.
 */
int property_getValue(lua_State* L) {
    static const std::string _loggerCat = "property_getValue";
    using ghoul::lua::errorLocation;

    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("property_getValue", L, 1, nArguments);

    std::string uri = luaL_checkstring(L, -1);

    openspace::properties::Property* prop = property(uri);
    if (!prop) {
        LERRORC(
            "property_getValue",
            errorLocation(L) << "Property with URL '" << uri << "' was not found"
        );
        return 0;
    }
    else
        prop->getLuaValue(L);
    return 1;
}

/**
 * \ingroup LuaScripts
 * getPropertyValue(string):
 * Returns the value of the property identified by the passed URI as a Lua object that can
 * be passed to the setPropertyValue method.
 */
int loadScene(lua_State* L) {
    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("loadScene", L, 1, nArguments);

    std::string sceneFile = luaL_checkstring(L, -1);

    OsEng.renderEngine().scene()->scheduleLoadSceneFile(sceneFile);

    return 0;
}

int addSceneGraphNode(lua_State* L) {
    using ghoul::lua::errorLocation;

    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("addSceneGraphNode", L, 1, nArguments);

    ghoul::Dictionary d;
    try {
        ghoul::lua::luaDictionaryFromState(L, d);
    }
    catch (const ghoul::lua::LuaFormatException& e) {
        LERRORC("addSceneGraphNode", e.what());
        return 0;
    }

    SceneGraphNode* node = SceneGraphNode::createFromDictionary(d);
    
    std::string parent = d.value<std::string>(SceneGraphNode::KeyParentName);
    SceneGraphNode* parentNode = OsEng.renderEngine().scene()->sceneGraphNode(parent);
    if (!parentNode) {
        LERRORC(
            "addSceneGraphNode",
            errorLocation(L) << "Could not find parent node '" << parent << "'"
        );
        return 0;
    }
    node->setParent(parentNode);
    node->initialize();
    OsEng.renderEngine().scene()->sceneGraph().addSceneGraphNode(node);
        
    return 0;
}

int removeSceneGraphNode(lua_State* L) {
    using ghoul::lua::errorLocation;

    int nArguments = lua_gettop(L);
    SCRIPT_CHECK_ARGUMENTS("removeSceneGraphNode", L, 1, nArguments);
    
    std::string nodeName = luaL_checkstring(L, -1);
    SceneGraphNode* node = OsEng.renderEngine().scene()->sceneGraphNode(nodeName);
    if (!node) {
        LERRORC(
            "removeSceneGraphNode",
            errorLocation(L) << "Could not find node '" << nodeName << "'"
        );
        return 0;
    }

    OsEng.renderEngine().scene()->sceneGraph().removeSceneGraphNode(node);
    node->deinitialize();
    delete node;

    return 1;
}

} // namespace luascriptfunctions

}  // namespace openspace
