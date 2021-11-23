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

#include <modules/skybrowser/include/wwtcommunicator.h>

#include <modules/webbrowser/webbrowsermodule.h>
#include <modules/webbrowser/include/webkeyboardhandler.h>
#include <modules/webbrowser/include/browserinstance.h>
#include <ghoul/misc/dictionaryjsonformatter.h> // formatJson
#include <modules/skybrowser/include/utility.h>
#include <glm/gtx/vector_angle.hpp>
#include <cmath> // For atan2
#define _USE_MATH_DEFINES
#include <math.h> // For M_PI

namespace {
    constexpr const char* _loggerCat = "WwtCommunicator";

    constexpr const openspace::properties::Property::PropertyInfo BorderColorInfo =
    {
        "BorderColor",
        "Border Color",
        "The color of the border of the sky browser."
    };
    constexpr const openspace::properties::Property::PropertyInfo VerticalFovInfo =
    {
        "VerticalFieldOfView",
        "Vertical Field Of View",
        "The vertical field of view in degrees."
    };
    constexpr const openspace::properties::Property::PropertyInfo EquatorialAimInfo =
    {
        "EquatorialAim",
        "Equatorial aim of the sky browser",
        "The aim of the sky browser given in spherical equatorial coordinates right "
        "ascension (RA) and declination (Dec) in epoch J2000. The unit is degrees."
    };

} // namespace

namespace openspace {

    WwtCommunicator::WwtCommunicator(const ghoul::Dictionary& dictionary)
        : Browser(dictionary),
        _verticalFov(VerticalFovInfo, 10.f, 0.01f, 70.0f),
        _borderColor(BorderColorInfo, glm::ivec3(200), glm::ivec3(0), glm::ivec3(255)),
        _equatorialAim(EquatorialAimInfo, glm::dvec2(0.0), glm::dvec2(0.0, -90.0), 
            glm::dvec2(360.0, 90.0))
    {
        _borderColor.onChange([this]() {
            updateBorderColor();
        });
    }

    WwtCommunicator::~WwtCommunicator() {

    }

    void WwtCommunicator::displayImage(const std::string& url, int i)
    {   
        // Ensure there are no duplicates
        auto it = std::find(std::begin(_selectedImages), std::end(_selectedImages), i);
        if (it == std::end(_selectedImages)) {
            // Push newly selected image to front
            _selectedImages.push_front(i);
            // Index of image is used as layer ID as it is unique in the image data set
            sendMessageToWwt(addImage(std::to_string(i), url));
            sendMessageToWwt(setImageOpacity(std::to_string(i), 1.0));
        }
    }

    void WwtCommunicator::removeSelectedImage(int i) {
        // Remove from selected list
        auto it = std::find(std::begin(_selectedImages), std::end(_selectedImages), i);

        if (it != std::end(_selectedImages)) {
            _selectedImages.erase(it);
            sendMessageToWwt(removeImage(std::to_string(i)));
        }
    }

    void WwtCommunicator::sendMessageToWwt(const ghoul::Dictionary& msg) {
        std::string script = "sendMessageToWWT(" + ghoul::formatJson(msg) + ");";
        executeJavascript(script);
    }

    const std::deque<int>& WwtCommunicator::getSelectedImages() {
        return _selectedImages;
    }

    void WwtCommunicator::setVerticalFov(float vfov) {
        _verticalFov = vfov;
    }

    void WwtCommunicator::setWebpageBorderColor(glm::ivec3 color) {
        std::string stringColor = std::to_string(color.x) + ","
            + std::to_string(color.y) + "," + std::to_string(color.z);
        std::string script = "document.body.style.backgroundColor = 'rgb("
            + stringColor + ")';";
        executeJavascript(script);
    }

    void WwtCommunicator::setIsSyncedWithWwt(bool isSynced)
    {
        _isSyncedWithWwt = isSynced;
    }

    void WwtCommunicator::setEquatorialAim(glm::dvec3 cartesian)
    {
        _equatorialAim = skybrowser::cartesianToSpherical(cartesian);
    }

    void WwtCommunicator::highlight(glm::ivec3 addition)
    {
        setWebpageBorderColor(_borderColor.value() + addition);
    }

    void WwtCommunicator::removeHighlight(glm::ivec3 removal)
    {
        setWebpageBorderColor(_borderColor.value() - removal);
    }

    void WwtCommunicator::updateBorderColor()
    {
        setWebpageBorderColor(_borderColor);
    }

    glm::dvec2 WwtCommunicator::fieldsOfView() {
        glm::dvec2 browserFov = glm::dvec2(verticalFov() * browserRatio(), verticalFov());

        return browserFov;
    }

    bool WwtCommunicator::hasLoadedImages() const {
        return _hasLoadedImages;
    }

    glm::dvec3 WwtCommunicator::equatorialAimCartesian() const
    {
        return skybrowser::sphericalToCartesian(_equatorialAim);
    }

    void WwtCommunicator::setImageOrder(int i, int order) {
        // Find in selected images list
        auto current = std::find(
            std::begin(_selectedImages),
            std::end(_selectedImages),
            i
        );
        auto target = std::begin(_selectedImages) + order;

        // Make sure the image was found in the list
        if (current != std::end(_selectedImages) && target != std::end(_selectedImages)) {
            // Swap the two images
            std::iter_swap(current, target);
        }

        int reverseOrder = _selectedImages.size() - order - 1;
        ghoul::Dictionary message = setLayerOrder(std::to_string(i),
            reverseOrder);
        sendMessageToWwt(message);
    }

    void WwtCommunicator::loadImageCollection(const std::string& collection) {
        sendMessageToWwt(loadCollection(collection));
        _hasLoadedImages = true;
    }

    void WwtCommunicator::setImageOpacity(int i, float opacity) {
        ghoul::Dictionary msg = setImageOpacity(std::to_string(i), opacity);
        sendMessageToWwt(msg);
    }

    void WwtCommunicator::update()
    {
        if (_isSyncedWithWwt) {
            const std::chrono::time_point<std::chrono::high_resolution_clock> 
                timeBefore = std::chrono::high_resolution_clock::now();

            std::chrono::microseconds duration =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    timeBefore - latestCall
                    );

            if (duration > interval) {
                // Message WorldWide Telescope current view
                ghoul::Dictionary message = moveCamera(
                    _equatorialAim,
                    _verticalFov,
                    skybrowser::cameraRoll()
                );
                sendMessageToWwt(message);

                latestCall = std::chrono::high_resolution_clock::now();
            }
        }

        Browser::update();        
    }

    void WwtCommunicator::setHasLoadedImages(bool isLoaded) {
        _hasLoadedImages = isLoaded;
    }

    void WwtCommunicator::setIdInBrowser(const std::string& id) {
        // Send ID to it's browser
        executeJavascript("setId('" + id + "')");
    }


    glm::ivec3 WwtCommunicator::borderColor() const {
        return _borderColor;
    }

    float WwtCommunicator::verticalFov() const {
        return _verticalFov;
    }

    // WWT messages
    ghoul::Dictionary WwtCommunicator::moveCamera(const glm::dvec2& celestCoords, 
                                                  double fov, double roll, 
                                                  bool shouldMoveInstantly) {
        using namespace std::string_literals;
        ghoul::Dictionary msg;

        // Create message
        msg.setValue("event", "center_on_coordinates"s);
        msg.setValue("ra", celestCoords.x);
        msg.setValue("dec", celestCoords.y);
        msg.setValue("fov", fov);
        msg.setValue("roll", roll);
        msg.setValue("instant", shouldMoveInstantly);

        return msg;
    }

    ghoul::Dictionary WwtCommunicator::loadCollection(const std::string& url) {
        using namespace std::string_literals;
        ghoul::Dictionary msg;
        msg.setValue("event", "load_image_collection"s);
        msg.setValue("url", url);
        msg.setValue("loadChildFolders", true);

        return msg;
    }

    ghoul::Dictionary WwtCommunicator::setForeground(const std::string& name) {
        using namespace std::string_literals;
        ghoul::Dictionary msg;
        msg.setValue("event", "set_foreground_by_name"s);
        msg.setValue("name", name);

        return msg;
    }

    ghoul::Dictionary WwtCommunicator::addImage(const std::string& id, 
                                                const std::string& url) {
        using namespace std::string_literals;
        ghoul::Dictionary msg;
        msg.setValue("event", "image_layer_create"s);
        msg.setValue("id", id);
        msg.setValue("url", url);
        msg.setValue("mode", "preloaded"s);
        msg.setValue("goto", false);

        return msg;
    }

    ghoul::Dictionary WwtCommunicator::removeImage(const std::string& imageId) {
        using namespace std::string_literals;
        ghoul::Dictionary msg;
        msg.setValue("event", "image_layer_remove"s);
        msg.setValue("id", imageId);

        return msg;
    }

    ghoul::Dictionary WwtCommunicator::setImageOpacity(const std::string& imageId, 
                                                       double opacity) {
        using namespace std::string_literals;
        ghoul::Dictionary msg;
        msg.setValue("event", "image_layer_set"s);
        msg.setValue("id", imageId);
        msg.setValue("setting", "opacity"s);
        msg.setValue("value", opacity);

        return msg;
    }

    ghoul::Dictionary WwtCommunicator::setForegroundOpacity(double val) {
        using namespace std::string_literals;
        ghoul::Dictionary msg;
        msg.setValue("event", "set_foreground_opacity"s);
        msg.setValue("value", val);

        return msg;
    }

    ghoul::Dictionary WwtCommunicator::setLayerOrder(const std::string& id, int order) {
        // The lower the layer order, the more towards the back the image is placed
        // 0 is the background
        using namespace std::string_literals;
        ghoul::Dictionary msg;
        msg.setValue("event", "image_layer_order"s);
        msg.setValue("id", id);
        msg.setValue("order", order);
        msg.setValue("version", messageCounter);

        messageCounter++;

        return msg;
    }

} // namespace openspace
