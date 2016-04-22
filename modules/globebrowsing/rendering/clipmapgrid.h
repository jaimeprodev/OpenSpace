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

#ifndef __CLIPMAPGEOMETRY_H__
#define __CLIPMAPGEOMETRY_H__

#include <modules/globebrowsing/rendering/grid.h>

#include <vector>
#include <glm/glm.hpp>

namespace openspace {

//////////////////////////////////////////////////////////////////////////////////////////
//							CLIPMAP GRID	(Abstract class)							//
//////////////////////////////////////////////////////////////////////////////////////////

class ClipMapGrid : public Grid
{
public:
	ClipMapGrid(unsigned int resolution);

	~ClipMapGrid();

	virtual int xResolution() const;
	virtual int yResolution() const;
	int resolution() const;
};

//////////////////////////////////////////////////////////////////////////////////////////
//									OUTER CLIPMAP GRID									//
//////////////////////////////////////////////////////////////////////////////////////////

class OuterClipMapGrid : public ClipMapGrid
{
public:
	OuterClipMapGrid(unsigned int resolution);

	~OuterClipMapGrid();

protected:
	virtual std::vector<GLuint>		CreateElements(int xRes, int yRes);
	virtual std::vector<glm::vec4>	CreatePositions(int xRes, int yRes);
	virtual std::vector<glm::vec2>	CreateTextureCoordinates(int xRes, int yRes);
	virtual std::vector<glm::vec3>	CreateNormals(int xRes, int yRes);

private:
	void validate(int xRes, int yRes);

	static size_t numVerticesBottom(int resolution);
	static size_t numVerticesLeft(int resolution);
	static size_t numVerticesRight(int resolution);
	static size_t numVerticesTop(int resolution);

	static size_t numElements(int resolution);
	static size_t numVertices(int resolution);
};

//////////////////////////////////////////////////////////////////////////////////////////
//									INNER CLIPMAP GRID									//
//////////////////////////////////////////////////////////////////////////////////////////

class InnerClipMapGrid : public ClipMapGrid
{
public:
	InnerClipMapGrid(unsigned int resolution);

	~InnerClipMapGrid();

protected:
	virtual std::vector<GLuint>		CreateElements(int xRes, int yRes);
	virtual std::vector<glm::vec4>	CreatePositions(int xRes, int yRes);
	virtual std::vector<glm::vec2>	CreateTextureCoordinates(int xRes, int yRes);
	virtual std::vector<glm::vec3>	CreateNormals(int xRes, int yRes);

private:
	void validate(int xRes, int yRes);

	static size_t numElements(int resolution);
	static size_t numVertices(int resolution);
};

} // namespace openspace
#endif // __CLIPMAPGEOMETRY_H__