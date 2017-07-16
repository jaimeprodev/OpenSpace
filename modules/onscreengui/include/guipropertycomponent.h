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

#ifndef __OPENSPACE_MODULE_ONSCREENGUI___GUIPROPERTYCOMPONENT___H__
#define __OPENSPACE_MODULE_ONSCREENGUI___GUIPROPERTYCOMPONENT___H__

#include <modules/onscreengui/include/guicomponent.h>

#include <openspace/properties/property.h>

#include <functional>
#include <string>
#include <vector>

namespace openspace::properties {
    class Property;
    class PropertyOwner;
} // namespace openspace::properties

namespace openspace::gui {

class GuiPropertyComponent : public GuiComponent {
public:
    using SourceFunction = std::function<std::vector<properties::PropertyOwner*>()>;

    GuiPropertyComponent(std::string name);

    // This is the function that evaluates to the list of Propertyowners that this
    // component should render
    void setSource(SourceFunction func);

    void setVisibility(properties::Property::Visibility visibility);
    void setHasRegularProperties(bool hasOnlyRegularProperties);

    void render();

protected:
    void renderPropertyOwner(properties::PropertyOwner* owner);
    void renderProperty(properties::Property* prop, properties::PropertyOwner* owner);

    properties::Property::Visibility _visibility;

    SourceFunction _function;
    /// This is set to \c true if all properties contained in this GUIPropertyComponent
    /// are regular, i.e., not containing wildcards, regex, or groups
    /// This variable only has an impact on which \c setPropertyValue function is called
    bool _hasOnlyRegularProperties = false;
};

} // namespace openspace::gui

#endif // __OPENSPACE_MODULE_ONSCREENGUI___GUIPROPERTYCOMPONENT___H__
