/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2023                                                               *
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

#ifndef __OPENSPACE_MODULE_VOLUME___TRANSFERFUNCTIONPROPERTY___H__
#define __OPENSPACE_MODULE_VOLUME___TRANSFERFUNCTIONPROPERTY___H__

#include <openspace/properties/templateproperty.h>
#include <modules/volume/transferfunction.h>

namespace openspace::properties {

class TransferFunctionProperty : public TemplateProperty<volume::TransferFunction> {
public:
    TransferFunctionProperty(Property::PropertyInfo info,
        volume::TransferFunction value = volume::TransferFunction());

    std::string_view className() const override;
    int typeLua() const override;

    using TemplateProperty<volume::TransferFunction>::operator=;

protected:
    volume::TransferFunction fromLuaConversion(lua_State* state,
        bool& success) const override;
    void toLuaConversion(lua_State* state) const override;
    std::string toStringConversion() const override;
};

} // namespace openspace::properties

#endif // __OPENSPACE_MODULE_VOLUME___TRANSFERFUNCTIONPROPERTY___H__
