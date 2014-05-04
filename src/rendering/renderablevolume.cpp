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
#include <openspace/rendering/renderablevolume.h>

#include <ghoul/opengl/texturereader.h>
#include <ghoul/filesystem/filesystem.h>

#include <openspace/engine/openspaceengine.h>
#include <openspace/util/kameleonwrapper.h>
#include <openspace/util/constants.h>

#include <sgct.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <type_traits>

namespace {
    const std::string _loggerCat = "RenderableVolume";
    
    bool hasExtension (std::string const &filepath, std::string const &extension)
    {
        std::string ending = "." + extension;
        if (filepath.length() > ending.length()) {
            return (0 == filepath.compare (filepath.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }
    
    template<typename T>
    T stringToNumber(const std::string& number, std::function<T(T)> f = nullptr) {
        static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value,
                      "Must be integral or floating point");
        std::stringstream ss;
        T templateNumber = {0};
        ss << number;
        ss >> templateNumber;
        if( ! f)
            return templateNumber;
        
        return f(templateNumber);
            
    }

}

namespace openspace {

RenderableVolume::RenderableVolume(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary)
{
    // get path if available
    _relativePath = "";
    if(dictionary.hasKey(constants::scenegraph::keyPathModule)) {
       dictionary.getValue(constants::scenegraph::keyPathModule, _relativePath);
       _relativePath += "/";
    }
}

RenderableVolume::~RenderableVolume() {
}

std::string RenderableVolume::findPath(const std::string& path) {
    std::string tmp = absPath(path);
    if(FileSys.fileExists(tmp))
        return tmp;
    
    tmp = absPath(_relativePath + path);
    if(FileSys.fileExists(tmp))
        return tmp;
    
    LERROR("Could not find file '" << path << "'");
    
    return "";
}

ghoul::opengl::Texture* RenderableVolume::loadVolume(const std::string& filepath, const ghoul::Dictionary& hintsDictionary) {
    if( ! FileSys.fileExists(filepath)) {
        LWARNING("Could not load volume, could not find '" << filepath << "'");
        return nullptr;
    }
    
    if(hasExtension(filepath, "raw")) {
        ghoul::RawVolumeReader::ReadHints hints = readHints(hintsDictionary);
        ghoul::RawVolumeReader rawReader(hints);
        return rawReader.read(filepath);
    } else if(hasExtension(filepath, "cdf")) {
        
        std::string modelString;
        if (hintsDictionary.hasKey("Model") && hintsDictionary.getValue("Model", modelString)) {
            KameleonWrapper::Model model;
            if (modelString == "BATSRUS") {
                model = KameleonWrapper::Model::BATSRUS;
            } else if (modelString == "ENLIL") {
                model = KameleonWrapper::Model::ENLIL;
            } else {
                LWARNING("Hints does not specify a valid 'Model'");
                return nullptr;
            }
            
            std::string variableString;
            if (hintsDictionary.hasKey("Variable") && hintsDictionary.getValue("Variable", variableString)) {
                glm::size3_t dimensions(1,1,1);
                double tempValue;
                if (hintsDictionary.hasKey("Dimensions.1") && hintsDictionary.getValue("Dimensions.1", tempValue)) {
                    int intVal = static_cast<int>(tempValue);
                    if(intVal > 0)
                        dimensions[0] = intVal;
                }
                if (hintsDictionary.hasKey("Dimensions.2") && hintsDictionary.getValue("Dimensions.2", tempValue)) {
                    int intVal = static_cast<int>(tempValue);
                    if(intVal > 0)
                        dimensions[1] = intVal;
                }
                if (hintsDictionary.hasKey("Dimensions.3") && hintsDictionary.getValue("Dimensions.3", tempValue)) {
                    int intVal = static_cast<int>(tempValue);
                    if(intVal > 0)
                        dimensions[2] = intVal;
                }
                
                KameleonWrapper kw(filepath, model);
                float* data = kw.getUniformSampledValues(variableString, dimensions);
                return new ghoul::opengl::Texture(data, dimensions, ghoul::opengl::Texture::Format::Red, GL_RED, GL_FLOAT);
            } else {
                LWARNING("Hints does not specify a 'Variable'");
            }
            
            
        }
        LWARNING("Hints does not specify a 'Model'");
    } else {
        LWARNING("No valid file extension.");
    }
    return nullptr;
}

ghoul::RawVolumeReader::ReadHints RenderableVolume::readHints(const ghoul::Dictionary& dictionary) {
    ghoul::RawVolumeReader::ReadHints hints;
    hints._dimensions = glm::ivec3(1, 1, 1);
    hints._format = ghoul::opengl::Texture::Format::Red;
    hints._internalFormat = GL_R8;
    
    // parse hints
    double tempValue;
    if (dictionary.hasKey("Dimensions.1") && dictionary.getValue("Dimensions.1", tempValue)) {
        int intVal = static_cast<int>(tempValue);
        if(intVal > 0)
            hints._dimensions[0] = intVal;
    }
    if (dictionary.hasKey("Dimensions.2") && dictionary.getValue("Dimensions.2", tempValue)) {
        int intVal = static_cast<int>(tempValue);
        if(intVal > 0)
            hints._dimensions[1] = intVal;
    }
    if (dictionary.hasKey("Dimensions.3") && dictionary.getValue("Dimensions.3", tempValue)) {
        int intVal = static_cast<int>(tempValue);
        if(intVal > 0)
            hints._dimensions[2] = intVal;
    }
    
    std::string format;
    if (dictionary.hasKey("Format") && dictionary.getValue("Format", format)) {
        if(format == "RED") {
            hints._format = ghoul::opengl::Texture::Format::Red;
        } else if(format == "RG") {
            hints._format = ghoul::opengl::Texture::Format::RG;
        } else if(format == "RGB") {
            hints._format = ghoul::opengl::Texture::Format::RGB;
        } else if(format == "RGBA") {
            hints._format = ghoul::opengl::Texture::Format::RGBA;
        }
    }
    
    format = "";
    if (dictionary.hasKey("InternalFormat") && dictionary.getValue("InternalFormat", format)) {
        if(format == "R8") {
            hints._internalFormat = GL_R8;
        } else if(format == "RG8") {
            hints._internalFormat = GL_RG8;
        } else if(format == "RGB8") {
            hints._internalFormat = GL_RGB8;
        } else if(format == "RGBA8") {
            hints._internalFormat = GL_RGB8;
        } else if(format == "R32F") {
            hints._internalFormat = GL_R32F;
        } else if(format == "RG32F") {
            hints._internalFormat = GL_RG32F;
        } else if(format == "RGB32F") {
            hints._internalFormat = GL_RGB32F;
        } else if(format == "RGBA32F") {
            hints._internalFormat = GL_RGB32F;
        }
    }
    return hints;
}
    
ghoul::opengl::Texture* RenderableVolume::loadTransferFunction(const std::string& filepath) {

    std::string f = absPath(filepath);

    if ( ! FileSys.fileExists(f)) {
        return nullptr;
    }
    
    // check if not a txt based texture
    if ( ! hasExtension(filepath, "txt")) {
        return ghoul::opengl::loadTexture(f);
    }
    
    // it is a txt based texture
    std::ifstream in;
    in.open(filepath.c_str());
    if (!in.is_open()) {
        LERROR("Could not open file " << f);
        return nullptr;
    }
    
    int width = 512;
    float lower = 0.0f;
    float upper = 1.0f;
    
    struct mappingKey {
        float position{0.0f};
        glm::vec4 color{0.0f,0.0f,0.0f,0.0f};
        
        mappingKey(float p, const glm::vec4& c): position(p), color(c){};
        mappingKey(float p): position(p), color(glm::vec4(0.0f)){};
        bool operator<(const mappingKey& rhs) {return position < rhs.position;};
    };
    
    std::vector<mappingKey> mappingKeys;
    
    auto widthValidator = [](size_t in) { if(in > 0) return in; return static_cast<size_t>(1); };
    auto upperLowerValidator = [](float in) { return glm::clamp(in, 0.0f, 1.0f); };
    auto intensityValidator = [](float in) { if(in > 0.0f) return in; return 1.0f; };
    
    std::string line;
    while (std::getline(in, line)) {
        
        float intensity = 1.0f;
        glm::vec4 rgba = glm::vec4(0.0f);
        // tokenize the line
        std::istringstream iss(line);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},std::istream_iterator<std::string>{}};
        
        size_t tokenSize =tokens.size();
        if (tokenSize > 0) {
            std::string key = tokens.at(0);
            if(key == "width" && tokenSize == 2) {
                width = stringToNumber<int>(tokens.at(1),widthValidator);
            } else if(key == "lower" && tokenSize == 2) {
                lower = stringToNumber<float>(tokens.at(1),upperLowerValidator);
            } else if(key == "upper" && tokenSize == 2) {
                upper = stringToNumber<float>(tokens.at(1),upperLowerValidator);
            } else if(key == "mappingkey" && tokenSize == 6) {
                intensity = stringToNumber<float>(tokens.at(1), intensityValidator);
                for(int i = 0; i < 4; ++i)
                    rgba[i] = stringToNumber<float>(tokens.at(i+2));

                mappingKeys.push_back({intensity, rgba});
            }
        }
    }
    in.close();
    

    if (mappingKeys.size() < 1) {
        return nullptr;
    }
    
    mappingKeys.insert(mappingKeys.begin(), {lower});
    mappingKeys.push_back({upper});
    
    
    for(auto key: mappingKeys) {
        glm::vec4 rgba = key.color;
//        LDEBUG("i: " << key.position << ", rgba: (" << rgba[0] << ", " << rgba[1] << ", " << rgba[2] << ", " << rgba[3] << ")");
    }
    
    // allocate new float array with zeros
    float* transferFunction = new float[width*4]();
    for (int i = 0; i < 4*width; ++i) {
        transferFunction[i] = 0.0f;
    }
    
    size_t lowerIndex = static_cast<size_t>(floorf(lower*static_cast<float>(width)));
    size_t upperIndex = static_cast<size_t>(floorf(upper*static_cast<float>(width)));
    
//    LDEBUG("width: " << width);
//    LDEBUG("lower: " << lower);
//    LDEBUG("upper: " << upper);
//    LDEBUG("lowerIndex: " << lowerIndex);
//    LDEBUG("upperIndex: " << upperIndex);
    
    
    auto prevKey = mappingKeys.begin();
    auto currentKey = prevKey + 1;
    auto lastKey = mappingKeys.end() -1;
    
    for (size_t i=lowerIndex; i<upperIndex; i++) {
        
        float fpos = static_cast<float>(i)/static_cast<float>(width);
        
        if (fpos > (*currentKey).position) {
            prevKey = currentKey;
            currentKey++;
            if (currentKey == mappingKeys.end()) {
                currentKey = lastKey;
            }
        }
        
        float dist = fpos-(*prevKey).position;
        float weight = dist/((*currentKey).position-(*prevKey).position);
        
        //LDEBUG("fpos: " << fpos);
        //LDEBUG("(*prevKey).position: " << (*prevKey).position);
        //LDEBUG("(*currentKey).position: " << (*currentKey).position);
        //LDEBUG("weight: " << weight);
        
        for (size_t channel=0; channel<4; ++channel) {
            size_t position = 4*i + channel;
            // Interpolate linearly between prev and next mapping key
            
            //LDEBUG("i: " << i);
            //LDEBUG("position: " << position);
            //LDEBUG("(*prevKey).first " << (*prevKey).first);
            //LDEBUG("(*currentKey).first " << (*currentKey).first);
            //LDEBUG("dist: " << dist);
            //LDEBUG("weight: " << weight);
            float value =
            ((*prevKey).color[channel]*(1.f-weight) + (*currentKey).color[channel]*weight)/255.f;
            transferFunction[position] = value;
            
            //LDEBUG("["<< position <<"] " << value);
            
        }
//        LDEBUG(weight << ", (" <<
//               transferFunction[4*i+0] << ", " <<
//               transferFunction[4*i+1] << ", " <<
//               transferFunction[4*i+2] << ", " <<
//               transferFunction[4*i+3] << ")");
    }

    return new ghoul::opengl::Texture(transferFunction,
    		glm::size3_t(width,1,1),ghoul::opengl::Texture::Format::RGBA,
    		GL_RGBA, GL_FLOAT);;
}

} // namespace openspace
