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

#include <modules/skybrowser/skybrowsermodule.h>

 //#include <modules/webbrowser/webbrowsermodule.h>
 //#include <modules/webbrowser/include/screenspacebrowser.h>
#include <openspace/rendering/screenspacerenderable.h>

#include <openspace/rendering/renderable.h>

#include <openspace/engine/moduleengine.h>

#include <openspace/engine/globals.h>
#include <openspace/engine/globalscallbacks.h>
#include <openspace/interaction/navigationhandler.h>
#include <openspace/engine/windowdelegate.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/scene/scene.h>
#include <openspace/util/factorymanager.h>
#include <thread>
#include <chrono>
#include "skybrowsermodule_lua.inl"

#include <openspace/engine/windowdelegate.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/opengl/texture.h>

#include <cmath> // For atan2
#include <ghoul/misc/dictionaryjsonformatter.h> // formatJson
#include <glm/gtx/string_cast.hpp>


namespace {
    constexpr const openspace::properties::Property::PropertyInfo TestInfo = 
    {
        "Test",
        "Test Info",
        "tjobidabidobidabidopp plopp"
    };
    constexpr const openspace::properties::Property::PropertyInfo ZoomInfo =
    {
        "Zoom",
        "Zoom Info",
        "tjobidabidobidabidopp plupp"
    };

    struct [[codegen::Dictionary(SkyBrowserModule)]] Parameters {

        // [[codegen::verbatim(TestInfo.description)]]
        std::optional<std::string> test;

        // [[codegen::verbatim(ZoomInfo.description)]]
        std::optional<float> zoom;
    };
    
    #include "skybrowsermodule_codegen.cpp"
    
    
} // namespace

namespace openspace {

SkyBrowserModule::SkyBrowserModule()
    : OpenSpaceModule(SkyBrowserModule::Name)
    , _testProperty(TestInfo)
    , _zoomFactor(ZoomInfo, 50.f ,0.1f ,70.f)
    , _skyBrowser(nullptr)
    , _camIsSyncedWWT(true)
    , mouseIsClickedPreviously(false)
    , _listenForInteractions(true)
{
    addProperty(_testProperty);
    addProperty(_zoomFactor);

    createTarget();

    global::callback::mousePosition->emplace_back(
        [&](double x, double y) {
            _mousePosition = glm::vec2(static_cast<float>(x), static_cast<float>(y));
        }
    );
} 

void SkyBrowserModule::internalDeinitialize() {
        // Set flag to false so the thread can exit
    _camIsSyncedWWT = false;
    if (_threadWWTMessages.joinable()) {
        _threadWWTMessages.join();
        LINFO("Joined thread");
    }
    if (_threadHandleInteractions.joinable()) {
        _threadHandleInteractions.join();
        LINFO("Joined thread");
    }
}

scripting::LuaLibrary SkyBrowserModule::luaLibrary() const {

    scripting::LuaLibrary res;
    res.name = "skybrowser";
    res.functions = {
        {
            "create",
            &skybrowser::luascriptfunctions::createBrowser,
            {},
            "string or list of strings",
            "Add one or multiple exoplanet systems to the scene, as specified by the "
            "input. An input string should be the name of the system host star"
        },
        {
            "move",
            &skybrowser::luascriptfunctions::moveBrowser,
            {},
            "string or list of strings",
            "Add one or multiple exoplanet systems to the scene, as specified by the "
            "input. An input string should be the name of the system host star"
        },
        {
            "follow",
            &skybrowser::luascriptfunctions::followCamera,
            {},
            "string or list of strings",
            "Add one or multiple exoplanet systems to the scene, as specified by the "
            "input. An input string should be the name of the system host star"
        },
        {
            "loacImgCollection",
            &skybrowser::luascriptfunctions::loadImgCollection,
            {},
            "string or list of strings",
            "Add one or multiple exoplanet systems to the scene, as specified by the "
            "input. An input string should be the name of the system host star"
        }
    };

    return res;
}

float SkyBrowserModule::zoomFactor() const{
    return _zoomFactor;
}

void SkyBrowserModule::internalInitialize(const ghoul::Dictionary& dict) {
    
    const Parameters p = codegen::bake<Parameters>(dict);
    _testProperty = p.test.value_or(_testProperty);
    _zoomFactor = p.zoom.value_or(_zoomFactor);

    // register ScreenSpaceBrowser
    auto fScreenSpaceRenderable = FactoryManager::ref().factory<ScreenSpaceRenderable>();
    ghoul_assert(fScreenSpaceRenderable, "ScreenSpaceRenderable factory was not created");
    fScreenSpaceRenderable->registerClass<ScreenSpaceSkyBrowser>("ScreenSpaceSkyBrowser");

    // register ScreenSpaceTarget
    ghoul_assert(fScreenSpaceRenderable, "ScreenSpaceRenderable factory was not created");
    fScreenSpaceRenderable->registerClass<ScreenSpaceSkyTarget>("ScreenSpaceSkyTarget");

}

bool SkyBrowserModule::sendMessageToWWT(const ghoul::Dictionary& msg) {
    if (_skyBrowser) {
        std::string script = "sendMessageToWWT(" + ghoul::formatJson(msg) + ");";
        _skyBrowser->executeJavascript(script);
        return true;
    }
    else {
        LERROR("No sky browser added! Can't send message.");
        return false;
    }
}

void SkyBrowserModule::handleInteractions() {
    /*
   float scroll = global::navigationHandler->inputState().mouseScrollDelta();
   bool mouseIsOverBrowser = _skyBrowser->coordIsInsideBrowserScreenSpace(getMousePositionInScreenSpaceCoords());
   LINFO(std::to_string(mouseIsOverBrowser));
       
    _zoomFactor = std::clamp(_zoomFactor - scroll, 0.001f, 70.0f);
    CefStructBase<CefMouseEventTraits> event;
    _skyBrowser->sendMouseEvent(event, scroll, scroll);
   */
    _threadHandleInteractions = std::thread([&] {
        while (_listenForInteractions) {
            bool mouseIsClicked = global::navigationHandler->inputState().isMouseButtonPressed(MouseButton::Left);

            if (mouseIsClicked) {
                dragBrowser();
            }         
            else {
                mouseIsClickedPreviously = false;
            }
        }
    });
}

glm::vec2 SkyBrowserModule::getMousePositionInScreenSpaceCoords() {
    glm::vec2 size = glm::vec2(global::windowDelegate->currentWindowSize());

    // Transform pixel coordinates to screen space coordinates [-1,1]
    return glm::vec2((_mousePosition - (size / 2.0f)) * glm::vec2(1.0f,-1.0f) / (size / 2.0f));
}

void SkyBrowserModule::dragBrowser() {
    // First click on browser - user is not holding down the button since last frame
    glm::dvec2  mouseCoords = getMousePositionInScreenSpaceCoords();
    bool mouseIsOnBrowser = _skyBrowser->coordIsInsideBrowserScreenSpace(mouseCoords);

    if (mouseIsOnBrowser && !mouseIsClickedPreviously) {
        mouseIsClickedPreviously = true;
        startDragMousePos = mouseCoords;
        startDragObjectPos = _skyBrowser->getScreenSpacePosition();
    }
    else if (mouseIsClickedPreviously) {
        _skyBrowser->translate(mouseCoords - startDragMousePos, startDragObjectPos);
    }
}

void SkyBrowserModule::WWTfollowCamera() {
    
    // Start a thread to enable user interaction while sending the calls to WWT
    _threadWWTMessages = std::thread([&] {
        while (_camIsSyncedWWT) {

            // Get camera view direction
            const glm::dvec3 viewDirection = global::navigationHandler->camera()->viewDirectionWorldSpace();

            // Convert to celestial coordinates
            glm::dvec2 celestCoords = convertGalacticToCelestial(viewDirection);
            ghoul::Dictionary message = createMessageForMovingWWTCamera(celestCoords, _zoomFactor);

            // Sleep so we don't bombard WWT with too many messages
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            sendMessageToWWT(message);
            
        }
    });
    
}

ghoul::Dictionary SkyBrowserModule::createMessageForMovingWWTCamera(const glm::dvec2 celestCoords, const float fov,  const bool moveInstantly) const {
    using namespace std::string_literals;
    ghoul::Dictionary msg;
    msg.setValue("event", "center_on_coordinates"s);
    msg.setValue("ra", static_cast<double>(celestCoords[0]));
    msg.setValue("dec", static_cast<double>(celestCoords[1]));
    msg.setValue("fov", static_cast<double>(fov));
    msg.setValue("instant", moveInstantly);
    
    return msg;
}

ghoul::Dictionary SkyBrowserModule::createMessageForLoadingWWTImgColl(const std::string& url) const {
    using namespace std::string_literals;
    ghoul::Dictionary msg;
    msg.setValue("event", "center_on_coordinates"s);
    msg.setValue("url", url);

    return msg;
}

ghoul::Dictionary SkyBrowserModule::createMessageForPausingWWTTime() const {

    using namespace std::string_literals;
    ghoul::Dictionary msg;
    msg.setValue("event", "pause_time"s);

    return msg;
}


void SkyBrowserModule::initializeBrowser(ScreenSpaceSkyBrowser* skyBrowser) {
    _skyBrowser = skyBrowser;
}

ScreenSpaceSkyBrowser* SkyBrowserModule::skyBrowser() {
    return _skyBrowser;
}

glm::dvec2 SkyBrowserModule::convertGalacticToCelestial(glm::dvec3 rGal) const {
    
    // Used the math from this website: https://gea.esac.esa.int/archive/documentation/GD -->
    // R2/Data_processing/chap_cu3ast/sec_cu3ast_intro/ssec_cu3ast_intro_tansforms.html#SSS1
    const glm::dmat3 conversionMatrix = glm::dmat3({
      -0.0548755604162154,  0.4941094278755837, -0.8676661490190047, // col 0
      -0.8734370902348850, -0.4448296299600112, -0.1980763734312015, // col 1
      -0.4838350155487132,  0.7469822444972189,  0.4559837761750669  // col 2
        });
   
    glm::dvec3 rICRS = glm::transpose(conversionMatrix) * rGal;
    float ra = atan2(rICRS[1], rICRS[0]);
    float dec = atan2(rICRS[2], glm::sqrt((rICRS[0] * rICRS[0]) + (rICRS[1] * rICRS[1])));

    ra = ra > 0 ? ra : ra + (2 * glm::pi<float>());

    return glm::dvec2(glm::degrees(ra), glm::degrees(dec));
}


void SkyBrowserModule::checkIfTargetExist() {
    ScreenSpaceSkyTarget* target = static_cast<ScreenSpaceSkyTarget*>(global::renderEngine->screenSpaceRenderable("ScreenSpaceTarget")); 

    if (target) {
        LINFO("Target is not null!");

    }
    LINFO("Target has been checked!");
}

void SkyBrowserModule::createTarget() {

    using namespace std::string_literals;

    // Create target test
    std::string node = "{"
        "Type = 'ScreenSpaceSkyTarget',"
        "Identifier = 'ScreenSpaceTarget',"
        "Name = 'Screen Space Target',"
        "FaceCamera = false"
        "}";

    openspace::global::scriptEngine->queueScript(
        "openspace.addScreenSpaceRenderable(" + node + ")",
        scripting::ScriptEngine::RemoteScripting::Yes
    );

}



/*
std::vector<documentation::Documentation> SkyBrowserModule::documentations() const {
    return {
        ExoplanetsDataPreparationTask::documentation(),
        RenderableOrbitDisc::Documentation()
    };
}
*/
} // namespace openspace
