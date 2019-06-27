/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2019                                                               *
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

#include <modules/galaxy/rendering/renderablegalaxy.h>

#include <modules/galaxy/rendering/galaxyraycaster.h>
#include <modules/volume/rawvolume.h>
#include <modules/volume/rawvolumereader.h>
#include <openspace/engine/globals.h>
#include <openspace/rendering/raycastermanager.h>
#include <openspace/rendering/renderable.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/util/boxgeometry.h>
#include <openspace/util/updatestructures.h>
#include <ghoul/glm.h>
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/io/texture/texturereader.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/opengl/ghoul_gl.h>
#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/texture.h>
#include <ghoul/opengl/textureunit.h>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>

namespace {
    /*constexpr const char* GlslRayCastPath  = "${MODULES}/toyvolume/shaders/rayCast.glsl";
    constexpr const char* GlslBoundsVsPath = "${MODULES}/toyvolume/shaders/boundsVs.glsl";
    constexpr const char* GlslBoundsFsPath = "${MODULES}/toyvolume/shaders/boundsFs.glsl";*/
    constexpr const char* GlslRaycastPath =
        "${MODULES}/galaxy/shaders/galaxyraycast.glsl";
    constexpr const char* GlslBoundsVsPath =
        "${MODULES}/galaxy/shaders/raycasterbounds.vs";
    constexpr const char* GlslBoundsFsPath =
        "${MODULES}/galaxy/shaders/raycasterbounds.fs";
    constexpr const char* _loggerCat       = "Renderable Galaxy";

    constexpr openspace::properties::Property::PropertyInfo StepSizeInfo = {
        "StepSize",
        "Step Size",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo AbsorptionMultiplyInfo = {
        "AbsorptionMultiply",
        "Absorption Multiplier",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo EmissionMultiplyInfo = {
        "EmissionMultiply",
        "Emission Multiplier",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo PointStepSizeInfo = {
        "PointStepSize",
        "Point Step Size",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo TranslationInfo = {
        "Translation",
        "Translation",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo RotationInfo = {
        "Rotation",
        "Euler rotation",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo EnabledPointsRatioInfo = {
        "NEnabledPointsRatio",
        "Enabled points",
        "" // @TODO Missing documentation
    };
} // namespace

namespace openspace {

    RenderableGalaxy::RenderableGalaxy(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary)
    , _stepSize(StepSizeInfo, 0.01f, 0.0005f, 0.05f, 0.001f)
    , _absorptionMultiply(AbsorptionMultiplyInfo, 50.f, 0.0f, 500.0f)
    , _emissionMultiply(EmissionMultiplyInfo, 1500.f, 0.0f, 3000.0f)
    //, _pointStepSize(PointStepSizeInfo, 0.01f, 0.01f, 0.1f)
    //, _enabledPointsRatio(EnabledPointsRatioInfo, 0.2f, 0.f, 1.f)
    , _translation(TranslationInfo, glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f))
    , _rotation(RotationInfo, glm::vec3(0.f), glm::vec3(0.f), glm::vec3(6.28f))
{
    dictionary.getValue("StepSize", _stepSize);
    dictionary.getValue("AbsorptionMultiply", _absorptionMultiply);
    dictionary.getValue("EmissionMultiply", _emissionMultiply);
    dictionary.getValue("Translation", _translation);
    dictionary.getValue("Rotation", _rotation);

    if (dictionary.hasKeyAndValue<double>(StepSizeInfo.identifier)) {
        _stepSize = static_cast<float>(dictionary.value<double>(StepSizeInfo.identifier));
    }

    if (dictionary.hasKeyAndValue<double>(AbsorptionMultiplyInfo.identifier)) {
        _absorptionMultiply = static_cast<float>(dictionary.value<double>(AbsorptionMultiplyInfo.identifier));
    }

    if (dictionary.hasKeyAndValue<double>(EmissionMultiplyInfo.identifier)) {
        _emissionMultiply = static_cast<float>(dictionary.value<double>(EmissionMultiplyInfo.identifier));
    }

    if (dictionary.hasKeyAndValue<glm::vec3>(TranslationInfo.identifier)) {
        _translation = dictionary.value<glm::vec3>(TranslationInfo.identifier);
    }

    if (dictionary.hasKeyAndValue<glm::vec3>(RotationInfo.identifier)) {
        _rotation = dictionary.value<glm::vec3>(RotationInfo.identifier);
    }

    if (!dictionary.hasKeyAndValue<ghoul::Dictionary>("Volume")) {
        LERROR("No volume dictionary specified.");
    }

    ghoul::Dictionary volumeDictionary = dictionary.value<ghoul::Dictionary>("Volume");

    std::string volumeFilename;
    if (volumeDictionary.getValue("Filename", volumeFilename)) {
        _volumeFilename = absPath(volumeFilename);
    } else {
        LERROR("No volume filename specified.");
    }
    glm::vec3 volumeDimensions;
    if (volumeDictionary.getValue("Dimensions", volumeDimensions)) {
        _volumeDimensions = static_cast<glm::ivec3>(volumeDimensions);
    } else {
        LERROR("No volume dimensions specified.");
    }
    glm::vec3 volumeSize;
    if (volumeDictionary.getValue("Size", volumeSize)) {
        _volumeSize = volumeSize;
    }
    else {
        LERROR("No volume dimensions specified.");
    }

    /*if (!dictionary.hasKeyAndValue<ghoul::Dictionary>("Points")) {
        LERROR("No points dictionary specified.");
    }

    ghoul::Dictionary pointsDictionary = dictionary.value<ghoul::Dictionary>("Points");
    std::string pointsFilename;
    if (pointsDictionary.getValue("Filename", pointsFilename)) {
        _pointsFilename = absPath(pointsFilename);
    } else {
        LERROR("No points filename specified.");
    }
    pointsDictionary.getValue("Scaling", _pointScaling);*/
}

void RenderableGalaxy::initializeGL() {
    // Aspect is currently hardcoded to cubic voxels.
    _aspect = static_cast<glm::vec3>(_volumeDimensions);
    _aspect /= std::max(std::max(_aspect.x, _aspect.y), _aspect.z);

    volume::RawVolumeReader<glm::tvec4<GLubyte>> reader(
        _volumeFilename,
        _volumeDimensions
    );
    _volume = reader.read();

    _texture = std::make_unique<ghoul::opengl::Texture>(
        _volumeDimensions,
        ghoul::opengl::Texture::Format::RGBA,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        ghoul::opengl::Texture::FilterMode::Linear,
        ghoul::opengl::Texture::WrappingMode::Clamp);

    _texture->setPixelData(reinterpret_cast<char*>(
        _volume->data()),
        ghoul::opengl::Texture::TakeOwnership::No
    );

    _texture->setDimensions(_volume->dimensions());
    _texture->uploadTexture();

    _raycaster = std::make_unique<GalaxyRaycaster>(*_texture);
    _raycaster->initialize();

    global::raycasterManager.attachRaycaster(*_raycaster);

    auto onChange = [&](bool enabled) {
        if (enabled) {
            global::raycasterManager.attachRaycaster(*_raycaster);
        }
        else {
            global::raycasterManager.detachRaycaster(*_raycaster);
        }
    };

    onEnabledChange(onChange);

    addProperty(_stepSize);
    addProperty(_absorptionMultiply);
    addProperty(_emissionMultiply);
    //addProperty(_pointStepSize);
    //addProperty(_enabledPointsRatio);
    addProperty(_translation);
    addProperty(_rotation);

    // initialize points.
    /*std::ifstream pointFile(_pointsFilename, std::ios::in | std::ios::binary);

    std::vector<glm::vec3> pointPositions;
    std::vector<glm::vec3> pointColors;

    int64_t nPoints;
    pointFile.seekg(0, std::ios::beg); // read heder.
    pointFile.read(reinterpret_cast<char*>(&nPoints), sizeof(int64_t));

    _nPoints = static_cast<size_t>(nPoints);

    size_t nFloats = _nPoints * 7;

    std::vector<float> pointData(nFloats);
    pointFile.seekg(sizeof(int64_t), std::ios::beg); // read past heder.
    pointFile.read(reinterpret_cast<char*>(pointData.data()), nFloats * sizeof(float));
    pointFile.close();

    float maxdist = 0;

    for (size_t i = 0; i < _nPoints; ++i) {
        float x = pointData[i * 7 + 0];
        float y = pointData[i * 7 + 1];
        float z = pointData[i * 7 + 2];
        float r = pointData[i * 7 + 3];
        float g = pointData[i * 7 + 4];
        float b = pointData[i * 7 + 5];
        maxdist = std::max(maxdist, glm::length(glm::vec3(x, y, z)));
        //float a = pointData[i * 7 + 6];  alpha is not used.

        pointPositions.emplace_back(x, y, z);
        pointColors.emplace_back(r, g, b);
    }

    std::cout << maxdist << std::endl;

    glGenVertexArrays(1, &_pointsVao);
    glGenBuffers(1, &_positionVbo);
    glGenBuffers(1, &_colorVbo);

    glBindVertexArray(_pointsVao);
    glBindBuffer(GL_ARRAY_BUFFER, _positionVbo);
    glBufferData(GL_ARRAY_BUFFER,
        pointPositions.size() * sizeof(glm::vec3),
        pointPositions.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ARRAY_BUFFER, _colorVbo);
    glBufferData(GL_ARRAY_BUFFER,
        pointColors.size() * sizeof(glm::vec3),
        pointColors.data(),
        GL_STATIC_DRAW
    );

    _pointsProgram = global::renderEngine.buildRenderProgram(
        "Galaxy points",
        absPath("${MODULE_GALAXY}/shaders/points.vs"),
        absPath("${MODULE_GALAXY}/shaders/points.fs")
    );

    _pointsProgram->setIgnoreUniformLocationError(
        ghoul::opengl::ProgramObject::IgnoreError::Yes
    );

    GLint positionAttrib = _pointsProgram->attributeLocation("inPosition");
    GLint colorAttrib = _pointsProgram->attributeLocation("inColor");

    glBindBuffer(GL_ARRAY_BUFFER, _positionVbo);
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, _colorVbo);
    glEnableVertexAttribArray(colorAttrib);
    glVertexAttribPointer(colorAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);*/
}

void RenderableGalaxy::deinitializeGL() {
    if (_raycaster) {
        global::raycasterManager.detachRaycaster(*_raycaster);
        _raycaster = nullptr;
    }
}

bool RenderableGalaxy::isReady() const {
    return true;
}

void RenderableGalaxy::update(const UpdateData& data) {
    if (_raycaster) {
        //glm::mat4 transform = glm::translate(, static_cast<glm::vec3>(_translation));
        const glm::vec3 eulerRotation = static_cast<glm::vec3>(_rotation);
        glm::mat4 transform = glm::rotate(
            glm::mat4(1.0),
            eulerRotation.x,
            glm::vec3(1, 0, 0)
        );
        transform = glm::rotate(transform, eulerRotation.y, glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, eulerRotation.z,  glm::vec3(0, 0, 1));

        glm::mat4 volumeTransform = glm::scale(transform, _volumeSize);
        _pointTransform = glm::scale(transform, _pointScaling);

        const glm::vec4 translation = glm::vec4(_translation.value()*_volumeSize, 0.0);

        // Todo: handle floating point overflow, to actually support translation.

        volumeTransform[3] += translation;
        _pointTransform[3] += translation;

        _raycaster->setStepSize(_stepSize);
        _raycaster->setAspect(_aspect);
        _raycaster->setModelTransform(volumeTransform);
        _raycaster->setAbsorptionMultiplier(_absorptionMultiply);
        _raycaster->setEmissionMultiplier(_emissionMultiply);
        _raycaster->setTime(data.time.j2000Seconds());
    }
}

void RenderableGalaxy::render(const RenderData& data, RendererTasks& tasks) {
    RaycasterTask task { _raycaster.get(), data };

    const glm::vec3 position = data.camera.positionVec3();
    const float length = safeLength(position);
    const glm::vec3 galaxySize = _volumeSize;

    const float maxDim = std::max(std::max(galaxySize.x, galaxySize.y), galaxySize.z);

    const float lowerRampStart = maxDim * 0.02f;
    const float lowerRampEnd = maxDim * 0.5f;

    const float upperRampStart = maxDim * 2.f;
    const float upperRampEnd = maxDim * 10.f;

    float opacityCoefficient = 1.f;

    if (length < lowerRampStart) {
        opacityCoefficient = 0.f; // camera really close
    } else if (length < lowerRampEnd) {
        opacityCoefficient = (length - lowerRampStart) / (lowerRampEnd - lowerRampStart);
    } else if (length < upperRampStart) {
        opacityCoefficient = 1.f; // sweet spot (max)
    } else if (length < upperRampEnd) {
        opacityCoefficient = 1.f - (length - upperRampStart) /
                             (upperRampEnd - upperRampStart); //fade out
    } else {
        opacityCoefficient = 0;
    }

    _opacityCoefficient = opacityCoefficient;
    ghoul_assert(
        _opacityCoefficient >= 0.f && _opacityCoefficient <= 1.f,
        "Opacity coefficient was not between 0 and 1"
    );
    if (opacityCoefficient > 0) {
        _raycaster->setOpacityCoefficient(_opacityCoefficient);
        tasks.raycasterTasks.push_back(task);
    }
}

float RenderableGalaxy::safeLength(const glm::vec3& vector) const {
    const float maxComponent = std::max(
        std::max(std::abs(vector.x), std::abs(vector.y)), std::abs(vector.z)
    );
    return glm::length(vector / maxComponent) * maxComponent;
}

/*void RenderableGalaxy::postRender(const RenderData& data) {

    _raycaster->setStepSize(_pointStepSize);

    _pointsProgram->activate();
    setPscUniforms(*_pointsProgram.get(), data.camera, data.position);

    OsEng.ref().renderEngine().preRaycast(*_pointsProgram);

    glm::mat4 modelMatrix = _pointTransform;
    glm::mat4 viewMatrix = data.camera.viewMatrix();
    glm::mat4 projectionMatrix = data.camera.projectionMatrix();

    _pointsProgram->setUniform("model", modelMatrix);
    _pointsProgram->setUniform("view", viewMatrix);
    _pointsProgram->setUniform("projection", projectionMatrix);

    float emittanceFactor = _opacityCoefficient * static_cast<glm::vec3>(_volumeSize).x;
    _pointsProgram->setUniform("emittanceFactor",  emittanceFactor);

    glBindVertexArray(_pointsVao);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(false);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDrawArrays(GL_POINTS, 0, _nPoints * _enabledPointsRatio);
    glBindVertexArray(0);
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    OsEng.ref().renderEngine().postRaycast(*_pointsProgram);
}*/

} // namespace openspace
