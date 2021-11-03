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

#ifndef __OPENSPACE_MODULE_SKYBROWSER___SKYBROWSERMODULE___H__
#define __OPENSPACE_MODULE_SKYBROWSER___SKYBROWSERMODULE___H__

#include <openspace/util/openspacemodule.h>

#include <openspace/documentation/documentation.h>
#include <openspace/properties/scalar/boolproperty.h>
#include <openspace/properties/scalar/floatproperty.h>
#include <openspace/properties/stringproperty.h>
#include <openspace/util/distanceconstants.h>
#include <thread>
#include <string>

#include <iostream>
#include <fstream>

namespace openspace {

class ScreenSpaceSkyBrowser;
class ScreenSpaceSkyTarget;
class RenderableSkyBrowser;
class ScreenSpaceImageLocal;
class WwtDataHandler;
class SceneGraphNode;
class ImageData;
class Pair;


class SkyBrowserModule : public OpenSpaceModule {
public:

    constexpr static const char* Name = "SkyBrowser";

    // Constructor & destructor
    SkyBrowserModule();
    virtual ~SkyBrowserModule();

    // Getters
    std::vector<Pair>& getPairs();
    Pair* getPair(std::string id);
    SceneGraphNode* get3dBrowser();
    const WwtDataHandler* getWWTDataHandler();
    std::string selectedBrowserId();

    // Setters
    void setSelectedBrowser(ScreenSpaceSkyBrowser* ptr);
    void setSelectedBrowser(std::string id);
    void set3dBrowser(SceneGraphNode* node);
    void selectImage2dBrowser(int i);
    void selectImage3dBrowser(int i);
   
    // Rotation and animation
    void startRotation(glm::dvec3 endAnimation); // Pass in galactic coordinate
    void rotateCamera(double deltaTime);
    bool fadeBrowserAndTarget(bool makeTransparent, double fadeTime, double deltaTime);
    void lookAt3dBrowser();
   
    // Boolean functions
    bool cameraInSolarSystem();

    // Managing the browsers 
    void createTargetBrowserPair();
    void removeTargetBrowserPair(std::string& browserId);
    void addTargetBrowserPair(ScreenSpaceSkyTarget* target, ScreenSpaceSkyBrowser* browser);
    void moveHoverCircle(int i);
    
    // Image collection handling
    void loadImages(const std::string& root, const std::string& directory, 
                   std::vector<std::filesystem::path>& speckFiles);
    int nLoadedImages();

    // Manage mouse interactions
    void setSelectedObject();

    scripting::LuaLibrary luaLibrary() const override;
    //std::vector<documentation::Documentation> documentations() const override;


protected: 
    void internalInitialize(const ghoul::Dictionary& dict) override;
    void internalDeinitialize() override;

private:
    // The browsers and targets
    std::vector<Pair> _targetsBrowsers;
    Pair* _mouseOnPair{ nullptr };
    Pair* _selectedPair{ nullptr };
    bool _isBrowser{ false };
    ScreenSpaceImageLocal* _hoverCircle{ nullptr };
    SceneGraphNode* _browser3d{ nullptr };
    std::string _selectedBrowser{ "" }; // Currently selected browser (2D or 3D)

    // 2D vs 3D visualization mode
    double _solarSystemRadius = 30.0 * distanceconstants::AstronomicalUnit;
    double _fadingTime = 2.0;
    
    // Flags
    bool _fineTuneMode{ false };
    bool _isResizing{ false };
    bool _isDragging{ false };
    bool _cameraInSolarSystem{ true };
    bool _cameraIsRotating = false;

    // Mouse interaction - dragging and resizing
    glm::ivec3 _highlightAddition{ 35 }; // Highlight object when mouse hovers
    glm::vec2 _mousePosition; // Current mouse position in screen space coordinates
    glm::vec2 _startMousePosition;
    glm::vec2 _startDragPosition;
    glm::vec2 _startBrowserSize;
    glm::vec2 _resizeDirection{ 0.f };

    // Animation of rotation of camera to look at coordinate galactic coordinates
    glm::dvec3 _startAnimation;
    glm::dvec3 _endAnimation;
    double _threshold{ 0.0005 };
    double _speed{ 1.0 };
    
    // Data handler for the image collections
    WwtDataHandler* _dataHandler;    
};

} // namespace openspace

#endif // __OPENSPACE_MODULE_SKYBROWSER___SKYBROWSERMODULE___H__
