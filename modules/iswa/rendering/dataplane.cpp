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
#include <ghoul/io/texture/texturereader.h>
#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/textureunit.h>
#include <openspace/scene/scene.h>
#include <openspace/scene/scenegraphnode.h>
#include <openspace/engine/openspaceengine.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/util/spicemanager.h>

namespace {
    const std::string _loggerCat = "DataPlane";
}

namespace openspace {

DataPlane::DataPlane(const ghoul::Dictionary& dictionary)
    :CygnetPlane(dictionary)
    ,_dataOptions("dataOptions", "Data Options")
    ,_normValues("normValues", "Normalize Values", glm::vec2(1.0,1.0), glm::vec2(0), glm::vec2(5.0))
    ,_backgroundValues("backgroundValues", "Background Values", glm::vec2(0.0), glm::vec2(0), glm::vec2(1.0))
    ,_useLog("useLog","Use Logarithm", false)
    ,_useHistogram("_useHistogram", "Use Histogram", true)
    ,_useRGB("useRGB","Use RGB Channels", false)
    ,_averageValues("averageValues", "Average values", false)
    // ,_colorbar(nullptr)
{     
    std::string name;
    dictionary.getValue("Name", name);
    setName(name);

    addProperty(_useLog);
    addProperty(_useHistogram);
    addProperty(_useRGB);
    addProperty(_normValues);
    addProperty(_backgroundValues);
    addProperty(_averageValues);
    addProperty(_dataOptions);

    registerProperties();

    OsEng.gui()._iSWAproperty.registerProperty(&_useLog);
    OsEng.gui()._iSWAproperty.registerProperty(&_useHistogram);
    OsEng.gui()._iSWAproperty.registerProperty(&_useRGB);
    OsEng.gui()._iSWAproperty.registerProperty(&_normValues);
    OsEng.gui()._iSWAproperty.registerProperty(&_backgroundValues);
    OsEng.gui()._iSWAproperty.registerProperty(&_averageValues);
    OsEng.gui()._iSWAproperty.registerProperty(&_dataOptions);
    
    _normValues.onChange([this](){loadTexture();});
    _useLog.onChange([this](){loadTexture();});
    _useHistogram.onChange([this](){loadTexture();});
    _dataOptions.onChange([this](){
        if( _useRGB.value() && (_dataOptions.value().size() > 3)){
            LWARNING("More than 3 values, using only the red channel.");
        }
        loadTexture();
    });

    _useRGB.onChange([this](){
            changeTransferFunctions(_useRGB.value());
    });
}

DataPlane::~DataPlane(){}


bool DataPlane::initialize(){
    initializeTime();

    createPlane();
    
    if (_shader == nullptr) {
    // DatePlane Program
    RenderEngine& renderEngine = OsEng.renderEngine();
    _shader = renderEngine.buildRenderProgram("DataPlaneProgram",
        "${MODULE_ISWA}/shaders/dataplane_vs.glsl",
        "${MODULE_ISWA}/shaders/dataplane_fs.glsl"
        );
    if (!_shader)
        return false;
    }

    updateTexture();

    std::string tfPath = "${OPENSPACE_DATA}/scene/iswa/transferfunctions/colormap_parula.jpg";
    _transferFunctions.push_back(std::make_shared<TransferFunction>(tfPath));
    
    // std::cout << "Creating Colorbar" << std::endl;
    // _colorbar = std::make_shared<ColorBar>();
    // if(_colorbar){
    //     _colorbar->initialize(); 
    // }

    // _textures.push_back(nullptr);

    return isReady();
}

bool DataPlane::deinitialize(){
    unregisterProperties();
    destroyPlane();
    destroyShader();

    _textures[0] = nullptr;
    _memorybuffer = "";
    
    // _colorbar->deinitialize();
    // _colorbar = nullptr;

    return true;
}

bool DataPlane::loadTexture() {
    std::vector<float*> data = readData();
    if(data.empty())
        return false;

    bool texturesReady = false;
    std::vector<int> selectedOptions = _dataOptions.value();

    for(int option: selectedOptions){
        float* values = data[option];
        if(!values) continue;

        if(!_textures[option]){
            std::unique_ptr<ghoul::opengl::Texture> texture =  std::make_unique<ghoul::opengl::Texture>(
                                                                    values, 
                                                                    _dimensions,
                                                                    ghoul::opengl::Texture::Format::Red,
                                                                    GL_RED, 
                                                                    GL_FLOAT,
                                                                    ghoul::opengl::Texture::FilterMode::Linear,
                                                                    ghoul::opengl::Texture::WrappingMode::ClampToEdge
                                                                );

            if(texture){
                texture->uploadTexture();
                texture->setFilter(ghoul::opengl::Texture::FilterMode::Linear);
                _textures[option] = std::move(texture);
            }
        }else{
            _textures[option]->setPixelData(values);
            _textures[option]->uploadTexture();
        }
        texturesReady = true;
    }

    return texturesReady;
}

bool DataPlane::updateTexture(){
    if(_futureObject)
        return false;

    _memorybuffer = "";
    std::shared_ptr<DownloadManager::FileFuture> future = ISWAManager::ref().downloadDataToMemory(_data->id, _memorybuffer);

    if(future){
        _futureObject = future;
        return true;
    }

    return false;
}

void DataPlane::setUniforms(){

    // _shader->setUniform("textures", 1, units[1]);
    // _shader->setUniform("textures", 2, units[2]);
    // }
    std::vector<int> selectedOptions = _dataOptions.value();
    int activeTextures = selectedOptions.size();

    int activeTransferfunctions = _transferFunctions.size();

    ghoul::opengl::TextureUnit txUnits[activeTextures];
    int j = 0;
    for(int option : selectedOptions){
        if(_textures[option]){
            txUnits[j].activate();
            _textures[option]->bind();
            _shader->setUniform(
                "textures[" + std::to_string(j) + "]",
                txUnits[j]
            );

            j++;
        }
    }

    ghoul::opengl::TextureUnit tfUnits[activeTransferfunctions];
    j = 0;

    if((activeTransferfunctions == 1) && (_textures.size() != _transferFunctions.size())) {
        tfUnits[0].activate();
        _transferFunctions[0]->bind();
        _shader->setUniform(
            "transferFunctions[0]",
            tfUnits[0]
        );
    }else{
        for(int option : selectedOptions){
            std::cout << option << std::endl;
            if(_transferFunctions[option]){
                tfUnits[j].activate();
                _transferFunctions[option]->bind();
                _shader->setUniform(
                "transferFunctions[" + std::to_string(j) + "]",
                tfUnits[j]
                );

                j++;
            }
        }
    }


    _shader->setUniform("numTextures", activeTextures);
    _shader->setUniform("numTransferFunctions", activeTransferfunctions);
    _shader->setUniform("backgroundValues", _backgroundValues.value());
};

bool DataPlane::textureReady(){
    return (!_textures.empty());
}

void DataPlane::readHeader(){
    if(!_memorybuffer.empty()){
        std::stringstream memorystream(_memorybuffer);
        std::string line;

        int numOptions = 0;
        while(getline(memorystream,line)){
            if(line.find("#") == 0){
                if(line.find("# Output data:") == 0){

                    line = line.substr(26);
                    std::stringstream ss(line);

                    std::string token;
                    getline(ss, token, 'x');
                    int x = std::stoi(token);

                    getline(ss, token, '=');
                    int y = std::stoi(token);

                    _dimensions = glm::size3_t(x, y, 1);

                    getline(memorystream, line);
                    line = line.substr(1);

                    ss = std::stringstream(line);
                    std::string option;
                    while(ss >> option){
                        if(option != "x" && option != "y" && option != "z"){
                            _dataOptions.addOption({numOptions, name()+"_"+option});
                            numOptions++;
                            _textures.push_back(nullptr);
                        }
                    }

                    std::vector<int> v(1,0);
                    _dataOptions.setValue(v);
                }
            }else{
                break;
            }
        }
    }else{
        LWARNING("Noting in memory buffer, are you connected to the information super highway?");
    }
}

std::vector<float*> DataPlane::readData(){
    if(!_memorybuffer.empty()){
        if(!_dataOptions.options().size()) // load options for value selection
            readHeader();

        std::stringstream memorystream(_memorybuffer);
        std::string line;

        std::vector<int> selectedOptions = _dataOptions.value();

        int numSelected = selectedOptions.size();

        std::vector<float> min(numSelected, std::numeric_limits<float>::max()); 
        std::vector<float> max(numSelected, std::numeric_limits<float>::min());

        std::vector<float> sum(numSelected, 0.0f);
        std::vector<std::vector<float>> optionValues(numSelected, std::vector<float>());

        std::vector<float*> data(_dataOptions.options().size(), nullptr);
        for(int option : selectedOptions){
            data[option] = new float[_dimensions.x*_dimensions.y]{0.0f};
        }
        
        int numValues = 0;
        while(getline(memorystream, line)){
            if(line.find("#") == 0){ //part of the header
                continue;
            }

            std::stringstream ss(line); 
            std::vector<float> value;
            float v;
            while(ss >> v){
                value.push_back(v);
            }

            if(value.size()){
                for(int i=0; i<numSelected; i++){

                    float v = value[selectedOptions[i]+3]; //+3 because "options" x, y and z.

                    if(_useLog.value()){
                        int sign = (v>0)? 1:-1;
                        if(v != 0){
                            v = sign*log(fabs(v));
                        }
                    }

                    optionValues[i].push_back(v); 

                    min[i] = std::min(min[i], v);
                    max[i] = std::max(max[i], v);

                    sum[i] += v;
                }
                numValues++;
            }
        }

        if(numValues != _dimensions.x*_dimensions.y){
            LWARNING("Number of values read and expected are not the same");
            return std::vector<float*>();
        }
        
        for(int i=0; i<numSelected; i++){
            processData(data, selectedOptions[i], optionValues[i], min[i], max[i], sum[i]);
        }
        
        return data;
        
    } 
    else {
        LWARNING("Nothing in memory buffer, are you connected to the information super highway?");
        return std::vector<float*>();
    }
} 

void DataPlane::processData(std::vector<float*> outputData, int inputChannel, std::vector<float> inputData, float min, float max,float sum){
    
    float* output = outputData[inputChannel];
    // HISTOGRAM
    // number of levels/bins/values
    const int levels = 512;    
    // Normal Histogram where "levels" is the number of steps/bins
    std::vector<int> histogram = std::vector<int>(levels, 0);
    // Maps the old levels to new ones. 
    std::vector<float> newLevels = std::vector<float>(levels, 0.0f);
    
    const int numValues = inputData.size(); 
    
    // maps the data values to the histogram bin/index/level
    auto mapToHistogram = [levels](float val, float varMin, float varMax) {
        float probability = (val-varMin)/(varMax-varMin);
        float mappedValue = probability * levels;
        return glm::clamp(mappedValue, 0.0f, static_cast<float>(levels - 1));
    };
    
    //Calculate the mean
    float mean = (1.0 / numValues) * sum;
    //Calculate the Standard Deviation
    float standardDeviation = sqrt (((pow(sum, 2.0)) - ((1.0/numValues) * (pow(sum,2.0)))) / (numValues - 1.0));
    //calulate log mean
    // logmean /= numValues;   
    
    //HISTOGRAM FUNCTIONALITY
    //======================
    if(_useHistogram.value()){
        for(int i = 0; i < numValues; i++){
            float v = inputData[i];
            float pixelVal = mapToHistogram(v, min, max);       
            histogram[(int)pixelVal]++;
            inputData[i] = pixelVal;
        }
        
        // Map mean and standard deviation to histogram levels
        mean = mapToHistogram(mean , min, max);
        // logmean = mapToHistogram(logmean , min, max);
        standardDeviation = mapToHistogram(standardDeviation, min, max);
        min = 0.0f;
        max = levels - 1.0f;

        //Calculate the cumulative distributtion function (CDF)
        float previousCdf = 0.0f;
        for(int i = 0; i < levels; i++){
            
            float probability = histogram[i] / (float)numValues; 
            float cdf  = previousCdf + probability;
            cdf = glm::clamp(cdf, 0.0f, 1.0f); //just in case
            newLevels[i] = cdf * (levels-1);
            previousCdf = cdf;
        }
    }
    //======================
    
    for(int i=0; i< numValues; i++){
        
        float v = inputData[i];
   
        // if use histogram get the equalized values
        if(_useHistogram.value()){
            v = newLevels[(int)v];
            
            // Map mean and standard deviation to new histogram levels
            mean =  newLevels[(int) mean];
            // logmean =  newLevels[(int) logmean];
            standardDeviation = newLevels[(int) standardDeviation];
        }
        
        v = normalizeWithStandardScore(v, mean, standardDeviation);         
        output[i] += v;

    }
}

float DataPlane::normalizeWithStandardScore(float value, float mean, float sd){
    
    float zScoreMin = _normValues.value().x;
    float zScoreMax = _normValues.value().y;
    float standardScore = ( value - mean ) / sd;
    // Clamp intresting values
    standardScore = glm::clamp(standardScore, -zScoreMin, zScoreMax);
    //return and normalize
    return ( standardScore + zScoreMin )/(zScoreMin + zScoreMax );  
}

float DataPlane::normalizeWithLogarithm(float value, int logMean){
    float logMin = 10*_normValues.value().x;
    float logMax = 10*_normValues.value().y;

    float logNormalized = ((value/pow(10,logMean)+logMin))/(logMin+logMax);
    return glm::clamp(logNormalized,0.0f, 1.0f);
}

void DataPlane::changeTransferFunctions(bool multiple){
    _transferFunctions.clear();
    std::string tfPath;
    if(multiple){
        tfPath = "${OPENSPACE_DATA}/scene/iswa/transferfunctions/red.jpg";
        _transferFunctions.push_back(std::make_shared<TransferFunction>(tfPath));
        tfPath = "${OPENSPACE_DATA}/scene/iswa/transferfunctions/blue.jpg";
        _transferFunctions.push_back(std::make_shared<TransferFunction>(tfPath));
        tfPath = "${OPENSPACE_DATA}/scene/iswa/transferfunctions/green.jpg";
        _transferFunctions.push_back(std::make_shared<TransferFunction>(tfPath));
    }else{
        tfPath = "${OPENSPACE_DATA}/scene/iswa/transferfunctions/colormap_parula.jpg";
        _transferFunctions.push_back(std::make_shared<TransferFunction>(tfPath));
    }
}
}// namespace openspace