/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2022                                                               *
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

#ifndef __OPENSPACE_MODULE_SKYBROWSER___TARGETBROWSERPAIR___H__
#define __OPENSPACE_MODULE_SKYBROWSER___TARGETBROWSERPAIR___H__

#include <openspace/documentation/documentation.h>
#include <deque>

namespace openspace {

class ScreenSpaceSkyBrowser;
class ScreenSpaceSkyTarget;
class ScreenSpaceRenderable;
class ImageData;

class TargetBrowserPair {
public:
    constexpr static const float FadeThreshold = 0.01f;

    TargetBrowserPair(ScreenSpaceSkyBrowser* browser, ScreenSpaceSkyTarget* target);
    TargetBrowserPair& operator=(TargetBrowserPair other);

    // Target & Browser
    void initialize();
    // Highlighting
    void removeHighlight(const glm::ivec3& color); 
    void highlight(const glm::ivec3& color);
    // Animation
    void startAnimation(glm::dvec3 coordsEnd, float fovEnd, bool shouldLockAfter = true);
    void incrementallyAnimateToCoordinate(double deltaTime);
    void incrementallyFade(float goalState, float fadeTime, float deltaTime);
    // Mouse interaction
    bool checkMouseIntersection(const glm::vec2& mousePosition);
    glm::vec2 selectedScreenSpacePosition() const;
    bool isBrowserSelected() const;
    bool isTargetSelected() const;
    void fineTuneTarget(const glm::vec2& start, const glm::vec2& translation);
    void translateSelected(const glm::vec2& start, const glm::vec2& translation);
    void synchronizeAim();

    // Browser
    void sendIdToBrowser();
    void updateBrowserSize();

    // Target
    void centerTargetOnScreen();
    void lock();
    void unlock();

    bool hasFinishedFading(float goalState) const;
    bool isFacingCamera() const;
    bool isUsingRadiusAzimuthElevation() const;
    bool isEnabled() const;
    bool isLocked() const;

    void setEnabled(bool enable);
    void setIsSyncedWithWwt(bool isSynced);
    void setVerticalFov(float vfov);
    void setEquatorialAim(const glm::dvec2& aim);
    void setBorderColor(const glm::ivec3& color);
    void setScreenSpaceSize(const glm::vec2& dimensions);
    void setVerticalFovWithScroll(float scroll);
    void setSelectedWithId(const std::string& id);

    float verticalFov() const;
    glm::ivec3 borderColor() const;
    glm::dvec2 targetDirectionEquatorial() const;
    glm::dvec3 targetDirectionGalactic() const;
    std::string browserGuiName() const;
    std::string browserId() const;
    std::string targetId() const;
    std::string selectedId();
    glm::vec2 size() const;
    
    ScreenSpaceSkyTarget* target() const;
    ScreenSpaceSkyBrowser* browser() const;
    const std::deque<int>& selectedImages() const;
    
    // WorldWide Telescope image handling
    void setImageOrder(int i, int order);
    void selectImage(const ImageData& image, int i);
    void removeSelectedImage(int i);
    void loadImageCollection(const std::string& collection);
    void setImageOpacity(int i, float opacity);
    void hideChromeInterface(bool shouldHide);

    friend bool operator==(const TargetBrowserPair& lhs, 
                           const TargetBrowserPair& rhs);
    friend bool operator!=(const TargetBrowserPair& lhs, 
                           const TargetBrowserPair& rhs);

private:
    bool isTargetFadeFinished(float goalState) const;
    bool isBrowserFadeFinished(float goalState) const;

    // Selection
    ScreenSpaceRenderable* _selected = nullptr;
    bool _isSelectedBrowser = false;
    
    // Target and browser
    ScreenSpaceSkyTarget* _target = nullptr;
    ScreenSpaceSkyBrowser* _browser = nullptr;

    // Shared properties between the target and the browser
    float _verticalFov = 70.f;
    glm::dvec2 _equatorialAim = glm::dvec2(0.0);
    glm::ivec3 _borderColor = glm::ivec3(255);
    glm::vec2 _dimensions = glm::vec2(0.5f);
};

} // namespace openspace

#endif // __OPENSPACE_MODULE_SKYBROWSER___TARGETBROWSERPAIR___H__
