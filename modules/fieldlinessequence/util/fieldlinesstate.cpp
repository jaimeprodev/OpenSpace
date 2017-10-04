/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2017                                                               *
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

#include <modules/fieldlinessequence/util/fieldlinesstate.h>

#include <ext/json/json.hpp>
#include <openspace/util/time.h>

#include <ghoul/logging/logmanager.h>

#include <fstream>

namespace {
    std::string _loggerCat = "FieldlinesState";
    const int CURRENT_VERSION = 0;

    using json = nlohmann::json;
}

namespace openspace {

FieldlinesState::FieldlinesState() {}
FieldlinesState::FieldlinesState(const std::string& PATH_TO_OSFLS_FILE, bool& loadSucessful) {
    loadSucessful = loadStateFromOsfls(PATH_TO_OSFLS_FILE);
}

bool FieldlinesState::loadStateFromOsfls(const std::string& PATH_TO_OSFLS_FILE) {
    std::ifstream ifs(PATH_TO_OSFLS_FILE, std::ifstream::binary);
    if (!ifs.is_open()) {
        LERRORC("FieldlinesState", "Couldn't open file: " + PATH_TO_OSFLS_FILE);
        return false;
    }

    int binFileVersion;
    ifs.read( reinterpret_cast<char*>(&binFileVersion), sizeof(int));

    switch (binFileVersion) {
        case 0 : {
                // No need to put everything in this scope now, as only version 0 exists!
            }
            break;
        default :
             LERRORC("FieldlinesState","VERSION OF BINARY FILE WAS NOT RECOGNISED!");
             return false;
    }

    // Define tmp variables to store meta data in
    size_t nLines;
    size_t nPoints;
    size_t nExtras;
    size_t byteSizeAllNames;

    // Read single value variables
    ifs.read( reinterpret_cast<char*>(&_triggerTime),     sizeof(double));
    ifs.read( reinterpret_cast<char*>(&_model),           sizeof(int));
    ifs.read( reinterpret_cast<char*>(&_isMorphable),     sizeof(bool));
    ifs.read( reinterpret_cast<char*>(&nLines),           sizeof(size_t));
    ifs.read( reinterpret_cast<char*>(&nPoints),          sizeof(size_t));
    ifs.read( reinterpret_cast<char*>(&nExtras),          sizeof(size_t));
    ifs.read( reinterpret_cast<char*>(&byteSizeAllNames), sizeof(size_t));

    // RESERVE/RESIZE vectors
    // TODO: Do this without initializing values? Resize is slower than just using reserve, due to initialization of all values
    _lineStart.resize(nLines);
    _lineCount.resize(nLines);
    _vertexPositions.resize(nPoints);
    _extraQuantities.resize(nExtras);
    _extraQuantityNames.reserve(nExtras);

    // Read vertex position data
    ifs.read( reinterpret_cast<char*>(_lineStart.data()),       sizeof(GLint)*nLines);
    ifs.read( reinterpret_cast<char*>(_lineCount.data()),       sizeof(GLsizei)*nLines);
    ifs.read( reinterpret_cast<char*>(_vertexPositions.data()), sizeof(glm::vec3)*nPoints);

    // Read all extra quantities
    for (std::vector<float>& vec : _extraQuantities) {
        vec.resize(nPoints);
        ifs.read( reinterpret_cast<char*>(vec.data()), sizeof(float) * nPoints);
    }

    // Read all extra quantities' names. Stored as multiple c-strings
    std::string allNamesInOne;
    char* s = new char[byteSizeAllNames];
    ifs.read(s, byteSizeAllNames);
    allNamesInOne.assign(s, byteSizeAllNames);
    delete[] s;

    size_t offset = 0;
    for (size_t i = 0; i < nExtras; ++i) {
        auto endOfVarName = allNamesInOne.find('\0', offset);
        endOfVarName -= offset;
        std::string varName = allNamesInOne.substr(offset, endOfVarName);
        offset += varName.size() + 1;
        _extraQuantityNames.push_back(varName);
    }

    return true;
}

// TODO: This should probably be rewritten, but this is the way the files were structured by CCMC
// Structure of File! NO TRAILING COMMAS ALLOWED!
// Additional info can be stored within each line as the code only extracts the keys it needs (time, trace & data)
// The key/name of each line ("0" & "1" in the example below) is arbitrary
// {
//     "0":{
//         "time": "YYYY-MM-DDTHH:MM:SS.XXX",
//         "trace": {
//             "columns": ["x","y","z","s","temperature","rho","j_para"],
//             "data": [[8.694,127.853,115.304,0.0,0.047,9.249,-5e-10],...,[8.698,127.253,114.768,0.800,0.0,9.244,-5e-10]]
//         },
//     },
//     "1":{
//         "time": "YYYY-MM-DDTHH:MM:SS.XXX
//         "trace": {
//             "columns": ["x","y","z","s","temperature","rho","j_para"],
//             "data": [[8.694,127.853,115.304,0.0,0.047,9.249,-5e-10],...,[8.698,127.253,114.768,0.800,0.0,9.244,-5e-10]]
//         },
//     }
// }
void FieldlinesState::saveStateToJson(const std::string& ABS_FILEPATH) {
    // Create the file
    const std::string EXT = ".json";
    std::ofstream ofs(ABS_FILEPATH + EXT, std::ofstream::trunc);
    if (!ofs.is_open()) {
        LERROR("Failed to save state to json file at location: " << ABS_FILEPATH << EXT);
        return;
    }
    LINFO("Saving fieldline state to: " << ABS_FILEPATH << EXT );

    json jColumns = {"x", "y", "z"};
    for (std::string s : _extraQuantityNames) {
        jColumns.push_back(s);
    }

    json jFile;

    const std::string TIME_STRING = Time(_triggerTime).ISO8601();

    const size_t N_LINES         = _lineStart.size();
    const size_t N_POINTS        = _vertexPositions.size();
    const size_t N_EXTRAS        = _extraQuantities.size();

    size_t pointIndex = 0;
    for (size_t lineIndex = 0; lineIndex < N_LINES; lineIndex++) {
        json jData = json::array();
        for (size_t i = 0; i < _lineCount[lineIndex]; i++, pointIndex++) {
            const glm::vec3 POS = _vertexPositions[pointIndex];
            json jDataElement = {POS.x, POS.y, POS.z};

            for (size_t extraIndex = 0; extraIndex < N_EXTRAS; extraIndex++) {
                jDataElement.push_back(_extraQuantities[extraIndex][pointIndex]);
            }
            jData.push_back(jDataElement);
        }

        jFile[std::to_string(lineIndex)] = {
            {"time", TIME_STRING},
            {"trace", {
                {"columns", jColumns},
                {"data", jData}
            }}
        };
    }

    //------------------------------ WRITE EVERYTHING TO FILE ------------------------------
    const int INDENTATION_SPACES = 2;
    ofs << std::setw(INDENTATION_SPACES) << jFile << std::endl;

    LINFO("Saved fieldline state to: " << ABS_FILEPATH << EXT );
}

// Returns one of the extra quantity vectors, _extraQuantities[INDEX].
// If INDEX is out of scope an empty vector is returned and the referenced bool will be false.
const vector<float>&  FieldlinesState::extraQuantity(const size_t INDEX,
                                                     bool& isSuccessful) const {
    if (INDEX < _extraQuantities.size()) {
        isSuccessful = true;
        return _extraQuantities[INDEX];
    }
    isSuccessful = false;
    // return empty vector which goes out of scope hence unusable!
    return std::vector<float>();
}

} // namespace openspace
