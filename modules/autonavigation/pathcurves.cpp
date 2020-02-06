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

namespace {
constexpr const char* _loggerCat = "PathCurve";
} // namespace

namespace openspace::autonavigation {

PathCurve::~PathCurve() {}

// Approximate the curve length by dividing the curve into smaller linear 
// segments and accumulate their length
double PathCurve::arcLength(double tLimit) {
    double dt = 0.01; // TODO: choose a good dt
    double sum = 0.0;
    for (double t = 0.0; t <= tLimit - dt; t += dt) {
        double ds = glm::length(valueAt(t + dt) - valueAt(t));
        sum += ds;
    }
    return sum;
}

// TODO: remove when not needed
// Created for debugging
std::vector<glm::dvec3> PathCurve::getPoints() {
    return _points;
}

Bezier3Curve::Bezier3Curve(CameraState& start, CameraState& end) {
    glm::dvec3 startNodePos = sceneGraphNode(start.referenceNode)->worldPosition();
    glm::dvec3 endNodePos = sceneGraphNode(end.referenceNode)->worldPosition();
    double startNodeRadius = sceneGraphNode(start.referenceNode)->boundingSphere();
    double endNodeRadius = sceneGraphNode(end.referenceNode)->boundingSphere();

    glm::dvec3 startNodeToStartPos = start.position - startNodePos;
    glm::dvec3 endNodeToEndPos = end.position - endNodePos;

    double startTangentLength = 2.0 * startNodeRadius;
    double endTangentLength = 2.0 * endNodeRadius;
    glm::dvec3 startTangentDirection = normalize(startNodeToStartPos);
    glm::dvec3 endTangentDirection = normalize(endNodeToEndPos);

    // Start by going outwards
    _points.push_back(start.position);
    _points.push_back(start.position + startTangentLength * startTangentDirection);

    if (start.referenceNode != end.referenceNode) {

        glm::dvec3 startNodeToEndNode = endNodePos - startNodePos;
        glm::dvec3 startToEndDirection = normalize(end.position - start.position);

        // Assuming we move straigh out to point to a distance proportional to radius, angle is enough to check collision risk
        double cosStartAngle = glm::dot(startTangentDirection, startToEndDirection);
        double cosEndAngle = glm::dot(endTangentDirection, startToEndDirection);
        double cosStartDir = glm::dot(startTangentDirection, endTangentDirection);

        //TODO: investigate suitable values, could be risky close to surface..
        bool TARGET_BEHIND_STARTNODE = cosStartAngle < -0.8;
        bool TARGET_BEHIND_ENDNODE = cosEndAngle > 0.8;
        bool TARGET_IN_OPPOSITE_DIRECTION = cosStartAngle > 0.7;

        // Avoid collision with startnode by adding control points on the side of it
        if (TARGET_BEHIND_STARTNODE) {
            glm::dvec3 parallell = glm::proj(startNodeToStartPos, startNodeToEndNode);
            glm::dvec3 orthogonal = normalize(startNodeToStartPos - parallell);
            double dist = 5.0 * startNodeRadius;
            glm::dvec3 extraKnot = startNodePos + dist * orthogonal;

            _points.push_back(extraKnot + parallell);
            _points.push_back(extraKnot);
            _points.push_back(extraKnot - parallell);
        }

        // Zoom out, to get a better understanding in a 180 degree turn situation
        if (TARGET_IN_OPPOSITE_DIRECTION) {
            glm::dvec3 parallell = glm::proj(startNodeToStartPos, startNodeToEndNode);
            glm::dvec3 orthogonal = normalize(startNodeToStartPos - parallell);
            double dist = 0.5 * length(startNodeToEndNode);
            // Distant middle point
            glm::dvec3 extraKnot = startNodePos + dist * normalize(parallell) + 3.0 * dist * orthogonal;

            _points.push_back(extraKnot - 0.3 * dist *  normalize(parallell));
            _points.push_back(extraKnot);
            _points.push_back(extraKnot + 0.3 * dist *  normalize(parallell));
        }

        // Avoid collision with endnode by adding control points on the side of it
        if (TARGET_BEHIND_ENDNODE) {
            glm::dvec3 parallell = glm::proj(endNodeToEndPos, startNodeToEndNode);
            glm::dvec3 orthogonal = normalize(endNodeToEndPos - parallell);
            double dist = 5.0 * endNodeRadius;
            glm::dvec3 extraKnot = endNodePos + dist * orthogonal;

            _points.push_back(extraKnot - parallell);
            _points.push_back(extraKnot);
            _points.push_back(extraKnot + parallell);
        }
    }

    _points.push_back(end.position + endTangentLength * endTangentDirection);
    _points.push_back(end.position);
}

glm::dvec3 Bezier3Curve::valueAt(double t) {
    return interpolation::piecewiseCubicBezier(t, _points);
}

LinearCurve::LinearCurve(CameraState& start, CameraState& end) {
    _points.push_back(start.position);
    _points.push_back(end.position);
}

glm::dvec3 LinearCurve::valueAt(double t) {
    return interpolation::linear(t, _points[0], _points[1]);
}

// TODO: Iprove handling of pauses
PauseCurve::PauseCurve(CameraState& state) {
    _points.push_back(state.position);
}

glm::dvec3 PauseCurve::valueAt(double t) {
    return _points[0];
}

} // namespace openspace::autonavigation
