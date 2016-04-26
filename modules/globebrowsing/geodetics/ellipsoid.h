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

#ifndef __ELLIPSOID_H__
#define __ELLIPSOID_H__

#include <modules/globebrowsing/geodetics/geodetic2.h>

namespace openspace {

	/**
	This class is based largely on the Ellipsoid class defined in the book
	"3D Engine Design for Virtual Globes". Most planets or planetary objects are better
	described using ellipsoids than spheres.
	*/
class Ellipsoid {
public:
	/**
	\param radii defines three radii for the Ellipsoid
	*/
	Ellipsoid(Vec3 radii);
	
	/**
	\param x defines the radius in x direction.
	\param y defines the radius in y direction.
	\param z defines the radius in z direction.
	*/
	Ellipsoid(Scalar x, Scalar y, Scalar z);
	~Ellipsoid();

	/**
	Scales a point along the geocentric normal and places it on the surface of the
	Ellipsoid.
	\param p is a point in the cartesian coordinate system to be placed on the surface
	of the Ellipsoid
	*/
	Vec3 scaleToGeocentricSurface(const Vec3& p) const;
	/**
	Scales a point along the geodetic normal and places it on the surface of the
	Ellipsoid.
	\param p is a point in the cartesian coordinate system to be placed on the surface
	of the Ellipsoid
	*/
	Vec3 scaleToGeodeticSurface(const Vec3& p) const;

	Vec3 geodeticSurfaceNormal(const Vec3& p) const;
	Vec3 geodeticSurfaceNormal(Geodetic2 geodetic2) const;

	Geodetic2 cartesianToGeodetic2(const Vec3& p) const;
	Vec3 geodetic2ToCartesian(const Geodetic2& geodetic2) const;
	Vec3 geodetic3ToCartesian(const Geodetic3& geodetic3) const;

private:
	
	struct EllipsoidCache
	{
		const Vec3 _radiiSquared;
		const Vec3 _oneOverRadiiSquared;
		const Vec3 _radiiToTheFourth;
	};

	const Vec3 _radii;
	const EllipsoidCache _cachedValues;
};
} // namespace openspace

#endif // __ELLIPSOID_H__
