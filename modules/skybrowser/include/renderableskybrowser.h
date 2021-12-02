#ifndef __OPENSPACE_MODULE_SKYBROWSER___RENDERABLESKYBROWSER___H__
#define __OPENSPACE_MODULE_SKYBROWSER___RENDERABLESKYBROWSER___H__

#include <modules/skybrowser/include/wwtcommunicator.h>
#include <modules/base/rendering/renderableplane.h>
#include <openspace/documentation/documentation.h>

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4100)
#endif // _MSC_VER

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif // __clang__

#include <include/cef_client.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__

#ifdef _MSC_VER
#pragma warning (pop)
#endif // _MSC_VER

namespace ghoul::opengl { class Texture; }

namespace openspace::documentation { struct Documentation; }

namespace openspace {

    class RenderableSkyBrowser : public RenderablePlane, public WwtCommunicator
    {
    public:
        static constexpr const char* KeyIdentifier = "Identifier";

        RenderableSkyBrowser(const ghoul::Dictionary& dictionary);
        ~RenderableSkyBrowser();

        // Inherited from RenderablePlane
        void initializeGL() override;
        void deinitializeGL() override;
        void update(const UpdateData& data) override;
        void render(const RenderData& data, RendererTasks& rendererTask) override;

        // Set up initialization with wwt
        void setIdInBrowser();

        // Place
        void placeAt3dPosition(const glm::dvec3& positionSpeck, float verticalFov, 
                               const std::string& sceneGraphNodeId);

    private:
        void bindTexture() override;
        void unbindTexture() override;
    };
}

#endif // __OPENSPACE_MODULE_SKYBROWSER___RENDERABLESKYBROWSER___H__

