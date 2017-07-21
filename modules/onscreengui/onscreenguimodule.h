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

#ifndef __OPENSPACE_MODULE_ONSCREENGUI___ONSCREENGUIMODULE___H__
#define __OPENSPACE_MODULE_ONSCREENGUI___ONSCREENGUIMODULE___H__

#include <openspace/util/openspacemodule.h>

#include <modules/onscreengui/include/gui.h>

namespace openspace {

struct Touch {
    bool active;
    glm::vec2 pos;
    uint32_t action;
};

class OnScreenGUIModule : public OpenSpaceModule {
public:
    constexpr static const char* Name = "OnScreenGUI";

    OnScreenGUIModule();
    
    static gui::GUI gui;
    static Touch touchInput;
};

} // namespace openspace

#endif // __OPENSPACE_MODULE_ONSCREENGUI___ONSCREENGUIMODULE___H__
