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

#ifndef __RENDERABLEVOLUMEGL_H__
#define __RENDERABLEVOLUMEGL_H__

// open space includes
#include <openspace/rendering/renderablevolume.h>

// ghoul includes
#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/texture.h>
#include <ghoul/opengl/framebufferobject.h>
#include <ghoul/io/rawvolumereader.h>
#include <ghoul/filesystem/file.h>

 namespace sgct_utils {
    class SGCTBox;
}

namespace openspace {

class RenderableVolumeGL: public RenderableVolume {
public:
	// constructors & destructor
	RenderableVolumeGL(const ghoul::Dictionary& dictionary);
	~RenderableVolumeGL();
    
    bool initialize();
    bool deinitialize();

	virtual void render(const Camera *camera, const psc& thisPosition);
	virtual void update();

private:
	ghoul::Dictionary _hintsDictionary;

    std::string _filename;
    std::string _transferFunctionPath;
	std::string _samplerFilename;
    
    ghoul::filesystem::File* _transferFunctionFile;

	ghoul::opengl::Texture* _volume;
	ghoul::opengl::Texture* _transferFunction;

	GLuint _boxArray;
	ghoul::opengl::ProgramObject *_boxProgram;
	sgct_utils::SGCTBox* _box;
	glm::vec3 _boxScaling;
	GLint _MVPLocation, _modelTransformLocation, _typeLocation;
    
    bool _updateTransferfunction;
    int _id;
};

} // namespace openspace

#endif
