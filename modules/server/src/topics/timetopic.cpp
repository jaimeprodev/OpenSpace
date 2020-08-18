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

#include "modules/server/include/topics/timetopic.h"

#include <modules/server/include/connection.h>
#include <openspace/engine/globals.h>
#include <openspace/properties/property.h>
#include <openspace/query/query.h>
#include <openspace/util/timemanager.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/misc/profiling.h>

namespace {
    constexpr const char* EventKey = "event";
    constexpr const char* SubscribeEvent = "start_subscription";
    constexpr const char* UnsubscribeEvent = "stop_subscription";
    constexpr const std::chrono::milliseconds TimeUpdateInterval(50);
} // namespace

using nlohmann::json;

namespace openspace {

TimeTopic::TimeTopic()
    : _lastUpdateTime(std::chrono::system_clock::now())
{}

TimeTopic::~TimeTopic() {
    if (_timeCallbackHandle != UnsetOnChangeHandle) {
        global::timeManager.removeTimeChangeCallback(_timeCallbackHandle);
    }
    if (_deltaTimeCallbackHandle != UnsetOnChangeHandle) {
        global::timeManager.removeDeltaTimeChangeCallback(_deltaTimeCallbackHandle);
    }
}

bool TimeTopic::isDone() const {
    return _isDone;
}

void TimeTopic::handleJson(const nlohmann::json& json) {
    std::string event = json.at(EventKey).get<std::string>();
    if (event == UnsubscribeEvent) {
        _isDone = true;
        return;
    }

    sendFullTimeData();

    if (event != SubscribeEvent) {
        _isDone = true;
        return;
    }

    _timeCallbackHandle = global::timeManager.addTimeChangeCallback([this]() {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        if (now - _lastUpdateTime > TimeUpdateInterval) {
            sendCurrentTime();
        }
    });

    _deltaTimeCallbackHandle = global::timeManager.addDeltaTimeChangeCallback([this]() {
        // Throttle by last update,
        // but force update if pause state or target delta changes.

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        const double targetDeltaTime = global::timeManager.targetDeltaTime();
        const bool isPaused = global::timeManager.isPaused();
        const bool forceUpdate =
            isPaused != _lastPauseState || targetDeltaTime != _lastTargetDeltaTime;

        if (forceUpdate || now - _lastUpdateTime > TimeUpdateInterval) {
            sendFullTimeData();
        }
    });
}

void TimeTopic::sendCurrentTime() {
    ZoneScoped

    const json timeJson = {
        { "time", global::timeManager.time().ISO8601() }
    };
    const json payload = wrappedPayload(timeJson);
    _connection->sendJson(payload);
    _lastUpdateTime = std::chrono::system_clock::now();
}

void TimeTopic::sendFullTimeData() {
    const std::string currentTime = global::timeManager.time().ISO8601();
    const double deltaTime = global::timeManager.deltaTime();
    const double targetDeltaTime = global::timeManager.targetDeltaTime();
    const bool isPaused = global::timeManager.isPaused();

    const json timeJson = {
        { "time", currentTime },
        { "deltaTime", deltaTime},
        { "targetDeltaTime", targetDeltaTime},
        { "isPaused", isPaused },
    };

    _connection->sendJson(wrappedPayload(timeJson));
    _lastUpdateTime = std::chrono::system_clock::now();
    _lastPauseState = isPaused;
    _lastTargetDeltaTime = targetDeltaTime;
}

} // namespace openspace
