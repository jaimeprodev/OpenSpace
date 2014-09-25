/*****************************************************************************************
*                                                                                       *
* OpenSpace                                                                             *
*                                                                                       *
* Copyright (c) 2014                                                                    *
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

// open space includes
#include <openspace/rendering/planets/renderableplanet.h>
#include <openspace/util/constants.h>
#include <openspace/rendering/planets/planetgeometry.h>

#include <ghoul/opengl/texturereader.h>
#include <ghoul/opengl/textureunit.h>
#include <ghoul/filesystem/filesystem.h>

#include <openspace/util/time.h>
#include <openspace/util/spicemanager.h>

#include <openspace/engine/openspaceengine.h>
#include <sgct.h>

namespace {
const std::string _loggerCat = "RenderablePlanet";
}

namespace openspace {

RenderablePlanet::RenderablePlanet(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary)
    , _colorTexturePath("colorTexture", "Color Texture")
    , _programObject(nullptr)
    , _texture(nullptr)
    , _geometry(nullptr)
{
	std::string name;
	bool success = dictionary.getValue(constants::scenegraphnode::keyName, name);
	assert(success);

    std::string path;
    success = dictionary.getValue(constants::scenegraph::keyPathModule, path);
	assert(success);

    ghoul::Dictionary geometryDictionary;
    success = dictionary.getValueSafe(
		constants::renderableplanet::keyGeometry, geometryDictionary);
	if (success) {
		geometryDictionary.setValue(constants::scenegraphnode::keyName, name);
        geometryDictionary.setValue(constants::scenegraph::keyPathModule, path);
        _geometry
              = planetgeometry::PlanetGeometry::createFromDictionary(geometryDictionary);
	}

	dictionary.getValue("Frame", _target);

    // TODO: textures need to be replaced by a good system similar to the geometry as soon
    // as the requirements are fixed (ab)
    std::string texturePath = "";
	success = dictionary.getValueSafe("Textures.Color", texturePath);
	if (success)
        _colorTexturePath = path + "/" + texturePath;

	addPropertySubOwner(_geometry);

    addProperty(_colorTexturePath);
    _colorTexturePath.onChange(std::bind(&RenderablePlanet::loadTexture, this));
}

RenderablePlanet::~RenderablePlanet()
{
    deinitialize();
}

bool RenderablePlanet::initialize()
{
    bool completeSuccess = true;
    if (_programObject == nullptr)
        completeSuccess
              &= OsEng.ref().configurationManager().getValue("pscShader", _programObject);

    loadTexture();
    completeSuccess &= (_texture != nullptr);

    completeSuccess &= _geometry->initialize(this);

    return completeSuccess;
}

bool RenderablePlanet::deinitialize()
{
    _geometry->deinitialize();
    delete _geometry;
    _geometry = nullptr;
    delete _texture;
    _texture = nullptr;
    return true;
}

void RenderablePlanet::render(const Camera* camera, const psc& thisPosition, RuntimeData* runtimeData)
{
	if (!_programObject)
		return;
	if (!_texture)
		return;

    // activate shader
    _programObject->activate();

    // fetch data
    psc currentPosition = thisPosition;
	psc campos          = camera->position();
    glm::mat4 camrot    = camera->viewRotationMatrix();
   // PowerScaledScalar scaling = camera->scaling();

    PowerScaledScalar scaling = glm::vec2(1, -6);

    // scale the planet to appropriate size since the planet is a unit sphere
    glm::mat4 transform = glm::mat4(1);
	
	// set spice-orientation in accordance to timestamp
	glm::dmat3 stateMatrix;
	openspace::SpiceManager::ref().getPositionTransformMatrixGLM("GALACTIC", "IAU_EARTH", runtimeData->getTime(), stateMatrix);
	
	//earth needs to be rotated for that to work.
	glm::mat4 rot = glm::rotate(transform, 90.f, glm::vec3(1, 0, 0));
		
	for (int i = 0; i < 3; i++){
		for (int j = 0; j < 3; j++){
			transform[i][j] = stateMatrix[i][j];
		}
	}
	transform = transform* rot;
	

    // setup the data to the shader
    _programObject->setUniform("ViewProjection", camera->viewProjectionMatrix());
    _programObject->setUniform("ModelTransform", transform);
    _programObject->setUniform("campos", campos.vec4());
    _programObject->setUniform("objpos", currentPosition.vec4());
    _programObject->setUniform("camrot", camrot);
    _programObject->setUniform("scaling", scaling.vec2());
	
    // Bind texture
    ghoul::opengl::TextureUnit unit;
    unit.activate();
    _texture->bind();
    _programObject->setUniform("texture1", unit);

    // render
    _geometry->render();

    // disable shader
    _programObject->deactivate();

}

void RenderablePlanet::update()
{
}

void RenderablePlanet::loadTexture()
{
    delete _texture;
    _texture = nullptr;
    if (_colorTexturePath.value() != "") {
        _texture = ghoul::opengl::loadTexture(absPath(_colorTexturePath));
        if (_texture) {
            LDEBUG("Loaded texture from '" << absPath(_colorTexturePath) << "'");
            _texture->uploadTexture();
        }
    }
}

}  // namespace openspace
