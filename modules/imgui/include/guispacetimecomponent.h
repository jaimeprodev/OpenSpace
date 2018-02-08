/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2018                                                               *
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

#ifndef __OPENSPACE_MODULE_IMGUI___GUISPACETIMECOMPONENT___H__
#define __OPENSPACE_MODULE_IMGUI___GUISPACETIMECOMPONENT___H__

#include <modules/imgui/include/guicomponent.h>

#include <openspace/properties/scalar/floatproperty.h>
#include <openspace/util/timeconversion.h>

namespace openspace::gui {

class GuiSpaceTimeComponent : public GuiComponent {
public:
    GuiSpaceTimeComponent();
    void render() override;

private:
    float _deltaTime;
    int _deltaTimeUnit; // aka static_cast<int>(TimeUnit)

    float _accelerationDelta;

    double _oldDeltaTime;
    float _slidingDelta;

    bool _firstFrame;

    std::string _timeUnits;


    //properties::FloatProperty _minMaxDeltaTime;
    //float _localMinMaxDeltatime; // We don't want the default display inside the component
};

} // namespace openspace::gui

#endif // __OPENSPACE_MODULE_IMGUI___GUISPACETIMECOMPONENT___H__
