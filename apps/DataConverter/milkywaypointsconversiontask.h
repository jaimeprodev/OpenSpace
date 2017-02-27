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

#ifndef __OPENSPACE_APP_DATACONVERTER___MILKYWAYPOINTSCONVERSIONTASK___H__
#define __OPENSPACE_APP_DATACONVERTER___MILKYWAYPOINTSCONVERSIONTASK___H__

#include <apps/DataConverter/conversiontask.h>
#include <string>
#include <ghoul/glm.h>
#include <functional>
#include <modules/volume/textureslicevolumereader.h>
#include <modules/volume/rawvolumewriter.h>


namespace openspace {
namespace dataconverter {

/**
 * Converts ascii based point data
 * int64_t n
 * (float x, float y, float z, float r, float g, float b) * n
 * to a binary (floating point) representation with the same layout.
 */
class MilkyWayPointsConversionTask : public ConversionTask {
public:
    MilkyWayPointsConversionTask(const std::string& inFilename,
                                 const std::string& outFilename);
    
    void perform(const std::function<void(float)>& onProgress) override;
private:
    std::string _inFilename;    
    std::string _outFilename;
};

} // namespace dataconverter
} // namespace openspace

#endif // __OPENSPACE_APP_DATACONVERTER___MILKYWAYPOINTSCONVERSIONTASK___H__
