#ifndef __OPENSPACE_MODULE_SKYBROWSER___SCREENSPACESKYBROWSER___H__
#define __OPENSPACE_MODULE_SKYBROWSER___SCREENSPACESKYBROWSER___H__

#include <modules/webbrowser/include/screenspacebrowser.h>
#include <openspace/properties/stringproperty.h>

namespace openspace {
    class ScreenSpaceSkyTarget;

    class ScreenSpaceSkyBrowser : public ScreenSpaceBrowser
    {
    public:
        ScreenSpaceSkyBrowser(const ghoul::Dictionary& dictionary);
        virtual ~ScreenSpaceSkyBrowser() = default;

        bool initializeGL() override;
        bool deinitializeGL() override;
        bool setConnectedTarget();

        // Communication with the webpage and WWT
        void executeJavascript(std::string script) const;
        ghoul::Dictionary createMessageForMovingWWTCamera(const glm::dvec2 celestCoords, const double fov, const bool moveInstantly = true) const;
        ghoul::Dictionary createMessageForPausingWWTTime() const;
        ghoul::Dictionary createMessageForLoadingWWTImgColl(const std::string& url) const;
        ghoul::Dictionary createMessageForSettingForegroundWWT(const std::string& name) const;
        ghoul::Dictionary createMessageForSettingForegroundOpacityWWT(double val) const;
        bool sendMessageToWWT(const ghoul::Dictionary& msg);
        void sendMouseEvent(CefStructBase<CefMouseEventTraits> event, int x, int y) const;
        void WWTfollowCamera();
        float fieldOfView() const;
        void setVerticalFieldOfView(float fov);
        void scrollZoom(float scroll);
        ScreenSpaceSkyTarget* getSkyTarget();

        // Translation
        //void translate(glm::vec2 translation);

        // Position and dimension and corners
        glm::vec2 getBrowserPixelDimensions();
        glm::vec2 coordIsOnResizeArea(glm::vec2 coord);
        // Scaling
        void scale(glm::vec2 scalingFactor);
        void scale(float scalingFactor);
        glm::mat4 scaleMatrix() override;
        // Resizing
        void saveResizeStartSize();
        void updateBrowserSize();
        void setBorderColor(glm::ivec3 addColor);
        glm::ivec3 getColor();
        // Flag for dimensions
        bool _browserDimIsDirty;
        properties::FloatProperty _vfieldOfView;
        properties::StringProperty _skyTargetID;
    private:
        glm::vec2 _startDimensionsSize;
        float _startScale;
        properties::Vec2Property _browserDimensions;
        bool _camIsSyncedWWT;
        ScreenSpaceSkyTarget* _skyTarget;
        std::thread _threadWWTMessages;
        glm::ivec3 _borderColor;
       
    };
}

#endif // __OPENSPACE_MODULE_SKYBROWSER___SCREENSPACESKYBROWSER___H__

