/*****************************************************************************************
*                                                                                       *
* OpenSpace                                                                             *
*                                                                                       *
* Copyright (c) 2014-2018                                                               *
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

#ifndef __OPENSPACE_MODULE_GLOBEBROWSING___SHADOWCOMPONENT___H__
#define __OPENSPACE_MODULE_GLOBEBROWSING___SHADOWCOMPONENT___H__

#include <openspace/properties/propertyowner.h>

#include <openspace/properties/scalar/boolproperty.h>
#include <openspace/properties/scalar/floatproperty.h>
#include <openspace/properties/scalar/intproperty.h>
#include <openspace/properties/vector/vec2property.h>
#include <openspace/properties/vector/vec4property.h>
#include <openspace/properties/stringproperty.h>
#include <openspace/properties/triggerproperty.h>

#include <openspace/util/camera.h>

#include <ghoul/glm.h>
#include <ghoul/opengl/texture.h>
#include <ghoul/opengl/uniformcache.h>

#include <string>
#include <sstream>

namespace ghoul {
    class Dictionary;
}
   
namespace ghoul::filesystem { class File; }

namespace ghoul::opengl {
    class ProgramObject;
} // namespace ghoul::opengl

namespace openspace {
    struct RenderData;
    struct UpdateData;

    namespace documentation { struct Documentation; }

    static const GLfloat ShadowBorder[] = { 1.f, 1.f, 1.f, 1.f };

    class ShadowComponent : public properties::PropertyOwner {
    public:
        struct ShadowMapData {
            glm::dmat4 shadowMatrix;
            GLuint shadowDepthTexture;
        };
    public:
        ShadowComponent(const ghoul::Dictionary& dictionary);

        void initialize();
        void initializeGL();
        void deinitializeGL();
        //bool deinitialize();

        bool isReady() const;

        RenderData begin(const RenderData& data);
        void end();
        void update(const UpdateData& data);

        static documentation::Documentation Documentation();

        bool isEnabled() const;

        ShadowComponent::ShadowMapData shadowMapData() const;

    private:
        void createDepthTexture();
        void createShadowFBO();

        // Debug
        void saveDepthBuffer();
        void checkGLError(const std::string & where) const;

    private:

        ShadowMapData _shadowData;

        // Texture coords in [0, 1], while clip coords in [-1, 1]
        const glm::dmat4 _toTextureCoordsMatrix = glm::dmat4(
            glm::dvec4(0.5, 0.0, 0.0, 0.0),
            glm::dvec4(0.0, 0.5, 0.0, 0.0),
            glm::dvec4(0.0, 0.0, 1.0, 0.0),
            glm::dvec4(0.5, 0.5, 0.0, 1.0)
        );

        // DEBUG
        properties::TriggerProperty _saveDepthTexture;
        properties::IntProperty _distanceFraction;
        properties::BoolProperty _enabled;
        
        ghoul::Dictionary _shadowMapDictionary;
        
        int _shadowDepthTextureHeight;
        int _shadowDepthTextureWidth;
        
        GLuint _shadowDepthTexture;
        GLuint _positionInLightSpaceTexture;
        GLuint _shadowFBO;
        GLuint _firstPassSubroutine;
        GLuint _secondPassSubroutine;
        GLint _defaultFBO;
        GLint _mViewport[4];

        GLboolean _faceCulling;
        GLboolean _polygonOffSet;
        GLboolean _depthIsEnabled;
        GLboolean _blendIsEnabled = false;

        GLenum _faceToCull;
        GLenum _depthFunction;

        GLfloat _polygonOffSetFactor;
        GLfloat _polygonOffSetUnits;
        GLfloat _colorClearValue[4];
        GLfloat _depthClearValue;

        glm::vec3 _sunPosition;

        glm::dmat4 _shadowMatrix;

        glm::dvec3 _cameraPos;
        glm::dvec3 _cameraFocus;
        glm::dquat _cameraRotation;

        std::stringstream _serializedCamera;

        std::unique_ptr<Camera> _lightCamera;

        // DEBUG
        bool _executeDepthTextureSave;
        
    };

} // namespace openspace

#endif // __OPENSPACE_MODULE_GLOBEBROWSING___SHADOWCOMPONENT___H__
