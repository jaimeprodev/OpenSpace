/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2016                                                               *
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

#include <openspace/scene/scale.h>
#include <openspace/util/factorymanager.h>
#include <ghoul/logging/logmanager.h>

namespace {
    const std::string _loggerCat = "Scale";
    const std::string KeyType = "Type";
}

namespace openspace {

Scale* Scale::createFromDictionary(const ghoul::Dictionary& dictionary) {
    if (!dictionary.hasValue<std::string>(KeyType)) {
        LERROR("Ephemeris did not have key '" << KeyType << "'");
        return nullptr;
    }

    std::string scaleType;
    dictionary.getValue(KeyType, scaleType);
    ghoul::TemplateFactory<Scale>* factory
          = FactoryManager::ref().factory<Scale>();
    Scale* result = factory->create(scaleType, dictionary);
    if (result == nullptr) {
        LERROR("Failed creating Scale object of type '" << scaleType << "'");
        return nullptr;
    }

    return result;
}

Scale::Scale() {}
    
Scale::Scale(const ghoul::Dictionary& dictionary) {}
    
Scale::~Scale() {}
    
bool Scale::initialize() {
    return true;
}
    
void Scale::update(const UpdateData& data) {}

} // namespace openspace