#include <openspace/util/openspacemodule.h>


#include <openspace/documentation/documentation.h>
#include <modules/skybrowser/skybrowsermodule.h>
#include <openspace/engine/globals.h>
#include <openspace/engine/moduleengine.h>
#include <openspace/rendering/renderengine.h>

#include <openspace/scripting/scriptengine.h>
#include <ghoul/misc/dictionaryluaformatter.h>

#include <ghoul/filesystem/filesystem.h>
#include <ghoul/fmt.h>
#include <ghoul/glm.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/misc/assert.h>
#include <fstream>
#include <sstream>
#include <modules/skybrowser/include/screenspaceskybrowser.h>
#include <modules/skybrowser/include/screenspaceskytarget.h>
#include <openspace/interaction/navigationhandler.h>
#include <openspace/util/camera.h>
#include <thread> 

#include <openspace/util/coordinateconversion.h>
#include <glm/gtx/rotate_vector.hpp>


namespace {
    constexpr const char _loggerCat[] = "SkyBrowserModule";
} // namespace


namespace openspace::skybrowser::luascriptfunctions {

    int loadImgCollection(lua_State* L) {
        // Load image
        ghoul::lua::checkArgumentsAndThrow(L, 1, "lua::loadCollection");
        const std::string& imageName = ghoul::lua::value<std::string>(L, 1);

        ScreenSpaceSkyBrowser* browser = dynamic_cast<ScreenSpaceSkyBrowser*>(global::renderEngine->screenSpaceRenderable("SkyBrowser1"));
       
        browser->sendMessageToWWT(browser->createMessageForSettingForegroundWWT(imageName));
        LINFO("Loading image " + imageName);
       // browser->sendMessageToWWT(browser->createMessageForMovingWWTCamera(glm::vec2(0.712305533333333, 41.269167), 24.0f));
        browser->sendMessageToWWT(browser->createMessageForSettingForegroundOpacityWWT(100));
        return 1;
    }
    
    int followCamera(lua_State* L) {
        // Load images from url
        ghoul::lua::checkArgumentsAndThrow(L, 0, "lua::followCamera");

        SkyBrowserModule* module = global::moduleEngine->module<SkyBrowserModule>();
        std::string root = "https://raw.githubusercontent.com/WorldWideTelescope/wwt-web-client/master/assets/webclient-explore-root.wtml";

        module->getWWTDataHandler()->loadWTMLCollectionsFromURL(root, "root");
        LINFO(std::to_string( module->getWWTDataHandler()->loadAllImagesFromXMLs()));

        return 1;
    }

    int moveBrowser(lua_State* L) {
        // Load images from local directory
        ghoul::lua::checkArgumentsAndThrow(L, 0, "lua::moveBrowser");
        SkyBrowserModule* module = global::moduleEngine->module<SkyBrowserModule>();
        module->getWWTDataHandler()->loadWTMLCollectionsFromDirectory(absPath("${MODULE_SKYBROWSER}/WWTimagedata/"));
        std::string noOfLoadedImgs = std::to_string(module->getWWTDataHandler()->loadAllImagesFromXMLs());
        LINFO("Loaded " + noOfLoadedImgs + " WorldWide Telescope images.");

        ScreenSpaceSkyBrowser* browser = dynamic_cast<ScreenSpaceSkyBrowser*>(global::renderEngine->screenSpaceRenderable("SkyBrowser1"));
        const std::vector<std::string>& imageUrls = module->getWWTDataHandler()->getAllImageCollectionUrls();
        for (const std::string url : imageUrls) {
            browser->sendMessageToWWT(browser->createMessageForLoadingWWTImgColl(url));
        }
        return 1;
    }

    int createBrowser(lua_State* L) {
        // Send image list to GUI
        ghoul::lua::checkArgumentsAndThrow(L, 0, "lua::createBrowser");
        SkyBrowserModule* module = global::moduleEngine->module<SkyBrowserModule>();
        // If no data has been loaded yet, load it!
        if (module->getWWTDataHandler()->getLoadedImages().size() == 0) {
            moveBrowser(L);
        }
       
        std::vector<std::pair<std::string, std::string>> names = module->getWWTDataHandler()->getAllThumbnailUrls();

        lua_newtable(L);

        int number = 1;
        for (const std::pair<std::string, std::string>& s : names) {
            // Push a table { image name, image url } with index : number
            lua_newtable(L);
            lua_pushstring(L, s.first.c_str());
            lua_rawseti(L, -2, 1);
            lua_pushstring(L, s.second.c_str());
            lua_rawseti(L, -2, 2);

            lua_rawseti(L, -2, number);
            ++number;
        }
        
        return 1;
    }
    int adjustCamera(lua_State* L) {
        ghoul::lua::checkArgumentsAndThrow(L, 0, "lua::adjustCamera");

        return 1;
    }
    
}

