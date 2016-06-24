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
#ifndef __STATS_TRACKER_H__
#define __STATS_TRACKER_H__

#include <ghoul/logging/logmanager.h>
#include <ghoul/filesystem/filesystem>


#include <fstream>
#include <unordered_map>
#include <vector>
#include <set>
#include <memory>



namespace openspace {
    

    template <typename T> 
    struct StatsRecord : public std::unordered_map<std::string, T> {

    };

    template <typename T>
    struct StatsCollection : public std::vector<StatsRecord<T>> {
        std::set<std::string> keys;
    };


    template <typename T> class TemplatedStatsCollector{
    public:

        TemplatedStatsCollector(bool& enabled, const std::string& delimiter) 
            : _enabled(enabled) 
            , _delimiter(delimiter)
            , _writePos(0) { };

        ~TemplatedStatsCollector() { };

        void startNewRecord() {
            _data.push_back(StatsRecord<T>());
        }

        T& operator[](const std::string& name) {
            if (_enabled) {
                _data.keys.insert(name);
                return _data.back()[name];
            }
            else return _dummy;
        }

        T previous(const std::string& name) {
            if (_enabled && _data.size() > 1) {
                return _data[_data.size() - 2][name];   
            }
            return T();
        }

        bool hasRecordsToWrite() {
            return _writePos < _data.size();
        }

        void reset() {
            _data.clear();
            _writePos = 0;
        }

        void writeHeader(std::ostream& os) {
            auto keyIt = _data.keys.begin();
            os << *keyIt;
            while (++keyIt != _data.keys.end()) {
                os << _delimiter << *keyIt;
            }
        }

        void writeNextRecord(std::ostream& os) {
            if (hasRecordsToWrite()) {
                // output line by line
                StatsRecord<T>& record = _data[_writePos];

                // Access every key. Records with no entry will get a default value
                auto keyIt = _data.keys.begin();
                os << record[(*keyIt)];
                while (++keyIt != _data.keys.end()) {
                    os << _delimiter << record[(*keyIt)];
                }

                _writePos++;
            }
        }


    private:
        
        StatsCollection<T> _data;
        T _dummy; // used when disabled
        bool& _enabled;

        size_t _writePos;
        std::string _delimiter;

    };

    class StatsCollector {

    public:

        StatsCollector() = delete;

        StatsCollector(const std::string& filename, const std::string& delimiter = ",", bool enabled = true)
            : _filename(filename)
            , _enabled(enabled)
            , _delimiter(delimiter)
            , _hasWrittenHead(false)
            , i(TemplatedStatsCollector<int>(_enabled, delimiter))
            , d(TemplatedStatsCollector<double>(_enabled, delimiter))
        {

        };

        ~StatsCollector() {
            dumpToDisk();
        }
        
        void startNewRecord() {
            if (_enabled) {
                i.startNewRecord();
                d.startNewRecord();
            }
        }

        void disable() {
            _enabled = false;
        }

        void enable() {
            _enabled = true;
        }

        void dumpToDisk() {
            if (!_hasWrittenHead) {
                writeHead();
            }
            writeData();
        }

        TemplatedStatsCollector<int> i;
        TemplatedStatsCollector<double> d;

    private:
        void writeHead() {
            std::ofstream ofs(_filename);
            i.writeHeader(ofs); ofs << _delimiter; d.writeHeader(ofs); ofs << std::endl;
            ofs.close();
        }

        void writeData() {
            std::ofstream ofs(_filename, std::ofstream::out | std::ofstream::app);
            while (i.hasRecordsToWrite() || d.hasRecordsToWrite()) {
                i.writeNextRecord(ofs); ofs << _delimiter; d.writeNextRecord(ofs);
                ofs << std::endl;
            }
            i.reset(); d.reset();
            ofs.close();
        }

        std::string _filename;
        std::string _delimiter;
        bool _enabled;
        bool _hasWrittenHead;

    };

    /*

    template <typename T>
    struct StatsCsvWriter {
        StatsCsvWriter(const std::string& delimiter)
            : _delimiter(delimiter) { };

        virtual void write(const StatsCollection<T>& stats, const std::string& filename) {
            std::ofstream ofs(filename);
            
            // output headers
            auto keyIt = stats.keys.begin();
            ofs << *keyIt;
            while (keyIt++ != stats.keys.end()) {
                ofs << _delimiter << *keyIt;
            }
            ofs << std::endl;

            // output line by line
            for (const StatsRecord<T>& record : stats) {
                // Access every key. Records with no entry will get a default value
                auto keyIt = stats.keys.begin();
                ofs << record[*keyIt];
                while (keyIt++ != stats.keys.end()) {
                    ofs << _delimiter << record[*keyIt];
                }
                ofs << std::endl;
            }
            ofs.close();
        }

    private:
        std::string _delimiter;
    };
    */


} // namespace openspace





#endif  // __STATS_TRACKER_H__