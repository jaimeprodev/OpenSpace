#ifndef __OPENSPACE_MODULE_SKYBROWSER___SCREENSPACESKYTARGET___H__
#define __OPENSPACE_MODULE_SKYBROWSER___SCREENSPACESKYTARGET___H__

#include <openspace/rendering/screenspacerenderable.h>

#include <openspace/properties/scalar/floatproperty.h>
#include <ghoul/opengl/ghoul_gl.h>

#include <ghoul/opengl/texture.h>

namespace openspace::documentation { struct Documentation; }

namespace openspace {

    class ScreenSpaceSkyTarget : public ScreenSpaceRenderable {
    
    public:
        ScreenSpaceSkyTarget(const ghoul::Dictionary& dictionary);
        virtual ~ScreenSpaceSkyTarget() = default;

        void bindTexture() override;
    private:
        std::unique_ptr<ghoul::opengl::Texture> _texture;

    };
}

#endif // __OPENSPACE_MODULE_SKYBROWSER___SCREENSPACESKYTARGET___H__

