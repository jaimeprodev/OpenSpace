/*****************************************************************************************
*                                                                                       *
* OpenSpace                                                                             *
*                                                                                       *
* Copyright (c) 2014-2019                                                               *
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

#include <modules/autonavigation/pathcurves.h>

#include <modules/autonavigation/helperfunctions.h>
#include <openspace/query/query.h>
#include <openspace/scene/scenegraphnode.h>
#include <ghoul/logging/logmanager.h>
#include <glm/gtx/projection.hpp>
#include <algorithm>
#include <vector>

namespace {
    constexpr const char* _loggerCat = "PathCurve";
    const double Epsilon = 1E-7;
} // namespace

namespace openspace::autonavigation {

PathCurve::~PathCurve() {}

const double PathCurve::length() const {
    return _totalLength;
}

glm::dvec3 PathCurve::positionAt(double relativeLength) {
    double u = curveParameter(relativeLength * _totalLength); // TODO: only use relative length?
    return interpolate(u);
}

// Compute the curve parameter from an arc length value, using a combination of
// Newton's method and bisection. Source:
// https://www.geometrictools.com/Documentation/MovingAlongCurveSpecifiedSpeed.pdf
// Input s is a length value, in the range [0, _length]
// Returns curve parameter in range [0, 1]
double PathCurve::curveParameter(double s) {
    if (s <= Epsilon) return 0.0;
    if (s >= _totalLength) return 1.0;

    unsigned int segmentIndex;
    for (segmentIndex = 1; segmentIndex < _nSegments; ++segmentIndex) {
        if (s <= _lengthSums[segmentIndex])
            break;
    }

    // initial guess for Newton's method
    double segmentS = s - _lengthSums[segmentIndex - 1];
    double segmentLength = _lengths[segmentIndex];

    const double uMin = _parameterIntervals[segmentIndex - 1];
    const double uMax = _parameterIntervals[segmentIndex];
    double u = uMin + (uMax - uMin) * (segmentS / segmentLength);

    const int nIterations = 40;

    // initialize root bounding limits for bisection
    double lower = uMin;
    double upper = uMax;

    for (int i = 0; i < nIterations; ++i) {
        double F = arcLength(uMin, u) - segmentS;

        const double tolerance = 0.1; // meters. Note that distances are very large
        if (std::abs(F) <= tolerance) {
            return u;
        }

        // generate a candidate for Newton's method
        double dfdu = approximatedDerivative(u, Epsilon); // > 0
        double uCandidate = u - F / dfdu;

        // update root-bounding interval and test candidate
        if (F > 0) {  // => candidate < u <= upper
            upper = u;
            u = (uCandidate <= lower) ? (upper + lower) / 2.0 : uCandidate;
        }
        else { // F < 0 => lower <= u < candidate
            lower = u;
            u = (uCandidate >= upper) ? (upper + lower) / 2.0 : uCandidate;
        }
    }

    // No root was found based on the number of iterations and tolerance. However, it is
    // safe to report the last computed u value, since it is within the segment interval
    return u;
}

// TODO: remove when not needed
// Created for debugging
std::vector<glm::dvec3> PathCurve::getPoints() {
    return _points;
}

void PathCurve::initParameterIntervals() {
    ghoul_assert(_nSegments > 0, "Cannot have a curve with zero segments!");
    _parameterIntervals.clear();
    _parameterIntervals.reserve(_nSegments + 1);

    // compute initial values, to be able to compute lengths
    double dt = 1.0 / _nSegments;
    _parameterIntervals.push_back(0.0);
    for (unsigned int i = 1; i < _nSegments; i++) {
        _parameterIntervals.push_back(dt * i);
    }
    _parameterIntervals.push_back(1.0);

    // lengths
    _lengths.clear();
    _lengths.reserve(_nSegments + 1);
    _lengthSums.clear();
    _lengthSums.reserve(_nSegments + 1);

    _lengths.push_back(0.0);
    _lengthSums.push_back(0.0);
    for (unsigned int i = 1; i <= _nSegments; i++) {
        double u = _parameterIntervals[i];
        double uPrev = _parameterIntervals[i - 1];
        _lengths.push_back(arcLength(uPrev, u));
        _lengthSums.push_back(_lengthSums[i - 1] + _lengths[i]);
    }
    _totalLength = _lengthSums.back();

    // scale parameterIntervals to better match arc lengths
    for (unsigned int i = 1; i <= _nSegments; i++) {
        _parameterIntervals[i] = _lengthSums[i] / _totalLength;
    }
}

double PathCurve::approximatedDerivative(double u, double h) {
    if (u <= h) {
        return (1.0 / h) * glm::length(interpolate(0.0 + h) - interpolate(0.0));
    }
    if (u >= 1.0 - h) {
        return (1.0 / h) * glm::length(interpolate(1.0) - interpolate(1.0 - h));
    }
    return (0.5 / h) * glm::length(interpolate(u + h) - interpolate(u - h));
}

double PathCurve::arcLength(double limit) {
    return arcLength(0.0, limit);
}

double PathCurve::arcLength(double lowerLimit, double upperLimit) {
    return helpers::fivePointGaussianQuadrature(
        lowerLimit,
        upperLimit,
        [this](double u) { return approximatedDerivative(u); }
    );
}

LinearCurve::LinearCurve(const Waypoint& start, const Waypoint& end) {
    _points.push_back(start.position());
    _points.push_back(end.position());
    _nSegments = 1;
    initParameterIntervals();
}

glm::dvec3 LinearCurve::interpolate(double u) {
    ghoul_assert(u >= 0 && u <= 1.0, "Interpolation variable out of range [0, 1]");
    return interpolation::linear(u, _points[0], _points[1]);
}

} // namespace openspace::autonavigation
