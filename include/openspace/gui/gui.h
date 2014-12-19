/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014                                                                    *
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

#ifndef __GUI_H__
#define __GUI_H__

#include <openspace/gui/guihelpcomponent.h>
#include <openspace/gui/guiperformancecomponent.h>
#include <openspace/gui/guipropertycomponent.h>
#include <openspace/scripting/scriptengine.h>

namespace openspace {
namespace gui {

class GUI {
public:
	GUI();

	bool isEnabled() const;
	void setEnabled(bool enabled);

	void initialize();
	void deinitialize();

	void initializeGL();
	void deinitializeGL();

	bool mouseButtonCallback(int key, int action);
	bool mouseWheelCallback(int position);
	bool keyCallback(int key, int action);
	bool charCallback(unsigned int character);

	void startFrame(float deltaTime, const glm::vec2& windowSize, const glm::vec2& mousePos, bool mouseButtonsPressed[2]);
	void endFrame();

	void renderMainWindow();

	static openspace::scripting::ScriptEngine::LuaLibrary luaLibrary();

//protected:
	GuiPerformanceComponent _performance;
	GuiPropertyComponent _property;
	GuiHelpComponent _help;

	bool _isEnabled;

	bool _showHelp;
};

} // namespace gui
} // namespace openspace

#endif // __GUI_H__
