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

#ifndef __CULLING_H__
#define __CULLING_H__

#include <memory>
#include <glm/glm.hpp>

// open space includes

#include <openspace/rendering/renderable.h>

#include <modules/globebrowsing/geodetics/geodetic2.h>
#include <modules/globebrowsing/geodetics/ellipsoid.h>
#include <modules/globebrowsing/rendering/aabb.h>



namespace openspace {

    using namespace glm;



    enum PointLocation {
        AboveLeft = 0, Above, AboveRight, // 0, 1, 2
        Left, Inside, Right,      // 3, 4, 5
        BelowLeft, Below, BelowRight  // 6, 7, 8
    };


    class FrustumCuller {
    public:

        FrustumCuller();
        ~FrustumCuller();

        static const AABB3 viewFrustum;

        /**
        Returns true if the point is inside the view frustrum defined in RenderData.
        The third argument marginScreenSpace is added to the default screen space
        boundaries. E.g. for marginScreenSpace = {0.2, 0.2}, all points with
        1.2 < x,y < 1.2 would cause isVisible to return true.
        */
        static bool isVisible(
            const RenderData& data,
            const dvec3& point);


        /**
        Returns false if the patch element is guaranteed to be outside the view
        frustrum, and true is the patch element MAY be inside the view frustrum.
        */
        static bool isVisible(
            const RenderData& data,
            const GeodeticPatch& patch,
            const Ellipsoid& ellipsoid);


        static bool isVisible(
            const RenderData& data,
            const GeodeticPatch& patch,
            const Ellipsoid& ellipsoid,
            const Scalar maxHeight);

    private:

        /**
        The optional screen space margin vector is used to resize area defining
        what is considered to be inside the view frustrum.
        */
        static PointLocation testPoint(
            const glm::dvec2& pointScreenSpace,
            const glm::dvec2& marginScreenSpace = vec2(0));

        static bool testPoint(
            const glm::dvec3& pointScreenSpace,
            const glm::dvec3& marginScreenSpace = vec3(0));


        static glm::dvec2 transformToScreenSpace(
            const dvec3& point,
            const dmat4x4& modelViewProjection);

    };

    class HorizonCuller {
    public:
        HorizonCuller();
        ~HorizonCuller();

        static bool isVisible(
            const Vec3& cameraPosition,
            const Vec3& globePosition,
            const Vec3& objectPosition,
            Scalar objectBoundingSphereRadius,
            Scalar minimumGlobeRadius);

        static bool isVisible(
            const RenderData& data,
            const GeodeticPatch& patch,
            const Ellipsoid& ellipsoid,
            float height);

    private:
    };


}  // namespace openspace

#endif  // __CULLING_H__