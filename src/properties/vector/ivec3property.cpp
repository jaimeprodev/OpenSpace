/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2021                                                               *
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

#include <openspace/properties/vector/ivec3property.h>

#include <ghoul/glm.h>
#include <ghoul/lua/ghoul_lua.h>
#include <ghoul/misc/misc.h>
#include <limits>
#include <sstream>

namespace {

glm::ivec3 fromLuaConversion(lua_State* state, bool& success) {
    glm::ivec3 result = glm::ivec3(0);
    lua_pushnil(state);
    for (glm::length_t i = 0; i < ghoul::glm_components<glm::ivec3>::value; ++i) {
        int hasNext = lua_next(state, -2);
        if (hasNext != 1) {
            success = false;
            return glm::ivec3(0);
        }
        if (lua_isnumber(state, -1) != 1) {
            success = false;
            return glm::ivec3(0);
        }
        else {
            result[i] = static_cast<glm::ivec3::value_type>(lua_tonumber(state, -1));
            lua_pop(state, 1);
        }
    }

    // The last accessor argument is still on the stack
    lua_pop(state, 1);
    success = true;
    return result;
}

bool toLuaConversion(lua_State* state, glm::ivec3 value) {
    lua_newtable(state);
    int number = 1;
    for (glm::length_t i = 0; i < ghoul::glm_components<glm::ivec3>::value; ++i) {
        lua_pushnumber(state, static_cast<lua_Number>(value[i]));
        lua_rawseti(state, -2, number);
        ++number;
    }
    return true;
}

bool toStringConversion(std::string& outValue, glm::ivec3 inValue) {
    outValue = "{";
    for (glm::length_t i = 0; i < ghoul::glm_components<glm::ivec3>::value; ++i) {
        outValue += std::to_string(inValue[i]) + ",";
    }
    outValue.pop_back();
    outValue += "}";
    return true;
}

} // namespace

namespace openspace::properties {

REGISTER_NUMERICALPROPERTY_SOURCE(
    IVec3Property,
    glm::ivec3,
    glm::ivec3(0),
    glm::ivec3(std::numeric_limits<int>::lowest()),
    glm::ivec3(std::numeric_limits<int>::max()),
    glm::ivec3(1),
    fromLuaConversion,
    toLuaConversion,
    toStringConversion,
    LUA_TTABLE
)

} // namespace openspace::properties
