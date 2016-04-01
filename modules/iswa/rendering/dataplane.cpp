// /*****************************************************************************************
//  *                                                                                       *
//  * OpenSpace                                                                             *
//  *                                                                                       *
//  * Copyright (c) 2014-2016                                                               *
//  *                                                                                       *
//  * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
//  * software and associated documentation files (the "Software"), to deal in the Software *
//  * without restriction, including without limitation the rights to use, copy, modify,    *
//  * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
//  * permit persons to whom the Software is furnished to do so, subject to the following   *
//  * conditions:                                                                           *
//  *                                                                                       *
//  * The above copyright notice and this permission notice shall be included in all copies *
//  * or substantial portions of the Software.                                              *
//  *                                                                                       *
//  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
//  * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
//  * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
//  * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
//  * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
//  * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
//  ****************************************************************************************/

#include <modules/iswa/rendering/dataplane.h>
#include <ghoul/filesystem/filesystem>
#include <ghoul/io/texture/texturereader.h>
#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/textureunit.h>
#include <modules/kameleon/include/kameleonwrapper.h>
#include <openspace/scene/scene.h>
#include <openspace/scene/scenegraphnode.h>
#include <openspace/engine/openspaceengine.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/util/spicemanager.h>


namespace {
	const std::string _loggerCat = "DataPlane";
}

namespace openspace {

DataPlane::DataPlane(std::shared_ptr<KameleonWrapper> kw) 
	:CygnetPlane()
	, _kw(kw)
{	
	_id = id();
	setName("DataPlane" + std::to_string(_id));
	registerProperties();

	_fileExtension = "";
	_path = "";
	_cygnetId.onChange([this](){ 
		_fileExtension = "";
		_path = "";
		ISWAManager::ref().fileExtension(_cygnetId.value(), &_fileExtension);
	});


	KameleonWrapper::Model model = _kw->model();
	if(	model == KameleonWrapper::Model::BATSRUS){
		_var = "p";
	}else{
		_var = "rho";
	}

}


DataPlane::~DataPlane(){}


bool DataPlane::initialize(){
	_modelScale = _kw->getModelScaleScaled();
	_pscOffset  = _kw->getModelBarycenterOffsetScaled();

	CygnetPlane::initialize();
	// std::cout << _modelScale.x << ", " << _modelScale.y << ", " << _modelScale.z << ", " << _modelScale.w << std::endl;
	// std::cout << _pscOffset.x << ", " << _pscOffset.y << ", " << _pscOffset.z << ", " << _pscOffset.w << std::endl;

	ISWAManager::ref().fileExtension(_cygnetId.value(), &_fileExtension);

	_dimensions = glm::size3_t(500,500,1);
	float zSlice = 0.5f;
	_dataSlice = _kw->getUniformSliceValues(std::string(_var), _dimensions, zSlice);

    loadTexture();

    return isReady();
}

bool DataPlane::deinitialize(){
	ISWACygnet::deinitialize();
	return true;
}



void DataPlane::render(){
	psc position = _parent->worldPosition();
	glm::mat4 transform = glm::mat4(1.0);

	glm::mat4 rotx = glm::rotate(transform, static_cast<float>(M_PI_2), glm::vec3(1, 0, 0));
	glm::mat4 roty = glm::rotate(transform, static_cast<float>(M_PI_2), glm::vec3(0, -1, 0));
	glm::mat4 rotz = glm::rotate(transform, static_cast<float>(M_PI_2), glm::vec3(0, 0, 1));

	glm::mat4 rot = glm::mat4(1.0);
	for (int i = 0; i < 3; i++){
		for (int j = 0; j < 3; j++){
			transform[i][j] = static_cast<float>(_stateMatrix[i][j]);
		}
	}

	transform = transform * rotz * roty; //BATSRUS
	// transform = transform * roty;

	// transform = glm::rotate(transform, _roatation.value()[0], glm::vec3(1,0,0));
	// transform = glm::rotate(transform, _roatation.value()[1], glm::vec3(0,1,0));
	// transform = glm::rotate(transform, _roatation.value()[2], glm::vec3(0,0,1));

	glm::vec4 v(1,0,0,1);
	glm::vec3 xVec = glm::vec3(transform*v);
	xVec = glm::normalize(xVec);

	double  lt;
    glm::vec3 sunVec =
    SpiceManager::ref().targetPosition("SUN", "Earth", "GALACTIC", {}, _time, lt);
    sunVec = glm::normalize(sunVec);

    float angle = acos(glm::dot(xVec, sunVec));
    glm::vec3 ref =  glm::cross(xVec, sunVec);

    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, ref); 
    transform = rotation * transform;
	position += transform*glm::vec4(_pscOffset.x, _pscOffset.z, _pscOffset.y, _pscOffset.w); 

	// Activate shader
	_shader->activate();
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);

	_shader->setUniform("ViewProjection", OsEng.renderEngine().camera()->viewProjectionMatrix());
	_shader->setUniform("ModelTransform", transform);
	setPscUniforms(_shader.get(), OsEng.renderEngine().camera(), position);

	ghoul::opengl::TextureUnit unit;
	unit.activate();
	_texture->bind();
	_shader->setUniform("texture1", unit);

	glBindVertexArray(_quad);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glEnable(GL_CULL_FACE);
	_shader->deactivate();


}

void DataPlane::update(){

	if(_path != ""){
		CygnetPlane::update();
	} else {
		if(_fileExtension == ""){
			//send new request
		} else{
			_path = "${OPENSPACE_DATA}/"+ name()+_fileExtension;
			updateTexture();
		}
	}
}

void DataPlane::setParent(){
	KameleonWrapper::Model model = _kw->model();
	if(	model == KameleonWrapper::Model::BATSRUS ||
		model == KameleonWrapper::Model::OpenGGCM ||
		model == KameleonWrapper::Model::LFM)
	{
		_parent = OsEng.renderEngine().scene()->sceneGraphNode("Earth");
		_frame = "GSM";
	}else if(
		model == KameleonWrapper::Model::ENLIL ||
		model == KameleonWrapper::Model::MAS ||
		model == KameleonWrapper::Model::Adapt3D ||
		model == KameleonWrapper::Model::SWMF)
	{
		_parent = OsEng.renderEngine().scene()->sceneGraphNode("SolarSystem");
		_frame = "GALACTIC";
	}else{
		//Warning!
	}
}



void DataPlane::loadTexture() {
        //std::unique_ptr<ghoul::opengl::Texture> texture = ghoul::io::TextureReader::ref().loadTexture(absPath(_texturePath));
		ghoul::opengl::Texture::FilterMode filtermode = ghoul::opengl::Texture::FilterMode::Linear;
		ghoul::opengl::Texture::WrappingMode wrappingmode = ghoul::opengl::Texture::WrappingMode::ClampToEdge;
		std::unique_ptr<ghoul::opengl::Texture> texture = 
			std::make_unique<ghoul::opengl::Texture>(_dataSlice, _dimensions, ghoul::opengl::Texture::Format::Red, GL_RED, GL_FLOAT, filtermode, wrappingmode);
		if (texture) {
			// LDEBUG("Loaded texture from '" << absPath(_path) << "'");

			texture->uploadTexture();

			// Textures of planets looks much smoother with AnisotropicMipMap rather than linear
			texture->setFilter(ghoul::opengl::Texture::FilterMode::Linear);

            _texture = std::move(texture);
		}
	
}

// void DataPlane::createPlane() {
//     // ============================
//     // 		GEOMETRY (quad)
//     // ============================
//     const GLfloat x = _modelScale.x/2.0;
//     const GLfloat y = _modelScale.z/2.0;
//     const GLfloat w = _modelScale.w;
//     const GLfloat vertex_data[] = { // square of two triangles (sigh)
//         //	  x      y     z     w     s     t
//         -x, -y, 0, w, 0, 1,
//          x,  y, 0, w, 1, 0,
//         -x,  y, 0, w, 0, 0,
//         -x, -y, 0, w, 0, 1,
//          x, -y, 0, w, 1, 1,
//          x,  y, 0, w, 1, 0,
//     };

//     glBindVertexArray(_quad); // bind array
//     glBindBuffer(GL_ARRAY_BUFFER, _vertexPositionBuffer); // bind buffer
//     glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, reinterpret_cast<void*>(0));
//     glEnableVertexAttribArray(1);
//     glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, reinterpret_cast<void*>(sizeof(GLfloat) * 4));
// }

void DataPlane::updateTexture(){}

int DataPlane::id(){
		static int id = 0;
		return id++;
}
}// namespace openspace