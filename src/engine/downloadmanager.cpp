/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2015                                                               *
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

#include <openspace/engine/downloadmanager.h>

#include <ghoul/filesystem/filesystem.h>
#include <ghoul/logging/logmanager.h>

#include <fstream>
#include <thread>

#ifdef OPENSPACE_CURL_ENABLED
#include <curl/curl.h>
#endif

namespace {
    const std::string _loggerCat = "DownloadManager";
    
    const std::string RequestIdentifier = "identifier";
    const std::string RequestFileVersion = "file_version";
    const std::string RequestApplicationVersion = "application_version";

    struct ProgressInformation {
        CURL* curl;
        openspace::DownloadManager::FileFuture* future;
        const openspace::DownloadManager::DownloadProgressCallback* callback;
    };

    size_t writeData(void* ptr, size_t size, size_t nmemb, FILE* stream) {
        size_t written;
        written = fwrite(ptr, size, nmemb, stream);
        return written;
    }


    int xferinfo(void* p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
    {
        if (dltotal == 0)
            return 0;

        ghoul_assert(p, "Passed progress information is nullptr");
        ProgressInformation* i = static_cast<ProgressInformation*>(p);
        ghoul_assert(i, "Passed pointer is not a ProgressInformation");
        ghoul_assert(i->curl, "CURL pointer is nullptr");
        ghoul_assert(i->future, "FileFuture is not initialized");
        ghoul_assert(i->callback, "Callback pointer is nullptr");

        ghoul_assert(dltotal, "Download total is 0");
        ghoul_assert(dlnow <= dltotal, "Downloaded filesize is bigger then total size");
        i->future->progress = static_cast<float>(dlnow) / static_cast<float>(dltotal);

        if (*(i->callback)) {
            // The callback function is a pointer to an std::function; that is the reason
            // for the excessive referencing
            (*(i->callback))(*(i->future));
        }


        //CURL* curl = myp->curl;
        //double curtime = 0;
        //curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);
        //fprintf(stderr, "UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
        //        "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
        //        "\r\n",
        //        ulnow, ultotal, dlnow, dltotal);
 
        return 0;
    }
}

namespace openspace {

DownloadManager::DownloadManager(std::string requestURL, int applicationVersion)
    : _requestURL(std::move(requestURL))
    , _applicationVersion(std::move(applicationVersion))
{
    curl_global_init(CURL_GLOBAL_ALL);
    // TODO: Check if URL is accessible ---abock
    // TODO: Allow for multiple requestURLs
}

DownloadManager::FileFuture* DownloadManager::downloadFile(
    const std::string& url,
    const ghoul::filesystem::File& file,
    DownloadFinishedCallback finishedCallback,
    DownloadProgressCallback progressCallback)
{
    FileFuture* future = new FileFuture(
        file.filename()
    );
    FILE* fp = fopen(file.path().c_str(), "wb");

    LDEBUG("Starting download for file: '" << url <<
    "' into file '" << file.path() << "'");
    std::thread([url, finishedCallback, progressCallback, future, fp]() {
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);

            if (progressCallback) {
                ProgressInformation p = {
                    curl,
                    future,
                    &progressCallback
                };
                curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
                curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &p);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            }

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp);

            if (res != CURLE_OK) {
                future->errorMessage = curl_easy_strerror(res);
            }

            if (finishedCallback)
                finishedCallback(*future);
        }
    }).detach();

    return future;
}

//bool DownloadManager::downloadRequestFiles(
//    const std::string& identifier,
//    const ghoul::filesystem::Directory& destination,
//    int version,
//    DownloadFinishedCallback finishedCallback,
//    DownloadProgressCallback progressCallback)
//{
//    bool s = FileSys.createDirectory(destination, true);
//    // TODO: Check s ---abock
//    // TODO: Escaping is necessary ---abock
//    const std::string fullRequest =_requestURL + "?" +
//        RequestIdentifier + "=" + identifier + "&" +
//        RequestFileVersion + "=" + std::to_string(version) + "&" +
//        RequestApplicationVersion + "=" + std::to_string(_applicationVersion);
//    LDEBUG("Request: " << fullRequest);
//
//    std::string requestFile = absPath("${TEMPORARY}/" + identifier);
//    LDEBUG("Request File: " << requestFile);
//
//    //std::vector<std::thread> threads;
//
//    bool success = downloadFile(
//        fullRequest,
//        requestFile,
//        [destination, &progressCallback](const ghoul::filesystem::File& f) {
//            LDEBUG("Finished: " << f.path());
//            std::ifstream temporary(f.path());
//            std::string line;
//            int nFiles = 0;
//            int nFinished = 0;
//            while (std::getline(temporary, line)) {
//                ++nFiles;
//                std::string file = ghoul::filesystem::File(line).filename();
//
//                LDEBUG("\tLine: " << line << " ; Dest: " << destination.path() + "/" + file);
//                //threads.push_back(
//                    //std::thread([&nFinished, line, destination, file, progressCallback](){
//                    DlManager.downloadFile(
//                        line,
//                        destination.path() + "/" + file,
//                        [&nFinished](const ghoul::filesystem::File& f) { ++nFinished; },
//                        [&progressCallback](const ghoul::filesystem::File& f, float progress) { progressCallback(f, progress); }
//                    //);}
//                    //)
//                );
//            }
//        }
//    );
//
//    //for (std::thread& t : threads)
//        //t.join();
//
//
//
//    return true;
//}



DownloadManager::FileFuture::FileFuture(std::string file)
    : progress(0.f)
    , filePath(std::move(file))
{}

} // namespace openspace
