/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2016                                                               *
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

#ifndef __GPUDATA_H__
#define __GPUDATA_H__


#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/texture.h>
#include <ghoul/opengl/textureunit.h>

#include <string>
#include <memory>

namespace openspace {

using namespace ghoul::opengl;



class UpdatableUniformLocation {
public:
    void updateUniformLocations(ProgramObject* program, const std::string& name);
    
protected:
    GLint _uniformLocation = -1;  
};

    
template<typename T>
class GPUData : public UpdatableUniformLocation{
public:
    
    void setValue(ProgramObject* program, T val){
        program->setUniform(_uniformLocation, val);
    }

};


class GPUTexture : public UpdatableUniformLocation{
public:
    void setValue(ProgramObject* program, std::shared_ptr<Texture> texture){
        _texUnit.activate();
        texture->bind();
        program->setUniform(_uniformLocation, _texUnit);
    }

private:

    TextureUnit _texUnit;
};

} // namespace openspace

#endif // __GPUDATA_H__
