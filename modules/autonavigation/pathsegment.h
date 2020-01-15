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

#ifndef __OPENSPACE_MODULE___PATHSEGMENT___H__
#define __OPENSPACE_MODULE___PATHSEGMENT___H__

#include <ghoul/glm.h>
#include <vector>

namespace openspace::autonavigation {

struct CameraState {
    glm::dvec3 position;
    glm::dquat rotation;
    std::string referenceNode;
};

enum CurveType {
    Bezier, 
    Bezier2,
    Linear,
    Linear2
};

// TODO: move to their own file
class PathCurve {
public:
    virtual ~PathCurve() = 0;
    virtual glm::dvec3 interpolate(double t) = 0;
protected:
    // the points used for creating the curve (e.g. control points of a Bezier curve)
    std::vector<glm::dvec3> _points; 
};

class BezierCurve : public PathCurve {
public :
    BezierCurve(CameraState& start, CameraState& end);
    glm::dvec3 interpolate(double t);
};

class Bezier2Curve : public PathCurve {
public:
    Bezier2Curve(CameraState& start, CameraState& end);
    glm::dvec3 interpolate(double t);
};

class LinearCurve : public PathCurve {
public:
    LinearCurve(CameraState& start, CameraState& end);
    glm::dvec3 interpolate(double t);
};

class Linear2Curve : public PathCurve {
public:
    Linear2Curve(CameraState& start, CameraState& end);
    glm::dvec3 interpolate(double t);
};

class PathSegment {
public:
    PathSegment(CameraState start, CameraState end, double startTime, CurveType type = Linear2);
    ~PathSegment() = default;

    // Mutators
    void setStart(CameraState cs);
    void setDuration(double d);

    // Accessors
    const CameraState start() const;
    const CameraState end() const;
    const double duration() const;
    const double startTime() const;

    const glm::vec3 getPositionAt(double t) const;
    const glm::dquat getRotationAt(double t) const;
    const glm::dquat getLookAtRotation(double t, glm::dvec3 currentPos, glm::dvec3 up) const;

private: 
    CameraState _start;
    CameraState _end;
    double _duration; 
    double _startTime; 
    CurveType _curveType; // Ideally, we don't want to need to save this

    std::shared_ptr<PathCurve> _curve; // OBS! Does it make more sense to use unique_ptr? However, then PathSegments cannot be copied.
};

} // namespace openspace::autonavigation

#endif // __OPENSPACE_MODULE___PATHSEGMENT___H__
