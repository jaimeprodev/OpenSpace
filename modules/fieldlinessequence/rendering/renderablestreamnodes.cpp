/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2020                                                               *
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
 //including our own h file
#include <modules/fieldlinessequence/rendering/renderablestreamnodes.h>

// Includes from fieldlinessequence, might not need all of them
#include <modules/fieldlinessequence/fieldlinessequencemodule.h>
#include <modules/fieldlinessequence/util/kameleonfieldlinehelper.h>
#include <openspace/engine/globals.h>
#include <openspace/engine/windowdelegate.h>
#include <openspace/interaction/navigationhandler.h>
#include <openspace/interaction/orbitalnavigator.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/scene/scene.h>
#include <openspace/util/timemanager.h>
#include <openspace/util/updatestructures.h>
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/logging/logmanager.h>
// Test debugging tools more then logmanager
#include <ghoul/logging/consolelog.h>
#include <ghoul/logging/visualstudiooutputlog.h>
#include <ghoul/filesystem/cachemanager.h>

#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/textureunit.h>
#include <fstream>
#include <thread>
#include <openspace/json.h>

// This is a call to use the nlohmann json file
using json = nlohmann::json;

namespace {
    // log category
    constexpr const char* _loggerCat = "renderableStreamNodes";

    // GL variables for shaders, probably needed some of them atleast
    constexpr const GLuint VaPosition   = 0; // MUST CORRESPOND TO THE SHADER PROGRAM
    constexpr const GLuint VaColor      = 1; // MUST CORRESPOND TO THE SHADER PROGRAM
    constexpr const GLuint VaFiltering  = 2; // MUST CORRESPOND TO THE SHADER PROGRAM
    constexpr const GLuint VaIndex      = 3; // MUST CORRESPOND TO THE SHADER PROGRAM

    constexpr int8_t CurrentCacheVersion = 1;

    // ----- KEYS POSSIBLE IN MODFILE. EXPECTED DATA TYPE OF VALUE IN [BRACKETS]  ----- //
    // ---------------------------- MANDATORY MODFILE KEYS ---------------------------- //
   // [STRING] "json"
    constexpr const char* KeyInputFileType = "InputFileType";
    // [STRING] should be path to folder containing the input files
    constexpr const char* KeySourceFolder = "SourceFolder";

    // ---------------------- MANDATORY INPUT TYPE SPECIFIC KEYS ---------------------- //
    // [STRING] Currently supports: "batsrus", "enlil" & "pfss"
    constexpr const char* KeyJsonSimulationModel = "SimulationModel";

    // ----------------------- OPTIONAL INPUT TYPE SPECIFIC KEYS ---------------------- //
    // [STRING]
    constexpr const char* KeyJsonScalingFactor = "ScaleToMeters";
    //[INT] Threshold Radius should have a range
    constexpr const char* KeyThresholdRadius = "ThresholdRadius";

    // ---------------------------- OPTIONAL MODFILE KEYS  ---------------------------- //
    // [STRING ARRAY] Values should be paths to .txt files
    constexpr const char* KeyColorTablePaths = "ColorTablePaths";
    //[INT] Line Width should have a range
    constexpr const char* KeyLineWidth = "LineWidth";

    // ------------- POSSIBLE STRING VALUES FOR CORRESPONDING MODFILE KEY ------------- //
    constexpr const char* ValueInputFileTypeJson = "json";

    // --------------------------------- Property Info -------------------------------- //
    constexpr openspace::properties::Property::PropertyInfo ColorModeInfo = {
        "colorMode",
        "Color Mode",
        "Color lines uniformly or using color tables based on specific values on nodes,"
        "for examples flux values."
    };
    constexpr openspace::properties::Property::PropertyInfo ColorFluxInfo = {
        "colorFlux",
        "Flux value to Color By",
        "Flux values used to color lines if the 'By Flux value' color method is selected."
    };
    constexpr openspace::properties::Property::PropertyInfo ColorTablePathInfo = {
        "colorTablePath",
        "Path to Color Table",
        "Color Table/Transfer Function to use for 'By Flux Value' coloring."
    };
    constexpr openspace::properties::Property::PropertyInfo StreamColorInfo = {
        "color",
        "Color",
        "Color of particles."
    };
    constexpr openspace::properties::Property::PropertyInfo NodeSizeInfo = {
       "nodeSize",
       "Size of nodes",
       "Change the size of the nodes"
    };
    constexpr openspace::properties::Property::PropertyInfo NodeSizeLargerFluxInfo = {
       "nodeSizeLargerFlux",
       "Size of nodes for larger flux",
       "Change the size of the nodes when flux is larger than flux threshold value"
    };
    constexpr openspace::properties::Property::PropertyInfo LineWidthInfo = {
       "lineWidth",
       "Line Width",
       "This value specifies the line width of the field lines if the "
       "selected rendering method includes lines."
    };
    constexpr openspace::properties::Property::PropertyInfo ThresholdFluxInfo = {
       "thresholdFlux",
       "Threshold flux value",
       "This value specifies the threshold that will be changed with the flux value."
    };
    constexpr openspace::properties::Property::PropertyInfo FilteringInfo = {
        "filteringlower",
        "Filtering Lower Value in AU",
        "Use filtering to show nodes within a given range."
    };
    constexpr openspace::properties::Property::PropertyInfo FilteringUpperInfo = {
        "filteringupper",
        "Filtering Upper Value in AU",
        "Use filtering to show nodes within a given range."
    };
    constexpr openspace::properties::Property::PropertyInfo AmountofNodesInfo = {
        "amountOfNodes",
        "Every nth node to render in",
        "Show only every nth node"
    };
    constexpr openspace::properties::Property::PropertyInfo DefaultNodeSkipInfo = {
        "nodeSkip",
        "Every nth node to render default",
        "Show only every nth node outside of skippingmethod"
    };
    constexpr openspace::properties::Property::PropertyInfo ScalingmethodInfo = {
        "scalingFlux",
        "Scale the flux value with color table",
        "Use scaling to color nodes with a given method."
    };
    constexpr openspace::properties::Property::PropertyInfo NodeskipMethodInfo = {
        "skippingNodes",
        "How to select nodes to skip",
        "Methods to select nodes to skip."
    };
    constexpr openspace::properties::Property::PropertyInfo colorTableRangeInfo = {
        "colorTableRange",
        "Color Table Range",
        "Valid range for the color table. [Min, Max]"
    };
    constexpr openspace::properties::Property::PropertyInfo DomainZInfo = {
        "zLimit",
        "Z-limits",
        "Valid range along the Z-axis. [Min, Max]"
    };
    constexpr openspace::properties::Property::PropertyInfo FluxColorAlphaInfo = {
        "fluxColorAlpha",
        "Flux Color Alpha",
        "The value of alpha for the flux color mode."
    };
    constexpr openspace::properties::Property::PropertyInfo FluxNodeskipThresholdInfo = {
        "skippingNodesByFlux",
        "Skipping Nodes By Flux",
        "Select nodes to skip depending on flux value."
    };
    constexpr openspace::properties::Property::PropertyInfo RadiusNodeSkipThresholdInfo = {
        "skippingNodesByRadius",
        "Skipping Nodes By Radius",
        "Select nodes to skip depending on Radius."
    };
    enum class SourceFileType : int {
        Json = 0,
        Invalid
    };

    float stringToFloat(const std::string input, const float backupValue = 0.f) {
        float tmp;
        try {
            tmp = std::stof(input);
        }
        catch (const std::invalid_argument& ia) {
            LWARNING(fmt::format(
                "Invalid argument: {}. '{}' is NOT a valid number", ia.what(), input
                ));
            return backupValue;
        }
        return tmp;
    }
    double stringToDouble(const std::string input, const float backupValue = 0.f) {
        double tmp;

        try {
            tmp = std::stod(input);
        }
        catch (const std::invalid_argument& ia) {
            LWARNING(fmt::format(
                "Invalid argument: {}. '{}' is NOT a valid number", ia.what(), input
                ));
            return backupValue;
        }
        return tmp;
    }
    // Changed everything from dvec3 to vec3
    glm::vec3 sphericalToCartesianCoord(glm::vec3 position) {
        glm::vec3 cartesianPosition = glm::vec3();

        // ρsinφcosθ 
        cartesianPosition.x = position.x * sin(position.z) * cos(position.y);
        // ρsinφsinθ
        cartesianPosition.y = position.x * sin(position.z) * sin(position.y);
        // ρcosφ
        cartesianPosition.z = position.x * cos(position.z);

        return cartesianPosition;
    }
} //namespace

namespace openspace {
    using namespace properties;
    RenderableStreamNodes::RenderableStreamNodes(const ghoul::Dictionary& dictionary)

        : Renderable(dictionary)
        , _pColorGroup({ "Color" })
        , _pColorMode(ColorModeInfo, OptionProperty::DisplayType::Radio)
        , _pScalingmethod(ScalingmethodInfo, OptionProperty::DisplayType::Radio)
        , _pNodeskipMethod(NodeskipMethodInfo, OptionProperty::DisplayType::Radio)
        , _pColorFlux(ColorFluxInfo, OptionProperty::DisplayType::Dropdown)
        , _pColorTablePath(ColorTablePathInfo)
        , _pStreamColor(StreamColorInfo,
            glm::vec4(0.96f, 0.88f, 0.8f, 0.5f),
            glm::vec4(0.f),
            glm::vec4(1.f))
        , _pStreamGroup({ "Streams" })
        , _pNodesamountGroup({ "NodeGroup" })
        , _pNodeSize(NodeSizeInfo, 2.f, 1.f, 10.f)
        , _pNodeSizeLargerFlux(NodeSizeLargerFluxInfo, 2.f, 1.f, 10.f)
        , _pLineWidth(LineWidthInfo, 1.f, 1.f, 20.f)
        , _pColorTableRange(colorTableRangeInfo)
        , _pDomainZ(DomainZInfo)
        , _pFluxColorAlpha(FluxColorAlphaInfo, 1.f, 0.f, 1.f)
        , _pThresholdFlux(ThresholdFluxInfo, 0.f, -20.f, 20.f)
        , _pFiltering(FilteringInfo, 0.f, 0.f, 5.f)
        , _pFilteringUpper(FilteringUpperInfo, 5.f, 0.f, 5.f)
        , _pAmountofNodes(AmountofNodesInfo, 1, 1, 100)
        , _pDefaultNodeSkip(DefaultNodeSkipInfo, 1, 1, 100)
        , _pFluxNodeskipThreshold(FluxNodeskipThresholdInfo, 0, -20, 10)
        , _pRadiusNodeSkipThreshold(RadiusNodeSkipThresholdInfo, 0.f, 0.f, 5.f) 
    {
        _dictionary = std::make_unique<ghoul::Dictionary>(dictionary);
    }

    void RenderableStreamNodes::definePropertyCallbackFunctions() {
    // Add Property Callback Functions
        _pColorFlux.onChange([this] {
            _pColorTablePath = _colorTablePaths[_pColorFlux];
        });

        _pColorTablePath.onChange([this] {
            _transferFunction->setPath(_pColorTablePath);
            _colorTablePaths[_pColorFlux] = _pColorTablePath;
        });
    }

    void RenderableStreamNodes::setModelDependentConstants() {
        // Just used as a default value.
        float limit = 8.f; 
        _pColorTableRange.setMinValue(glm::vec2(-limit));
        _pColorTableRange.setMaxValue(glm::vec2(limit));
        _pColorTableRange = glm::vec2(-2, 4);

        float limitZMin = -1000000000000;
        float limitZMax = 1000000000000;

        _pDomainZ.setMinValue(glm::vec2(limitZMin));
        _pDomainZ.setMaxValue(glm::vec2(limitZMax));
        _pDomainZ = glm::vec2(limitZMin, limitZMax);
    }
    
    void RenderableStreamNodes::initializeGL() {
        // EXTRACT MANDATORY INFORMATION FROM DICTIONARY
        // std::string filepath = "C:/Users/chrad171//openspace/OpenSpace/sync/http/bastille_day_streamnodes/1/datawithoutprettyprint_newmethod.json";
       
        SourceFileType sourceFileType = SourceFileType::Invalid;
        if (!extractMandatoryInfoFromDictionary(sourceFileType)) {
            return;
        }

        // Set a default color table, just in case the (optional) user defined paths are
        // corrupt or not provided!
        _colorTablePaths.push_back(FieldlinesSequenceModule::DefaultTransferFunctionFile);
        _transferFunction = std::make_unique<TransferFunction>(absPath(_colorTablePaths[0]));

        // EXTRACT OPTIONAL INFORMATION FROM DICTIONARY
        std::string outputFolderPath;
        //extractOptionalInfoFromDictionary(outputFolderPath);

        // EXTRACT SOURCE FILE TYPE SPECIFIC INFOMRATION FROM DICTIONARY & GET STATES FROM
        // SOURCE
        if (!loadJsonStatesIntoRAM(outputFolderPath)) {
            return;
        }
    
        ghoul::Dictionary colorTablesPathsDictionary;
        if (_dictionary->getValue(KeyColorTablePaths, colorTablesPathsDictionary)) {
            const size_t nProvidedPaths = colorTablesPathsDictionary.size();
            LDEBUG("Number of provided Paths: " + std::to_string(nProvidedPaths));
            if (nProvidedPaths > 0) {
                // Clear the default! It is already specified in the transferFunction
                _colorTablePaths.clear();
                for (size_t i = 1; i <= nProvidedPaths; ++i) {
                    _colorTablePaths.push_back(
                        colorTablesPathsDictionary.value<std::string>(std::to_string(i)));
                }
            }
        }

        // dictionary is no longer needed as everything is extracted
        _dictionary.reset();

        // No need to store source paths in memory if they are already in RAM!
        //if (!_loadingStatesDynamically) {
        //    _sourceFiles.clear();
        //}
        //_nStates = 274;
        setModelDependentConstants();
        setupProperties();
       
        extractTriggerTimesFromFileNames();
        computeSequenceEndTime();

        // Either we load in the data dynamically or statically at the start. /If we should load in everything to Ram this if statement is true.
        if (!_loadingStatesDynamically) {
          
            std::string _file = "StreamnodesCacheindex";
            std::string cachedFile = FileSys.cacheManager()->cachedFilename(
                _file,
                ghoul::filesystem::CacheManager::Persistent::Yes
            );
            // Check if we have a cached binary file for the data
            bool hasCachedFile = FileSys.fileExists(cachedFile);

            if (hasCachedFile) {
                LINFO(fmt::format("Cached file '{}' used for Speck file '{}'",
                    cachedFile, _file
                ));
                // Read in the data from the cached file
                bool success = readCachedFile(cachedFile);
                if (!success) {
                    // If something went wrong it is probably because we changed the cache version or some file was not found.
                    LWARNING("Cache file removed, something went wrong loading it.");
                    // If thats the case we want to load in the files from json format and then write new cached files. 
                    loadFilesIntoRam();
                    writeCachedFile("StreamnodesCacheindex");
                }
            }
            else {
                // We could not find the cachedfiles, parse the data statically instead and write it to binary format.
                loadFilesIntoRam();
                writeCachedFile("StreamnodesCacheindex");
            }
        }
        // If we are loading in states dynamically we would read new states during runtime, parsing json files pretty slowly.
       
        // Setup shader program
        _shaderProgram = global::renderEngine.buildRenderProgram(
            "Streamnodes",
            absPath("${MODULE_FIELDLINESSEQUENCE}/shaders/streamnodes_vs.glsl"),
            absPath("${MODULE_FIELDLINESSEQUENCE}/shaders/streamnodes_fs.glsl")
            );

        _uniformCache.streamColor = _shaderProgram->uniformLocation("streamColor");
        _uniformCache.nodeSize = _shaderProgram->uniformLocation("nodeSize");
        _uniformCache.nodeSizeLargerFlux = _shaderProgram->uniformLocation("nodeSizeLargerFlux");
        _uniformCache.thresholdFlux = _shaderProgram->uniformLocation("thresholdFlux");

        glGenVertexArrays(1, &_vertexArrayObject);
        glGenBuffers(1, &_vertexPositionBuffer);
        glGenBuffers(1, &_vertexColorBuffer);
        glGenBuffers(1, &_vertexFilteringBuffer);
        glGenBuffers(1, &_vertexindexBuffer);
    }

    bool RenderableStreamNodes::loadFilesIntoRam() {
        LDEBUG("Did not find cached file, loading in data and converting only for this run, this step wont be needed next time you run Openspace ");
        // Loop through all the files dependent on how many states we would like to read in
        for (size_t j = 0; j < _nStates; ++j) {
            
            std::ifstream streamdata(_sourceFiles[j]);
            if (!streamdata.is_open())
            {
                LDEBUG("did not read the data.json file");
                return false;
            }
            json jsonobj = json::parse(streamdata);
           
            const char* sNode = "node0";
            const char* sStream = "stream0";
            const char* sData = "data";

            const json& jTmp = *(jsonobj.begin()); // First node in the file
            const char* sTime = "time";
            std::string testtime = jsonobj["time"];
          
            size_t lineStartIdx = 0;
            const int numberofStreams = 383;
            constexpr const float AuToMeter = 149597870700.f;  // Astronomical Units
            
            // Clear all the vectors in order to not have old states information in them
            _vertexPositions.clear();
            _lineCount.clear();
            _lineStart.clear();
            _vertexRadius.clear();
            _vertexColor.clear();
            _vertexIndex.clear();

            int counter = 0;
            int NodeIndexCounter = 0;

            const size_t nPoints = 1;

            // Loop through all the streams
            for (int i = 0; i < numberofStreams; ++i) {

                NodeIndexCounter = 0;

                // Make an iterator at stream number i, then loop through that stream by iterating forward
                for (json::iterator lineIter = jsonobj["stream" + std::to_string(i)].begin();
                    lineIter != jsonobj["stream" + std::to_string(i)].end(); ++lineIter) {
                    
                    //get all the nodepositional values and Flux value
                    std::string r = (*lineIter)["R"].get<std::string>();
                    std::string phi = (*lineIter)["Phi"].get<std::string>();
                    std::string theta = (*lineIter)["Theta"].get<std::string>();
                    std::string flux = (*lineIter)["Flux"].get<std::string>();
                          
                    // Convert the values to float
                    float rValue = stringToFloat(r);
                    float phiValue = stringToFloat(phi);
                    float thetaValue = stringToFloat(theta);
                    float fluxValue = stringToFloat(flux);
                    float ninetyDeToRad = 1.57079633f * 2;
                    const float pi = 3.14159265359f;

                    // Push back values in order to be able to filter and color nodes by different threshold etc.
                    float rTimesFluxValue = fluxValue;
                    _vertexColor.push_back(rTimesFluxValue);
                    _vertexRadius.push_back(rValue);
                    _vertexIndex.push_back(NodeIndexCounter);
                    // NodeIndexcounter is used to decide how many nodes we want to show. 
                    NodeIndexCounter++;
                    rValue = rValue * AuToMeter;

                    glm::vec3 sphericalcoordinates =
                        glm::vec3(rValue, phiValue, thetaValue);

                    // Convert the position from spherical coordinates to cartesian.
                    glm::vec3 position = sphericalToCartesianCoord(sphericalcoordinates);
                  
                    _vertexPositions.push_back(position);
                    ++counter;
              
                    _lineCount.push_back(static_cast<GLsizei>(nPoints));
                    _lineStart.push_back(static_cast<GLsizei>(lineStartIdx));
                    lineStartIdx += nPoints;
                }
            }
            LDEBUG("Loaded in: " + std::to_string(_statesPos.size()) + " frames of nodedata out of " + std::to_string(_nStates) + " total.");
           
            // Push back the vectors into our statesvectors
            _statesPos.push_back(_vertexPositions);     
            _statesColor.push_back(_vertexColor);
            _statesRadius.push_back(_vertexRadius);
            _statesIndex.push_back(_vertexIndex);
        }    
        return true;
    }

    void RenderableStreamNodes::writeCachedFile(const std::string& file) const {
        // Todo, write all of the vertexobjects into here 
       
        std::string _file = "StreamnodesCacheindex";
        std::string cachedFile = FileSys.cacheManager()->cachedFilename(
            _file,
            ghoul::filesystem::CacheManager::Persistent::Yes
        );
       std::ofstream fileStream(cachedFile, std::ofstream::binary);

        if (!fileStream.good()) {
            LERROR(fmt::format("Error opening file '{}' for save cache file"), "StreamnodesCache");
            return;
        }

        fileStream.write(
            reinterpret_cast<const char*>(&CurrentCacheVersion),
            sizeof(int8_t)
        );
        
        std::string _file2 = "StreamnodesCacheColor";
        std::string _file3 = "StreamnodesCacheRadius";
        std::string _file4 = "StreamnodesCachePosition";
        std::string cachedFile2 = FileSys.cacheManager()->cachedFilename(
            _file2,
            ghoul::filesystem::CacheManager::Persistent::Yes
        );
        std::string cachedFile3 = FileSys.cacheManager()->cachedFilename(
            _file3,
            ghoul::filesystem::CacheManager::Persistent::Yes
        );
        std::string cachedFile4 = FileSys.cacheManager()->cachedFilename(
            _file4,
            ghoul::filesystem::CacheManager::Persistent::Yes
        );
        std::ofstream fileStream2(cachedFile2, std::ofstream::binary);
        std::ofstream fileStream3(cachedFile3, std::ofstream::binary);
        std::ofstream fileStream4(cachedFile4, std::ofstream::binary);

        int32_t nValues = static_cast<int32_t>(_vertexPositions.size());
        if (nValues == 0) {
            throw ghoul::RuntimeError("Error writing cache: No values were loaded");
            return;
        }

        fileStream.write(reinterpret_cast<const char*>(&nValues), sizeof(int32_t));

        LDEBUG("nvalues _vertex index size" + std::to_string(_vertexIndex.size()));

        for(int i = 0; i < _nStates; ++i){
        fileStream.write(reinterpret_cast<const char*>(_statesIndex[i].data()), nValues * sizeof(_vertexIndex[0]));
        fileStream2.write(reinterpret_cast<const char*>(_statesColor[i].data()), nValues * sizeof(_vertexColor[0]));
        fileStream3.write(reinterpret_cast<const char*>(_statesRadius[i].data()), nValues * sizeof(_vertexColor[0]));
        fileStream4.write(reinterpret_cast<const char*>(_statesPos[i].data()), nValues * sizeof(_vertexPositions[0]));
        }
    }

    bool RenderableStreamNodes::readCachedFile(const std::string& file) {
       // const std::string& file = "StreamnodesCache";
        std::ifstream fileStream(file, std::ifstream::binary);

        std::string _file2 = "StreamnodesCacheColor";
        std::string _file3 = "StreamnodesCacheRadius";
        std::string _file4 = "StreamnodesCachePosition";
        std::string cachedFile2 = FileSys.cacheManager()->cachedFilename(
            _file2,
            ghoul::filesystem::CacheManager::Persistent::Yes
        );
        std::string cachedFile3 = FileSys.cacheManager()->cachedFilename(
            _file3,
            ghoul::filesystem::CacheManager::Persistent::Yes
        );
        std::string cachedFile4 = FileSys.cacheManager()->cachedFilename(
            _file4,
            ghoul::filesystem::CacheManager::Persistent::Yes
        );
        std::ifstream fileStream2(cachedFile2, std::ifstream::binary);
        std::ifstream fileStream3(cachedFile3, std::ifstream::binary);
        std::ifstream fileStream4(cachedFile4, std::ifstream::binary);

        if (fileStream.good()) {
            int8_t version = 0;
            fileStream.read(reinterpret_cast<char*>(&version), sizeof(int8_t));
            if (version != CurrentCacheVersion) {
                LINFO("The format of the cached file has changed: deleting old cache");
                fileStream.close();
                FileSys.deleteFile(file);
                FileSys.deleteFile(cachedFile2);
                FileSys.deleteFile(cachedFile3);
                FileSys.deleteFile(cachedFile4);
                return false;
            }
            LDEBUG("testar int8" + std::to_string(version));
            int32_t nValuesvec = 0;
            fileStream.read(reinterpret_cast<char*>(&nValuesvec), sizeof(int32_t));   
           
            for (int i = 0; i < _nStates; ++i) {
                _vertexIndex.resize(nValuesvec);
                fileStream.read(reinterpret_cast<char*>(
                    _vertexIndex.data() ),
                    nValuesvec * sizeof(_vertexIndex[0]));

                _statesIndex.push_back(_vertexIndex);
                _vertexIndex.clear();
            }
            for (int i = 0; i < _nStates; ++i) {
                _vertexColor.resize(nValuesvec);
                fileStream2.read(reinterpret_cast<char*>(
                    _vertexColor.data()),
                    nValuesvec * sizeof(_vertexColor[0]));

                _statesColor.push_back(_vertexColor);
                _vertexColor.clear();
            }

            for (int i = 0; i < _nStates; ++i) {
                _vertexRadius.resize(nValuesvec);
                fileStream3.read(reinterpret_cast<char*>(
                    _vertexRadius.data()),
                    nValuesvec * sizeof(_vertexColor[0]));

                _statesRadius.push_back(_vertexRadius);
                _vertexRadius.clear();
            }
            for (int i = 0; i < _nStates; ++i) {
                _vertexPositions.resize(nValuesvec);
                fileStream4.read(reinterpret_cast<char*>(
                    _vertexPositions.data() ),
                    nValuesvec * sizeof(_vertexPositions[0]));

                _statesPos.push_back(_vertexPositions);
                _vertexPositions.clear();
            }
                bool success = fileStream.good();
        
                return success;
        }
        return false;
    }
    /**
     * Extracts the general information (from the lua modfile) that is mandatory for the class
     * to function; such as the file type and the location of the source files.
     * Returns false if it fails to extract mandatory information!
     */
    bool RenderableStreamNodes::extractMandatoryInfoFromDictionary(
        SourceFileType& sourceFileType)
    {
        _dictionary->getValue(SceneGraphNode::KeyIdentifier, _identifier);

        // ------------------- EXTRACT MANDATORY VALUES FROM DICTIONARY ------------------- //
        std::string inputFileTypeString;
        if (!_dictionary->getValue(KeyInputFileType, inputFileTypeString)) {
            LERROR(fmt::format("{}: The field {} is missing", _identifier, KeyInputFileType));
        }
        else {
            // Verify that the input type is corrects
            if (inputFileTypeString == ValueInputFileTypeJson) {
                sourceFileType = SourceFileType::Json;
            }
            else {
                LERROR(fmt::format(
                    "{}: {} is not a recognized {}",
                    _identifier, inputFileTypeString, KeyInputFileType
                    ));
                sourceFileType = SourceFileType::Invalid;
                return false;
            }
        }

        _colorTableRanges.push_back(glm::vec2(0, 1));

        std::string sourceFolderPath;
        if (!_dictionary->getValue(KeySourceFolder, sourceFolderPath)) {
            LERROR(fmt::format("{}: The field {} is missing", _identifier, KeySourceFolder));
            return false;
        }

        // Ensure that the source folder exists and then extract
        // the files with the same extension as <inputFileTypeString>
        ghoul::filesystem::Directory sourceFolder(sourceFolderPath);
        if (FileSys.directoryExists(sourceFolder)) {
            // Extract all file paths from the provided folder
            _sourceFiles = sourceFolder.readFiles(
                ghoul::filesystem::Directory::Recursive::No,
                ghoul::filesystem::Directory::Sort::Yes
                );
            // Ensure that there are available and valid source files left
            if (_sourceFiles.empty()) {
                LERROR(fmt::format(
                    "{}: {} contains no {} files",
                    _identifier, sourceFolderPath, inputFileTypeString
                    ));
                return false;
            }
        }
        else {
            LERROR(fmt::format(
                "{}: FieldlinesSequence {} is not a valid directory",
                _identifier,
                sourceFolderPath
                ));
            return false;
        }
        return true;
    }

    bool RenderableStreamNodes::extractJsonInfoFromDictionary(fls::Model& model) {
        std::string modelStr;
        if (_dictionary->getValue(KeyJsonSimulationModel, modelStr)) {
            std::transform(
                modelStr.begin(),
                modelStr.end(),
                modelStr.begin(),
                [](char c) { return static_cast<char>(::tolower(c)); }
            );
            model = fls::stringToModel(modelStr);
        }
        else {
            LERROR(fmt::format(
                "{}: Must specify '{}'", _identifier, KeyJsonSimulationModel
                ));
            return false;
        }
        float lineWidthValue;
        if (_dictionary->getValue(KeyLineWidth, lineWidthValue)) {
            _pLineWidth = lineWidthValue;
        }
        float thresholdRadiusValue;
        if (_dictionary->getValue(KeyThresholdRadius, thresholdRadiusValue)) {
            _pThresholdFlux = thresholdRadiusValue;
        }
        float scaleFactor;
        if (_dictionary->getValue(KeyJsonScalingFactor, scaleFactor)) {
            _scalingFactor = scaleFactor;
        }
        else {
            LWARNING(fmt::format(
                "{}: Does not provide scalingFactor. Assumes coordinates are in meters",
                _identifier
                ));
        }
        return true;
    }

    void RenderableStreamNodes::setupProperties() {

        // -------------- Add non-grouped properties (enablers and buttons) -------------- //
        addProperty(_pLineWidth);
        
        // ----------------------------- Add Property Groups ----------------------------- //
        addPropertySubOwner(_pColorGroup);
        addPropertySubOwner(_pStreamGroup);
        addPropertySubOwner(_pNodesamountGroup);

        // ------------------------- Add Properties to the groups ------------------------ //
        _pColorGroup.addProperty(_pColorMode);
        _pColorGroup.addProperty(_pScalingmethod);
        _pColorGroup.addProperty(_pColorTableRange);
        _pColorGroup.addProperty(_pColorTablePath);
        _pColorGroup.addProperty(_pStreamColor);
        _pColorGroup.addProperty(_pFluxColorAlpha);

        _pStreamGroup.addProperty(_pThresholdFlux);
        _pStreamGroup.addProperty(_pFiltering);
        _pStreamGroup.addProperty(_pFilteringUpper);
        _pStreamGroup.addProperty(_pDomainZ);

        _pNodesamountGroup.addProperty(_pNodeskipMethod);
        _pNodesamountGroup.addProperty(_pAmountofNodes);
        _pNodesamountGroup.addProperty(_pDefaultNodeSkip);
        _pNodesamountGroup.addProperty(_pNodeSize);
        _pNodesamountGroup.addProperty(_pNodeSizeLargerFlux);
        _pNodesamountGroup.addProperty(_pFluxNodeskipThreshold);
        _pNodesamountGroup.addProperty(_pRadiusNodeSkipThreshold);

        // --------------------- Add Options to OptionProperties --------------------- //
        _pColorMode.addOption(static_cast<int>(ColorMethod::Uniform), "Uniform");
        _pColorMode.addOption(static_cast<int>(ColorMethod::ByFluxValue), "By Flux Value");

        _pScalingmethod.addOption(static_cast<int>(ScalingMethod::Flux), "Flux");
        _pScalingmethod.addOption(static_cast<int>(ScalingMethod::RFlux), "Radius * Flux");
        _pScalingmethod.addOption(static_cast<int>(ScalingMethod::R2Flux), "Radius^2 * Flux");
        _pScalingmethod.addOption(static_cast<int>(ScalingMethod::log10RFlux), "log10(r) * Flux");
        _pScalingmethod.addOption(static_cast<int>(ScalingMethod::lnRFlux), "ln(r) * Flux");
        
        _pNodeskipMethod.addOption(static_cast<int>(NodeskipMethod::Uniform), "Uniform");
        _pNodeskipMethod.addOption(static_cast<int>(NodeskipMethod::Flux), "Flux");
        _pNodeskipMethod.addOption(static_cast<int>(NodeskipMethod::Radius), "Radius");
        definePropertyCallbackFunctions();

        // Set default
        _pColorTablePath = _colorTablePaths[0];
    }

    void RenderableStreamNodes::deinitializeGL() {
        glDeleteVertexArrays(1, &_vertexArrayObject);
        _vertexArrayObject = 0;

        glDeleteBuffers(1, &_vertexPositionBuffer);
        _vertexPositionBuffer = 0;

        glDeleteBuffers(1, &_vertexColorBuffer);
        _vertexColorBuffer = 0;

        glDeleteBuffers(1, &_vertexFilteringBuffer);
        _vertexFilteringBuffer = 0;
        glDeleteBuffers(1, &_vertexindexBuffer);
        _vertexindexBuffer = 0;

        if (_shaderProgram) {
            global::renderEngine.removeRenderProgram(_shaderProgram.get());
            _shaderProgram = nullptr;
        }

        // Stall main thread until thread that's loading states is done!
        bool printedWarning = false;
        while (_isLoadingStateFromDisk) {
            if (!printedWarning) {
                LWARNING("Trying to destroy class when an active thread is still using it");
                printedWarning = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    bool RenderableStreamNodes::isReady() const {
        return _shaderProgram != nullptr;
    }

    // Extract J2000 time from file names
    // Requires files to be named as such: 'YYYY-MM-DDTHH-MM-SS-XXX.osfls'
    void RenderableStreamNodes::extractTriggerTimesFromFileNames() {
        // number of  characters in filename (excluding '.json')
        constexpr const int FilenameSize = 23;
        // size(".json")
        constexpr const int ExtSize = 5;

        for (const std::string& filePath : _sourceFiles) {
            LDEBUG("filepath " + filePath);
            const size_t strLength = filePath.size();
            // Extract the filename from the path (without extension)
            std::string timeString = filePath.substr(
                strLength - FilenameSize - ExtSize,
                FilenameSize - 1
                );
            // Ensure the separators are correct
            timeString.replace(4, 1, "-");
            timeString.replace(7, 1, "-");
            timeString.replace(13, 1, ":");
            timeString.replace(16, 1, ":");
            timeString.replace(19, 1, ".");
            const double triggerTime = Time::convertTime(timeString);
            LDEBUG("timestring " + timeString);
            _startTimes.push_back(triggerTime);

        }
    }
    void RenderableStreamNodes::updateActiveTriggerTimeIndex(double currentTime) {
        auto iter = std::upper_bound(_startTimes.begin(), _startTimes.end(), currentTime);
        if (iter != _startTimes.end()) {
            if (iter != _startTimes.begin()) {
                _activeTriggerTimeIndex = static_cast<int>(
                    std::distance(_startTimes.begin(), iter)
                    ) - 1;
            }
            else {
                _activeTriggerTimeIndex = 0;
            }
        }
        else {
            _activeTriggerTimeIndex = static_cast<int>(_nStates) - 1;
        }
    }
    void RenderableStreamNodes::render(const RenderData& data, RendererTasks&) {
        if (_activeTriggerTimeIndex != -1) {
            _shaderProgram->activate();

            // Calculate Model View MatrixProjection
            const glm::dmat4 rotMat = glm::dmat4(data.modelTransform.rotation);
            const glm::dmat4 modelMat =
                glm::translate(glm::dmat4(1.0), data.modelTransform.translation) *
                rotMat *
                glm::dmat4(glm::scale(glm::dmat4(1), glm::dvec3(data.modelTransform.scale)));
            const glm::dmat4 modelViewMat = data.camera.combinedViewMatrix() * modelMat;

            _shaderProgram->setUniform("modelViewProjection",
                data.camera.sgctInternal.projectionMatrix() * glm::mat4(modelViewMat));

        // Decided if we want to use uniformCache or not
        _shaderProgram->setUniform(_uniformCache.streamColor, _pStreamColor);
        _shaderProgram->setUniform(_uniformCache.nodeSize, _pNodeSize);
        _shaderProgram->setUniform(_uniformCache.nodeSizeLargerFlux, _pNodeSizeLargerFlux);
        _shaderProgram->setUniform(_uniformCache.thresholdFlux, _pThresholdFlux);
        _shaderProgram->setUniform("colorMode", _pColorMode);
        _shaderProgram->setUniform("filterRadius", _pFiltering);
        _shaderProgram->setUniform("filterUpper", _pFilteringUpper);
        _shaderProgram->setUniform("ScalingMode", _pScalingmethod);
        _shaderProgram->setUniform("colorTableRange", _pColorTableRange.value());
        _shaderProgram->setUniform("domainLimZ", _pDomainZ.value());
        _shaderProgram->setUniform("Nodeskip", _pAmountofNodes);
        _shaderProgram->setUniform("Nodeskipdefault", _pDefaultNodeSkip);
        _shaderProgram->setUniform("NodeskipMethod", _pNodeskipMethod);
        _shaderProgram->setUniform("NodeskipFluxThreshold", _pFluxNodeskipThreshold);
        _shaderProgram->setUniform("NodeskipRadiusThreshold", _pRadiusNodeSkipThreshold);
        _shaderProgram->setUniform("fluxColorAlpha", _pFluxColorAlpha);

        if (_pColorMode == static_cast<int>(ColorMethod::ByFluxValue)) {
            ghoul::opengl::TextureUnit textureUnit;
            textureUnit.activate();
            _transferFunction->bind(); // Calls update internally
            _shaderProgram->setUniform("colorTable", textureUnit);
        }

        const std::vector<glm::vec3>& vertPos = _vertexPositions;
        glBindVertexArray(_vertexArrayObject);
        glLineWidth(_pLineWidth);
        //glBindBuffer(GL_ARRAY_BUFFER, _vertexPositionBuffer);
        
        /*glMultiDrawArrays(
            GL_LINE_STRIP, //_drawingOutputType,
            _lineStart.data(),
            _lineCount.data(),
            static_cast<GLsizei>(_lineStart.size())
        );*/

            GLint temp = 0;
            glDrawArrays(
                GL_POINTS,
                temp,
                static_cast<GLsizei>(_vertexPositions.size())
            );

            glBindVertexArray(0);
            _shaderProgram->deactivate();
        }
    }
    inline void unbindGL() {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void RenderableStreamNodes::computeSequenceEndTime() {
        if (_nStates > 1) {
            const double lastTriggerTime = _startTimes[_nStates - 1];
            const double sequenceDuration = lastTriggerTime - _startTimes[0];
            const double averageStateDuration = sequenceDuration /
                (static_cast<double>(_nStates) - 1.0);
            _sequenceEndTime = lastTriggerTime + averageStateDuration;
        }
        else {
            // If there's just one state it should never disappear!
            _sequenceEndTime = DBL_MAX;
        }
    }

    void RenderableStreamNodes::update(const UpdateData& data) {
        if (_shaderProgram->isDirty()) {
            _shaderProgram->rebuildFromFile();
        }
        //Everything below is for updating depending on time
        const double currentTime = data.time.j2000Seconds();
        const bool isInInterval = (currentTime >= _startTimes[0]) &&
             (currentTime < _sequenceEndTime);
        //const bool isInInterval = true;
        if (isInInterval) {
            const size_t nextIdx = _activeTriggerTimeIndex + 1;
            if (
                // true => Previous frame was not within the sequence interval
                //_activeTriggerTimeIndex < 0 ||
                // true => We stepped back to a time represented by another state
                currentTime < _startTimes[_activeTriggerTimeIndex] ||
                // true => We stepped forward to a time represented by another state
                (nextIdx < _nStates && currentTime >= _startTimes[nextIdx]))
            {
                updateActiveTriggerTimeIndex(currentTime);
                //LDEBUG("Vi borde uppdatera1");

                // _mustLoadNewStateFromDisk = true;

                _needsUpdate = true;
                _activeStateIndex = _activeTriggerTimeIndex;

            } // else {we're still in same state as previous frame (no changes needed)}
        }
            else {
                //not in interval => set everything to false
            //LDEBUG("not in interval");
                _activeTriggerTimeIndex = -1;
                _needsUpdate = false;
            }

            if (_needsUpdate) {
                if(_loadingStatesDynamically){
                if (!_isLoadingStateFromDisk) {
                    _isLoadingStateFromDisk = true;
                    LDEBUG("triggertime: " + std::to_string(_activeTriggerTimeIndex));
                std::string filePath = _sourceFiles[_activeTriggerTimeIndex];
               // auto vec = LoadJsonfile(filePath);
                std::thread readBinaryThread([this, f = std::move(filePath)]{
                     auto vec = LoadJsonfile(f);
                    });
                   readBinaryThread.detach();
                }
        
        _needsUpdate = false;

        if(_vertexPositions.size() > 5800){
            updatePositionBuffer();
            updateVertexColorBuffer();
            updateVertexFilteringBuffer();
            updateVertexIndexBuffer();
        }
            }
                // Needs fix, right now it stops cuz it cant find the states
                else if(!_statesPos[_activeTriggerTimeIndex].empty()){
                    _vertexPositions = _statesPos[_activeTriggerTimeIndex];
                    _vertexColor = _statesColor[_activeTriggerTimeIndex];
                    _vertexRadius = _statesRadius[_activeTriggerTimeIndex];
                    _vertexIndex = _statesIndex[_activeTriggerTimeIndex];
                    _needsUpdate = false;
                    updatePositionBuffer();
                    updateVertexColorBuffer();
                    updateVertexFilteringBuffer();
                    updateVertexIndexBuffer();
                }
            }
    }

    std::vector<std::string> RenderableStreamNodes::LoadJsonfile(std::string filepath) {
       
        //'YYYY-MM-DDTHH-MM-SS-XXX.osfls'
        //C:\Users\Chrad171\openspace\
        
        //std::ifstream streamdata("C:/Users/emiho502/desktop/OpenSpace/sync/http/bastille_day_streamnodes/1/datawithoutprettyprint_newmethod.json");
        //std::ifstream streamdata("C:/Users/chrad171//openspace/OpenSpace/sync/http/bastille_day_streamnodes/1/datawithoutprettyprint_newmethod.json");
        std::ifstream streamdata(filepath);
        //std::ifstream streamdata("C:/Users/chris/Documents/openspace/Openspace_ourbranch/OpenSpace/sync/http/bastille_day_streamnodes/2/datawithoutprettyprint_newmethod.json");
        if (!streamdata.is_open())
        {
            LDEBUG("did not read the data.json file");
        }
        json jsonobj = json::parse(streamdata);
        //json jsonobj;
        //streamdata >> jsonobj;
        
        //printDebug(jsonobj["stream0"]);
        //LDEBUG(jsonobj["stream0"]);
       // std::ofstream o("C:/Users/chris/Documents/openspace/Openspace_ourbranch/OpenSpace/sync/http/bastille_day_streamnodes/1/newdata2.json");
        //o << jsonobj << std::endl;
        LDEBUG("vi callade på loadJSONfile med  " + filepath);
        const char* sNode = "node0";
        const char* sStream = "stream0";
        const char* sData = "data";

        const json& jTmp = *(jsonobj.begin()); // First node in the file
        const char* sTime = "time";
        //double testtime = jTmp[sTime];
        std::string testtime = jsonobj["time"];
        //double testtime = Time::now();
        //const json::value_type& variableNameVec = jTmp[sStream][sNode][sData];
        //const size_t nVariables = variableNameVec.size();

        size_t lineStartIdx = 0;

        //Loop through all the nodes
        const int numberofStreams = 383;
        constexpr const float AuToMeter = 149597870700.f;
         _vertexPositions.clear();
         _lineCount.clear();
         _lineStart.clear();
         _vertexRadius.clear();
         _vertexColor.clear();
         _vertexIndex.clear();
        int counter = 0;
        int NodeIndexCounter = 0;
        
        const size_t nPoints = 1;
        for (int i = 0; i < numberofStreams; ++i) {
            NodeIndexCounter = 0;
            
            for (json::iterator lineIter = jsonobj["stream" + std::to_string(i)].begin();
                lineIter != jsonobj["stream" + std::to_string(i)].end(); ++lineIter) {
              //  for (size_t k = 0; k < 1999; ++k) {
               //     json::iterator lineIter = jsonobj["stream" + std::to_string(i)][std::to_string(k)].begin();
                
                //lineIter += 20;
                //const size_t Nodesamount = 
                //LDEBUG("testar debuggen");
                //log(ghoul::logging::LogLevel::Debug, _loggerCat, lineIter.key());
                //LDEBUG("stream" + std::to_string(i));
                //LDEBUG("Phi value: " + (*lineIter)["Phi"].get<std::string>());
                //LDEBUG("Theta value: " + (*lineIter)["Theta"].get<std::string>());
                //LDEBUG("R value: " + (*lineIter)["R"].get<std::string>());
                //LDEBUG("Flux value: " + (*lineIter)["Flux"].get<std::string>());

                // Probably needs some work with types, not loading in strings. 
                std::string r = (*lineIter)["R"].get<std::string>();
                std::string phi = (*lineIter)["Phi"].get<std::string>();
                std::string theta = (*lineIter)["Theta"].get<std::string>();
                std::string flux = (*lineIter)["Flux"].get<std::string>();

                float rValue = stringToFloat(r);
                float phiValue = stringToFloat(phi);
                float thetaValue = stringToFloat(theta);
                float fluxValue = stringToFloat(flux);
                float ninetyDeToRad = 1.57079633f * 2;
                const float pi = 3.14159265359f;
                float rTimesFluxValue = fluxValue;
                _vertexColor.push_back(rTimesFluxValue);
                _vertexRadius.push_back(rValue);
                _vertexIndex.push_back(NodeIndexCounter);
                NodeIndexCounter = NodeIndexCounter + 1;
                rValue = rValue * AuToMeter;
                
                glm::vec3 sphericalcoordinates =
                    glm::vec3(rValue, phiValue, thetaValue);

                //glm::dvec3 sphericalcoordinates =
                //    glm::dvec3(stringToDouble((*lineIter)["R"].get<std::string>()),
                  //      stringToDouble((*lineIter)["Phi"].get<std::string>()),
                   //     stringToDouble((*lineIter)["Theta"].get<std::string>()));

                //precision issue, right now rounding up at around 7th decimal. Probably 
                //around conversion with string to Double.
                //LDEBUG("R value after string to Float: " + std::to_string(stringToDouble
                //((*lineIter)["R"].get<std::string>())));
                //sphericalcoordinates.x = sphericalcoordinates.x * AuToMeter;
                glm::vec3 position = sphericalToCartesianCoord(sphericalcoordinates);

                _vertexPositions.push_back(position);
                ++counter;
                //   coordToMeters * glm::vec3(
                  //     stringToFloat((*lineIter)["Phi"].get<std::string>(), 0.0f),
                   //    ,

                    //   )
                   //);

                _lineCount.push_back(static_cast<GLsizei>(nPoints));
                _lineStart.push_back(static_cast<GLsizei>(lineStartIdx));
                lineStartIdx += nPoints;

                //_vertexColor.push_back(rTimesFluxValue);
                //_vertexRadius.push_back(rValue);

                //skipping nodes
               // int skipcounter = 0;
               // int nodeskipn = 10;
                //while (skipcounter < nodeskipn && lineIter != jsonobj["stream" + std::to_string(i)].end() - 1) {
                //    ++lineIter;
                //    ++skipcounter;
               // }
                //}
            }
        }

        LDEBUG("vertPos size:" + std::to_string(_vertexPositions.size()));
        LDEBUG("counter for how many times we push back" + std::to_string(counter));

        LDEBUG("Time:" + testtime);
        //openspace::printDebug("testar json"):
        //for 
        //LWARNING(fmt::format("Testar json", data));

        //LWARNING(fmt::format("Testar json"));
        _isLoadingStateFromDisk = false;
        //streamdata.close();
        return std::vector<std::string>();
    }
    void RenderableStreamNodes::updatePositionBuffer() {
        glBindVertexArray(_vertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, _vertexPositionBuffer);

        const std::vector<glm::vec3>& vertPos = _vertexPositions;

        glBufferData(
            GL_ARRAY_BUFFER,
            vertPos.size() * sizeof(glm::vec3),
            vertPos.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(VaPosition);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glVertexAttribPointer(VaPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);

        unbindGL();
    }
    void RenderableStreamNodes::updateVertexColorBuffer() {
        glBindVertexArray(_vertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, _vertexColorBuffer);

        const std::vector<float>& vertColor = _vertexColor;

            glBufferData(
                GL_ARRAY_BUFFER,
                vertColor.size() * sizeof(float),
                vertColor.data(),
                GL_STATIC_DRAW
            );

            glEnableVertexAttribArray(VaColor);
            glVertexAttribPointer(VaColor, 1, GL_FLOAT, GL_FALSE, 0, 0);

            unbindGL();
    }
    void RenderableStreamNodes::updateVertexFilteringBuffer() {
            glBindVertexArray(_vertexArrayObject);
            glBindBuffer(GL_ARRAY_BUFFER, _vertexFilteringBuffer);

            const std::vector<float>& vertexRadius = _vertexRadius;

            glBufferData(
                GL_ARRAY_BUFFER,
                vertexRadius.size() * sizeof(float),
                vertexRadius.data(),
                GL_STATIC_DRAW
            );

            glEnableVertexAttribArray(VaFiltering);
            glVertexAttribPointer(VaFiltering, 1, GL_FLOAT, GL_FALSE, 0, 0);

            unbindGL();
    }
    void RenderableStreamNodes::updateVertexIndexBuffer() {
        glBindVertexArray(_vertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, _vertexindexBuffer);

        const std::vector<int>& vertexIndex = _vertexIndex;

        glBufferData(
            GL_ARRAY_BUFFER,
            vertexIndex.size() * sizeof(float),
            vertexIndex.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(VaIndex);
        glVertexAttribPointer(VaIndex, 1, GL_FLOAT, GL_FALSE, 0, 0);

        unbindGL();
    }

    const std::vector<GLsizei>& RenderableStreamNodes::lineCount() const {
        return _lineCount;
    }

    const std::vector<GLint>& RenderableStreamNodes::lineStart() const {
        return _lineStart;
    }
    bool RenderableStreamNodes::loadJsonStatesIntoRAM(const std::string& outputFolder) {
        return true;
    }

} // namespace openspace
