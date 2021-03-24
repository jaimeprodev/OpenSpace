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

#include <modules/base/rendering/renderablemodel.h>

#include <modules/base/basemodule.h>
#include <openspace/documentation/documentation.h>
#include <openspace/documentation/verifier.h>
#include <openspace/engine/globals.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/util/time.h>
#include <openspace/util/timeconversion.h>
#include <openspace/util/updatestructures.h>
#include <openspace/scene/scene.h>
#include <openspace/scene/lightsource.h>
#include <ghoul/io/model/modelgeometry.h>
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/misc/invariants.h>
#include <ghoul/misc/profiling.h>
#include <ghoul/opengl/openglstatecache.h>
#include <ghoul/opengl/programobject.h>
#include <filesystem>
#include <optional>

namespace {
    constexpr const char* _loggerCat = "RenderableModel";
    constexpr const char* ProgramName = "ModelProgram";

    constexpr const int DefaultBlending = 0;
    constexpr const int AdditiveBlending = 1;
    constexpr const int PointsAndLinesBlending = 2;
    constexpr const int PolygonBlending = 3;
    constexpr const int ColorAddingBlending = 4;

    std::map<std::string, int> BlendingMapping = {
        { "Default", DefaultBlending },
        { "Additive", AdditiveBlending },
        { "Points and Lines", PointsAndLinesBlending },
        { "Polygon", PolygonBlending },
        { "Color Adding", ColorAddingBlending }
    };

    constexpr openspace::properties::Property::PropertyInfo enableAnimationInfo = {
        "EnableAnimation",
        "Enable Animation",
        "Enable Animation"
    };

    constexpr const std::array<const char*, 12> UniformNames = {
        "opacity", "nLightSources", "lightDirectionsViewSpace", "lightIntensities",
        "modelViewTransform", "normalTransform", "projectionTransform",
        "performShading", "ambientIntensity", "diffuseIntensity",
        "specularIntensity", "opacityBlending"
    };

    constexpr openspace::properties::Property::PropertyInfo AmbientIntensityInfo = {
        "AmbientIntensity",
        "Ambient Intensity",
        "A multiplier for ambient lighting."
    };

    constexpr openspace::properties::Property::PropertyInfo DiffuseIntensityInfo = {
        "DiffuseIntensity",
        "Diffuse Intensity",
        "A multiplier for diffuse lighting."
    };

    constexpr openspace::properties::Property::PropertyInfo SpecularIntensityInfo = {
        "SpecularIntensity",
        "Specular Intensity",
        "A multiplier for specular lighting."
    };

    constexpr openspace::properties::Property::PropertyInfo ShadingInfo = {
        "PerformShading",
        "Perform Shading",
        "This value determines whether this model should be shaded by using the position "
        "of the Sun."
    };

    constexpr openspace::properties::Property::PropertyInfo DisableFaceCullingInfo = {
        "DisableFaceCulling",
        "Disable Face Culling",
        "Disable OpenGL automatic face culling optimization."
    };

    constexpr openspace::properties::Property::PropertyInfo ModelTransformInfo = {
        "ModelTransform",
        "Model Transform",
        "This value specifies the model transform that is applied to the model before "
        "all other transformations are applied."
    };

    constexpr openspace::properties::Property::PropertyInfo RotationVecInfo = {
        "RotationVector",
        "Rotation Vector",
        "Rotation Vector using degrees"
    };

    constexpr openspace::properties::Property::PropertyInfo LightSourcesInfo = {
        "LightSources",
        "Light Sources",
        "A list of light sources that this model should accept light from."
    };

    constexpr openspace::properties::Property::PropertyInfo DisableDepthTestInfo = {
        "DisableDepthTest",
        "Disable Depth Test",
        "Disable Depth Testing for the Model."
    };

    constexpr openspace::properties::Property::PropertyInfo BlendingOptionInfo = {
        "BlendingOption",
        "Blending Options",
        "Debug option for blending colors."
    };

    constexpr openspace::properties::Property::PropertyInfo EnableOpacityBlendingInfo = {
        "EnableOpacityBlending",
        "Enable Opacity Blending",
        "Enable Opacity Blending."
    };

    struct [[codegen::Dictionary(RenderableModel)]] Parameters {
        // The file or files that should be loaded in this RenderableModel. The file can
        // contain filesystem tokens or can be specified relatively to the
        // location of the .mod file.
        // This specifies the model that is rendered by the Renderable.
        std::filesystem::path geometryFile;

        enum class ScaleUnit {
            Nanometer,
            Micrometer,
            Millimeter,
            Centimeter,
            Decimeter,
            Meter,
            Kilometer
        };

        // The scale of the model. For example if the model is in centimeters
        // then ModelScale=cm
        std::optional<ScaleUnit> modelScale;

        // Set if invisible parts (parts with no textures or materials) of the model
        // should be forced to render or not.
        std::optional<bool> forceRenderInvisible;

        // [[codegen::verbatim(enableAnimationInfo.description)]]
        std::optional<bool> enableAnimation;

        // The date and time that the model animation should start.
        // In format 'YYYY MM DD hh:mm:ss'.
        std::optional<std::string> animationStartTime [[codegen::dateTime()]];

        enum class AnimationTimeUnit {
            Nanosecond,
            Microsecond,
            Millisecond,
            Second,
            Minute
        };

        // The time scale for the animation relative to seconds.
        // Ex, if animation is in milliseconds then AnimationTimeScale = 0.001 or
        // AnimationTimeScale = Millisecond, default is Second
        std::optional<std::variant<AnimationTimeUnit, float>> animationTimeScale;

        enum class AnimationMode {
            Once,
            LoopFromStart,
            LoopInfinitely,
            BounceFromStart,
            BounceInfinitely
        };

        // The mode of how the animation should be played back.
        // Default is animation is played back once at the start time
        std::optional<AnimationMode> animationMode;

        // [[codegen::verbatim(AmbientIntensityInfo.description)]]
        std::optional<double> ambientIntensity;

        // [[codegen::verbatim(DiffuseIntensityInfo.description)]]
        std::optional<double> diffuseIntensity;

        // [[codegen::verbatim(SpecularIntensityInfo.description)]]
        std::optional<double> specularIntensity;

        // [[codegen::verbatim(ShadingInfo.description)]]
        std::optional<bool> performShading;

        // [[codegen::verbatim(DisableFaceCullingInfo.description)]]
        std::optional<bool> disableFaceCulling;

        // [[codegen::verbatim(ModelTransformInfo.description)]]
        std::optional<glm::dmat3x3> modelTransform;

        // [[codegen::verbatim(RotationVecInfo.description)]]
        std::optional<glm::dvec3> rotationVector;

        // [[codegen::verbatim(LightSourcesInfo.description)]]
        std::optional<std::vector<ghoul::Dictionary>> lightSources [[codegen::reference("core_light_source")]];

        // [[codegen::verbatim(DisableDepthTestInfo.description)]]
        std::optional<bool> disableDepthTest;

        // [[codegen::verbatim(BlendingOptionInfo.description)]]
        std::optional<std::string> blendingOption;

        // [[codegen::verbatim(EnableOpacityBlendingInfo.description)]]
        std::optional<bool> enableOpacityBlending;
    };
#include "renderablemodel_codegen.cpp"
} // namespace

namespace openspace {

documentation::Documentation RenderableModel::Documentation() {
    documentation::Documentation doc = codegen::doc<Parameters>();
    doc.id = "base_renderable_model";
    return doc;
}

RenderableModel::RenderableModel(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary)
    , _enableAnimation(enableAnimationInfo, false)
    , _ambientIntensity(AmbientIntensityInfo, 0.2f, 0.f, 1.f)
    , _diffuseIntensity(DiffuseIntensityInfo, 1.f, 0.f, 1.f)
    , _specularIntensity(SpecularIntensityInfo, 1.f, 0.f, 1.f)
    , _performShading(ShadingInfo, true)
    , _disableFaceCulling(DisableFaceCullingInfo, false)
    , _modelTransform(
        ModelTransformInfo,
        glm::dmat3(1.0),
        glm::dmat3(-1.0),
        glm::dmat3(1.0)
    )
    , _rotationVec(RotationVecInfo, glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(360.0))
    , _enableOpacityBlending(EnableOpacityBlendingInfo, false)
    , _disableDepthTest(DisableDepthTestInfo, false)
    , _blendingFuncOption(
        BlendingOptionInfo,
        properties::OptionProperty::DisplayType::Dropdown
    )
    , _lightSourcePropertyOwner({ "LightSources", "Light Sources" })
{
    const Parameters p = codegen::bake<Parameters>(dictionary);

    addProperty(_opacity);
    registerUpdateRenderBinFromOpacity();

    if (p.forceRenderInvisible.has_value()) {
        _forceRenderInvisible = *p.forceRenderInvisible;

        if (!_forceRenderInvisible) {
            // Asset file have specifically said to not render invisible parts,
            // do not notify in the log if invisible parts are detected and dropped
            _notifyInvisibleDropped = false;
        }
    }

    // Import Model from file
    std::string file;
    file = absPath(p.geometryFile.string());
    _geometry = ghoul::io::ModelReader::ref().loadModel(
        file,
        ghoul::io::ModelReader::ForceRenderInvisible(_forceRenderInvisible),
        ghoul::io::ModelReader::NotifyInvisibleDropped(_notifyInvisibleDropped)
    );

    if (p.modelScale.has_value()) {
        Parameters::ScaleUnit scaleUnit = *p.modelScale;

        switch (scaleUnit) {
            case Parameters::ScaleUnit::Nanometer:
                _modelScale = DistanceUnit::Nanometer;
                break;
            case Parameters::ScaleUnit::Micrometer:
                _modelScale = DistanceUnit::Micrometer;
                break;
            case Parameters::ScaleUnit::Millimeter:
                _modelScale = DistanceUnit::Millimeter;
                break;
            case Parameters::ScaleUnit::Centimeter:
                _modelScale = DistanceUnit::Centimeter;
                break;
            case Parameters::ScaleUnit::Decimeter:
                _modelScale = DistanceUnit::Decimeter;
                break;
            case Parameters::ScaleUnit::Meter:
                _modelScale = DistanceUnit::Meter;
                break;
            case Parameters::ScaleUnit::Kilometer:
                _modelScale = DistanceUnit::Kilometer;
                break;
            default:
                throw ghoul::MissingCaseException();
        }

        _scaleVector = glm::dvec3(convertUnit(_modelScale, DistanceUnit::Meter));
    }

    if (p.animationStartTime.has_value()) {
        if (!_geometry->hasAnimation()) {
            LWARNING("Animation start time given to model without animation");
        }
        _animationStart = *p.animationStartTime;
        _enableAnimation = true;
    }

    if (p.enableAnimation.has_value()) {
        if (!_geometry->hasAnimation()) {
            LWARNING("Attempting to enable animation for a model that does not have any");
        }
        else if (*p.enableAnimation &&_animationStart == "") {
            LWARNING("Cannot enable animation without a given start time");
        }
        else {
            _enableAnimation = *p.enableAnimation;
            _geometry->enableAnimation(_enableAnimation.value());
        }
    }

    if (p.animationTimeScale.has_value()) {
        if (!_geometry->hasAnimation()) {
            LWARNING("Animation time scale given to model without animation");
        }
        else if (std::holds_alternative<float>(*p.animationTimeScale)) {
            _geometry->setTimeScale(std::get<float>(*p.animationTimeScale));
        }
        else if (std::holds_alternative<Parameters::AnimationTimeUnit>(*p.animationTimeScale)) {
            Parameters::AnimationTimeUnit animationTimeUnit =
                std::get<Parameters::AnimationTimeUnit>(*p.animationTimeScale);
            TimeUnit timeUnit;

            switch (animationTimeUnit) {
                case Parameters::AnimationTimeUnit::Nanosecond:
                    timeUnit = TimeUnit::Nanosecond;
                    break;
                case Parameters::AnimationTimeUnit::Microsecond:
                    timeUnit = TimeUnit::Microsecond;
                    break;
                case Parameters::AnimationTimeUnit::Millisecond:
                    timeUnit = TimeUnit::Millisecond;
                    break;
                case Parameters::AnimationTimeUnit::Second:
                    timeUnit = TimeUnit::Second;
                    break;
                case Parameters::AnimationTimeUnit::Minute:
                    timeUnit = TimeUnit::Minute;
                    break;
                default:
                    throw ghoul::MissingCaseException();
            }

            _geometry->setTimeScale(convertTime(1.0, timeUnit, TimeUnit::Second));
        }
    }

    if (p.animationMode.has_value()) {
        if (!_geometry->hasAnimation()) {
            LWARNING("Animation mode given to model without animation");
        }

        switch (*p.animationMode) {
        case Parameters::AnimationMode::LoopFromStart:
            _animationMode = AnimationMode::LoopFromStart;
            break;
        case Parameters::AnimationMode::LoopInfinitely:
            _animationMode = AnimationMode::LoopInfinitely;
            break;
        case Parameters::AnimationMode::BounceFromStart:
            _animationMode = AnimationMode::BounceFromStart;
            break;
        case Parameters::AnimationMode::BounceInfinitely:
            _animationMode = AnimationMode::BounceInfinitely;
            break;
        case Parameters::AnimationMode::Once:
        default:
            _animationMode = AnimationMode::Once;
            break;
        }
    }

    if (p.modelTransform.has_value()) {
        _modelTransform = *p.modelTransform;
    }

    if (p.ambientIntensity.has_value()) {
        _ambientIntensity = *p.ambientIntensity;
    }
    if (p.diffuseIntensity.has_value()) {
        _diffuseIntensity = *p.diffuseIntensity;
    }
    if (p.specularIntensity.has_value()) {
        _specularIntensity = *p.specularIntensity;
    }

    if (p.performShading.has_value()) {
        _performShading = *p.performShading;
    }

    if (p.disableDepthTest.has_value()) {
        _disableDepthTest = *p.disableDepthTest;
    }

    if (p.disableFaceCulling.has_value()) {
        _disableFaceCulling = *p.disableFaceCulling;
    }

    if (p.lightSources.has_value()) {
        std::vector<ghoul::Dictionary> lightsources = *p.lightSources;

        for (const ghoul::Dictionary& lsDictionary : lightsources) {
            std::unique_ptr<LightSource> lightSource =
                LightSource::createFromDictionary(lsDictionary);
            _lightSourcePropertyOwner.addPropertySubOwner(lightSource.get());
            _lightSources.push_back(std::move(lightSource));
        }
    }

    if (_geometry->hasAnimation()) {
        addProperty(_enableAnimation);
    }

    addPropertySubOwner(_lightSourcePropertyOwner);
    addProperty(_ambientIntensity);
    addProperty(_diffuseIntensity);
    addProperty(_specularIntensity);
    addProperty(_performShading);
    addProperty(_disableFaceCulling);
    addProperty(_disableDepthTest);
    addProperty(_modelTransform);
    addProperty(_rotationVec);

    _rotationVec.onChange([this]() {
        _modelTransform = glm::mat4_cast(glm::quat(glm::radians(_rotationVec.value())));
    });

    _enableAnimation.onChange([this]() {
        if (!_geometry->hasAnimation()) {
            LWARNING("Attempting to enable animation for a model that does not have any");
        }
        else if (_enableAnimation.value() && _animationStart == "") {
            LWARNING("Cannot enable animation without a given start time");
            _enableAnimation = false;
        }
        else {
            _geometry->enableAnimation(_enableAnimation.value());
        }
    });


    if (p.rotationVector.has_value()) {
        _rotationVec = *p.rotationVector;
    }

    _blendingFuncOption.addOption(DefaultBlending, "Default");
    _blendingFuncOption.addOption(AdditiveBlending, "Additive");
    _blendingFuncOption.addOption(PointsAndLinesBlending, "Points and Lines");
    _blendingFuncOption.addOption(PolygonBlending, "Polygon");
    _blendingFuncOption.addOption(ColorAddingBlending, "Color Adding");

    addProperty(_blendingFuncOption);

    if (p.blendingOption.has_value()) {
        const std::string blendingOpt = *p.blendingOption;
        _blendingFuncOption.set(BlendingMapping[blendingOpt]);
    }

    if (p.enableOpacityBlending.has_value()) {
        _enableOpacityBlending = *p.enableOpacityBlending;
    }

    addProperty(_enableOpacityBlending);
}

bool RenderableModel::isReady() const {
    return _program;
}

void RenderableModel::initialize() {
    ZoneScoped

    if (_geometry->hasAnimation() && _enableAnimation.value() && _animationStart == "") {
        LWARNING("Model with animation not given any start time");
    }
    else if (_geometry->hasAnimation() && !_enableAnimation.value()) {
        LINFO("Model with deactivated animation was found. "
            "The animation could be activated by entering a start time in the asset file"
        );
    }

    for (const std::unique_ptr<LightSource>& ls : _lightSources) {
        ls->initialize();
    }
}

void RenderableModel::initializeGL() {
    ZoneScoped

    _program = BaseModule::ProgramObjectManager.request(
        ProgramName,
        []() -> std::unique_ptr<ghoul::opengl::ProgramObject> {
            return global::renderEngine->buildRenderProgram(
                ProgramName,
                absPath("${MODULE_BASE}/shaders/model_vs.glsl"),
                absPath("${MODULE_BASE}/shaders/model_fs.glsl")
            );
        }
    );

    ghoul::opengl::updateUniformLocations(*_program, _uniformCache, UniformNames);

    _geometry->initialize();
    _geometry->calculateBoundingRadius();
    setBoundingSphere(glm::sqrt(_geometry->boundingRadius()));
}

void RenderableModel::deinitializeGL() {
    _geometry->deinitialize();
    _geometry.reset();

    BaseModule::ProgramObjectManager.release(
        ProgramName,
        [](ghoul::opengl::ProgramObject* p) {
            global::renderEngine->removeRenderProgram(p);
        }
    );
    _program = nullptr;
}

void RenderableModel::render(const RenderData& data, RendererTasks&) {
    _program->activate();

    _program->setUniform(_uniformCache.opacity, _opacity);

    // Model transform and view transform needs to be in double precision
    const glm::dmat4 modelTransform =
        glm::translate(glm::dmat4(1.0), data.modelTransform.translation) * // Translation
        glm::dmat4(data.modelTransform.rotation) *  // Spice rotation
        glm::scale(glm::dmat4(1.0), glm::dvec3(data.modelTransform.scale)) *
        glm::scale(glm::dmat4(_modelTransform.value()), _scaleVector); // Model scale unit
    const glm::dmat4 modelViewTransform = data.camera.combinedViewMatrix() *
                                          modelTransform;

    int nLightSources = 0;
    _lightIntensitiesBuffer.resize(_lightSources.size());
    _lightDirectionsViewSpaceBuffer.resize(_lightSources.size());
    for (const std::unique_ptr<LightSource>& lightSource : _lightSources) {
        if (!lightSource->isEnabled()) {
            continue;
        }
        _lightIntensitiesBuffer[nLightSources] = lightSource->intensity();
        _lightDirectionsViewSpaceBuffer[nLightSources] =
            lightSource->directionViewSpace(data);

        ++nLightSources;
    }

    _program->setUniform(
        _uniformCache.nLightSources,
        nLightSources
    );
    _program->setUniform(
        _uniformCache.lightIntensities,
        _lightIntensitiesBuffer
    );
    _program->setUniform(
        _uniformCache.lightDirectionsViewSpace,
        _lightDirectionsViewSpaceBuffer
    );
    _program->setUniform(
        _uniformCache.modelViewTransform,
        glm::mat4(modelViewTransform)
    );

    glm::dmat4 normalTransform = glm::transpose(glm::inverse(modelViewTransform));

    _program->setUniform(
        _uniformCache.normalTransform,
        glm::mat4(normalTransform)
    );

    _program->setUniform(
        _uniformCache.projectionTransform,
        data.camera.projectionMatrix()
    );
    _program->setUniform(_uniformCache.ambientIntensity, _ambientIntensity);
    _program->setUniform(_uniformCache.diffuseIntensity, _diffuseIntensity);
    _program->setUniform(_uniformCache.specularIntensity, _specularIntensity);
    _program->setUniform(_uniformCache.performShading, _performShading);
    _program->setUniform(_uniformCache.opacityBlending, _enableOpacityBlending);

    if (_disableFaceCulling) {
        glDisable(GL_CULL_FACE);
    }

    glEnablei(GL_BLEND, 0);
    switch (_blendingFuncOption) {
        case DefaultBlending:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case AdditiveBlending:
            glBlendFunc(GL_ONE, GL_ONE);
            break;
        case PointsAndLinesBlending:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case PolygonBlending:
            glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
            break;
        case ColorAddingBlending:
            glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR);
            break;
    };

    if (_disableDepthTest) {
        glDisable(GL_DEPTH_TEST);
    }

    _geometry->render(*_program);
    if (_disableFaceCulling) {
        glEnable(GL_CULL_FACE);
    }

    global::renderEngine->openglStateCache().resetBlendState();

    if (_disableDepthTest) {
        glEnable(GL_DEPTH_TEST);
    }

    _program->deactivate();
}

void RenderableModel::update(const UpdateData& data) {
    if (_program->isDirty()) {
        _program->rebuildFromFile();
        ghoul::opengl::updateUniformLocations(*_program, _uniformCache, UniformNames);
    }

    if (_geometry->hasAnimation() && _animationStart != "") {
        double realtiveTime;
        double now = data.time.j2000Seconds();
        double startTime = data.time.convertTime(_animationStart);
        double duration = _geometry->animationDuration();

        switch (_animationMode) {
            case AnimationMode::LoopFromStart:
                realtiveTime = std::fmod(now - startTime, duration);
                break;
            case AnimationMode::LoopInfinitely:
                realtiveTime = std::fmod(now - startTime, duration);
                if (realtiveTime < 0) {
                    realtiveTime += duration;
                }
                break;
            case AnimationMode::BounceFromStart:
                realtiveTime =
                    duration - abs(fmod(now - startTime, 2 * duration) - duration);
                break;
            case AnimationMode::BounceInfinitely: {
                double modulo = fmod(now - startTime, 2 * duration);
                if (modulo < 0) {
                    modulo += 2 * duration;
                }
                realtiveTime =
                    duration - abs(modulo - duration);
                break;
            }
            case AnimationMode::Once:
            default:
                realtiveTime = now - startTime;
                break;
        }
        _geometry->update(realtiveTime);
    }
}

}  // namespace openspace
