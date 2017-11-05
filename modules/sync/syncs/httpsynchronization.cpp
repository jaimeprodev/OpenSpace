﻿/*****************************************************************************************
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

#include "httpsynchronization.h"

#include <openspace/util/httprequest.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/filesystem/filesystem.h>
#include <openspace/documentation/verifier.h>

#include <sstream>

namespace {
    const char* _loggerCat = "HttpSynchronization";
    const char* KeyIdentifier = "Identifier";
    const char* KeyVersion = "Version";
}

namespace openspace {

HttpSynchronization::HttpSynchronization(const ghoul::Dictionary& dict)
    : openspace::ResourceSynchronization()
{
    documentation::testSpecificationAndThrow(
        Documentation(),
        dict,
        "HttpSynchroniztion"
    );

    _identifier = dict.value<std::string>(KeyIdentifier);
    _version = static_cast<int>(dict.value<double>(KeyVersion));
}

documentation::Documentation HttpSynchronization::Documentation() {
    using namespace openspace::documentation;
    return {
        "HttpSynchronization",
        "http_synchronization",
        {
            {
                KeyIdentifier,
                new StringVerifier,
                Optional::No,
                "A unique identifier for this resource"
            },
            {
                KeyVersion,
                new IntVerifier,
                Optional::No,
                "The version of this resource"
            }
        }
    };
}

std::string HttpSynchronization::directory() {
    ghoul::filesystem::Directory d(
        _syncRoot +
        ghoul::filesystem::FileSystem::PathSeparator +
        _identifier +
        ghoul::filesystem::FileSystem::PathSeparator +
        std::to_string(_version)
    );

    return FileSys.absPath(d);
}

void HttpSynchronization::synchronize() {
    // TODO: First check if files exist.
    std::string listUrl = "http://data.openspaceproject.com/request?identifier=" +
                       _identifier +
                       "&file_version=" +
                        std::to_string(_version) +
                        "&application_version=" +
                        std::to_string(1);

    HttpMemoryDownload fileListDownload(listUrl);
    HttpRequest::RequestOptions opt;
    opt.requestTimeoutSeconds = 0;
    fileListDownload.download(opt);
    
    const std::vector<char>& buffer = fileListDownload.downloadedData();

    LINFO(std::string(buffer.begin(), buffer.end()));

    std::istringstream fileList(std::string(buffer.begin(), buffer.end()));
    std::string line = "";
    while (fileList >> line) {
        std::string filename = ghoul::filesystem::File(line, ghoul::filesystem::File::RawPath::Yes).filename();

        std::string fileDestination = directory() +
            ghoul::filesystem::FileSystem::PathSeparator +
            filename;

        HttpFileDownload fileDownload(line, fileDestination);
        fileDownload.download(opt);
    }

    resolve();
}

} // namespace openspace
