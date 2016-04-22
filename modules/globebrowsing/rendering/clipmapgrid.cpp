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

#include <modules/globebrowsing/rendering/clipmapgrid.h>

#include <ghoul/opengl/ghoul_gl.h>

namespace {
	const std::string _loggerCat = "ClipMapGrid";
}

namespace openspace {

//////////////////////////////////////////////////////////////////////////////////////////
//							CLIPMAP GRID	(Abstract class)							//
//////////////////////////////////////////////////////////////////////////////////////////

ClipMapGrid::ClipMapGrid(unsigned int resolution)
	: Grid(resolution, resolution, Geometry::Positions::No, Geometry::TextureCoordinates::Yes, Geometry::Normals::No)
{

}

ClipMapGrid::~ClipMapGrid()
{

}

int ClipMapGrid::xResolution() const {
	return resolution();
}

int ClipMapGrid::yResolution() const {
	return resolution();
}

int ClipMapGrid::resolution() const {
	return _xRes;
}

//////////////////////////////////////////////////////////////////////////////////////////
//									OUTER CLIPMAP GRID									//
//////////////////////////////////////////////////////////////////////////////////////////

OuterClipMapGrid::OuterClipMapGrid(unsigned int resolution)
: ClipMapGrid(resolution)
{
	_geometry = std::unique_ptr<Geometry>(new Geometry(
		CreateElements(resolution, resolution),
		Geometry::Positions::No,
		Geometry::TextureCoordinates::Yes,
		Geometry::Normals::No));

	_geometry->setVertexTextureCoordinates(CreateTextureCoordinates(resolution, resolution));
}

OuterClipMapGrid::~OuterClipMapGrid()
{

}

size_t OuterClipMapGrid::numElements(int resolution)
{
	int numElementsInTotalSquare = 6 * (resolution + 1) * (resolution + 1);
	int numElementsInHole = 6 * (resolution / 4 * resolution / 4);
	return numElementsInTotalSquare - numElementsInHole;
}

size_t OuterClipMapGrid::numVerticesBottom(int resolution)
{
	return (resolution + 1 + 2) * (resolution / 4 + 1 + 1);
}

size_t OuterClipMapGrid::numVerticesLeft(int resolution)
{
	return (resolution / 4 + 1 + 1) * (resolution / 2 + 1);
}

size_t OuterClipMapGrid::numVerticesRight(int resolution)
{
	return (resolution / 4 + 1 + 1) * (resolution / 2 + 1);
}

size_t OuterClipMapGrid::numVerticesTop(int resolution)
{
	return (resolution + 1 + 2) * (resolution / 4 + 1 + 1);
}

size_t OuterClipMapGrid::numVertices(int resolution)
{
	return	numVerticesBottom(resolution) +
			numVerticesLeft(resolution) +
			numVerticesRight(resolution) +
			numVerticesTop(resolution);
}

void OuterClipMapGrid::validate(int xRes, int yRes) {
	
	ghoul_assert(xRes == yRes,
		"Resolution must be equal in x and in y. ");
	int resolution = xRes;
	ghoul_assert(resolution >= 8,
		"Resolution must be at least 8. (" << resolution << ")");
	ghoul_assert(resolution == pow(2, int(log2(resolution))),
		"Resolution must be a power of 2. (" << resolution << ")");
}

std::vector<GLuint> OuterClipMapGrid::CreateElements(int xRes, int yRes) {
	validate(xRes, yRes);
	int resolution = xRes;
	std::vector<GLuint> elements;
	elements.reserve(numElements(resolution));
	
	// The clipmap geometry is built up by four parts as follows:
	// 0 = Bottom part
	// 1 = Left part
	// 2 = Right part
	// 3 = Top part
	//
	// 33333333
	// 33333333
	// 11    22
	// 11    22
	// 00000000
	// 00000000


	// x    v01---v11   x ..
	//       |  /  |
	// x    v00---v10   x ..
	//
	// x	x     x     x ..
	// :    :     :     :

	unsigned int previousVerts[4];
	previousVerts[0] = 0;
	previousVerts[1] = previousVerts[0] + numVerticesBottom(resolution);
	previousVerts[2] = previousVerts[1] + numVerticesLeft(resolution);
	previousVerts[3] = previousVerts[2] + numVerticesRight(resolution);

	// Build the bottom part of the clipmap geometry
	for (unsigned int y = 0; y < resolution / 4 + 1; y++) {
		for (unsigned int x = 0; x < resolution + 2; x++) {
			GLuint v00 = previousVerts[0] + (y + 0) * (resolution + 3) + x + 0;
			GLuint v10 = previousVerts[0] + (y + 0) * (resolution + 3) + x + 1;
			GLuint v01 = previousVerts[0] + (y + 1) * (resolution + 3) + x + 0;
			GLuint v11 = previousVerts[0] + (y + 1) * (resolution + 3) + x + 1;

			elements.push_back(v00);
			elements.push_back(v10);
			elements.push_back(v11);

			elements.push_back(v00);
			elements.push_back(v11);
			elements.push_back(v01);
		}
	}

	// Build the left part of the clipmap geometry
	for (unsigned int y = 0; y < resolution / 2; y++) {
		for (unsigned int x = 0; x < resolution / 4 + 1; x++) {
			GLuint v00 = previousVerts[1] + (y + 0) * (resolution / 4 + 2) + x + 0;
			GLuint v10 = previousVerts[1] + (y + 0) * (resolution / 4 + 2) + x + 1;
			GLuint v01 = previousVerts[1] + (y + 1) * (resolution / 4 + 2) + x + 0;
			GLuint v11 = previousVerts[1] + (y + 1) * (resolution / 4 + 2) + x + 1;

			elements.push_back(v00);
			elements.push_back(v10);
			elements.push_back(v11);

			elements.push_back(v00);
			elements.push_back(v11);
			elements.push_back(v01);
		}
	}

	// Build the left part of the clipmap geometry
	for (unsigned int y = 0; y < resolution / 2; y++) {
		for (unsigned int x = 0; x < resolution / 4 + 1; x++) {
			GLuint v00 = previousVerts[2] + (y + 0) * (resolution / 4 + 2) + x + 0;
			GLuint v10 = previousVerts[2] + (y + 0) * (resolution / 4 + 2) + x + 1;
			GLuint v01 = previousVerts[2] + (y + 1) * (resolution / 4 + 2) + x + 0;
			GLuint v11 = previousVerts[2] + (y + 1) * (resolution / 4 + 2) + x + 1;

			elements.push_back(v00);
			elements.push_back(v10);
			elements.push_back(v11);

			elements.push_back(v00);
			elements.push_back(v11);
			elements.push_back(v01);
		}
	}

	// Build the left part of the clipmap geometry
	for (unsigned int y = 0; y < resolution / 4 + 1; y++) {
		for (unsigned int x = 0; x < resolution + 2; x++) {
			GLuint v00 = previousVerts[3] + (y + 0) * (resolution + 3) + x + 0;
			GLuint v10 = previousVerts[3] + (y + 0) * (resolution + 3) + x + 1;
			GLuint v01 = previousVerts[3] + (y + 1) * (resolution + 3) + x + 0;
			GLuint v11 = previousVerts[3] + (y + 1) * (resolution + 3) + x + 1;

			elements.push_back(v00);
			elements.push_back(v10);
			elements.push_back(v11);

			elements.push_back(v00);
			elements.push_back(v11);
			elements.push_back(v01);
		}
	}
	return elements;
}

std::vector<glm::vec4> OuterClipMapGrid::CreatePositions(int xRes, int yRes)
{
	validate(xRes, yRes);
	int resolution = xRes;
	std::vector<glm::vec4> positions;
	positions.reserve(numVertices(resolution));
	std::vector<glm::vec2> templateTextureCoords = CreateTextureCoordinates(xRes, yRes);

	// Copy from 2d texture coordinates and use as template to create positions
	for (unsigned int i = 0; i < templateTextureCoords.size(); i++) {
		positions.push_back(
			glm::vec4(
				templateTextureCoords[i].x,
				templateTextureCoords[i].y,
				0,
				1));
	}

	return positions;
}


std::vector<glm::vec2> OuterClipMapGrid::CreateTextureCoordinates(int xRes, int yRes){
	validate(xRes, yRes);
	int resolution = xRes;
	std::vector<glm::vec2> textureCoordinates;
	textureCoordinates.reserve(numVertices(resolution));

	// Build the bottom part of the clipmap geometry
	for (int y = -1; y < resolution / 4 + 1; y++) {
		for (int x = -1; x < resolution + 2; x++) {
			textureCoordinates.push_back(glm::vec2(
				static_cast<float>(x) / resolution,
				static_cast<float>(y) / resolution));
		}
	}
	
	// Build the left part of the clipmap geometry
	for (int y = resolution / 4; y < 3 * resolution / 4 + 1; y++) {
		for (int x = -1; x < resolution / 4 + 1; x++) {
			textureCoordinates.push_back(glm::vec2(
				static_cast<float>(x) / resolution,
				static_cast<float>(y) / resolution));
		}
	}
	
	// Build the right part of the clipmap geometry
	for (int y = resolution / 4; y < 3 * resolution / 4 + 1; y++) {
		for (int x = 3 * resolution / 4; x < resolution + 2; x++) {
			float u = static_cast<float>(x) / resolution;
			float v = static_cast<float>(y) / resolution;
			textureCoordinates.push_back(glm::vec2(u, v));
		}
	}
	
	// Build the top part of the clipmap geometry
	for (int y = 3 * resolution / 4; y < resolution + 2; y++) {
		for (int x = -1; x < resolution + 2; x++) {
			textureCoordinates.push_back(glm::vec2(
				static_cast<float>(x) / resolution,
				static_cast<float>(y) / resolution));
		}
	}
	
	return textureCoordinates;
}

std::vector<glm::vec3> OuterClipMapGrid::CreateNormals(int xRes, int yRes) {
	validate(xRes, yRes);
	int resolution = xRes;
	std::vector<glm::vec3> normals;
	normals.reserve(numVertices(resolution));

	for (int y = -1; y < resolution + 2; y++) {
		for (int x = -1; x < resolution + 2; x++) {
			normals.push_back(glm::vec3(0, 0, 1));
		}
	}

	return normals;
}

//////////////////////////////////////////////////////////////////////////////////////////
//									INNER CLIPMAP GRID									//
//////////////////////////////////////////////////////////////////////////////////////////


InnerClipMapGrid::InnerClipMapGrid(unsigned int resolution)
	: ClipMapGrid(resolution)
{
	_geometry = std::unique_ptr<Geometry>(new Geometry(
		CreateElements(resolution, resolution),
		Geometry::Positions::No,
		Geometry::TextureCoordinates::Yes,
		Geometry::Normals::No));

	_geometry->setVertexTextureCoordinates(CreateTextureCoordinates(resolution, resolution));
}

InnerClipMapGrid::~InnerClipMapGrid()
{

}

size_t InnerClipMapGrid::numElements(int resolution)
{
	return resolution * resolution * 6;
}

size_t InnerClipMapGrid::numVertices(int resolution)
{
	return (resolution + 1) * (resolution + 1);
}

void InnerClipMapGrid::validate(int xRes, int yRes) {

	ghoul_assert(xRes == yRes,
		"Resolution must be equal in x and in y. ");
	int resolution = xRes;
	ghoul_assert(resolution >= 1,
		"Resolution must be at least 1. (" << resolution << ")");
}

std::vector<GLuint> InnerClipMapGrid::CreateElements(int xRes, int yRes) {
	validate(xRes, yRes);
	int resolution = xRes;
	std::vector<GLuint> elements;
	elements.reserve(numElements(resolution));

	// x    v01---v11   x ..
	//       |  /  |
	// x    v00---v10   x ..
	//
	// x	x     x     x ..
	// :    :     :     :

	for (unsigned int y = 0; y < resolution + 2; y++) {
		for (unsigned int x = 0; x < resolution + 2; x++) {
			GLuint v00 = (y + 0) * (resolution + 3) + x + 0;
			GLuint v10 = (y + 0) * (resolution + 3) + x + 1;
			GLuint v01 = (y + 1) * (resolution + 3) + x + 0;
			GLuint v11 = (y + 1) * (resolution + 3) + x + 1;

			elements.push_back(v00);
			elements.push_back(v10);
			elements.push_back(v11);

			elements.push_back(v00);
			elements.push_back(v11);
			elements.push_back(v01);
		}
	}

	return elements;
}

std::vector<glm::vec4> InnerClipMapGrid::CreatePositions(int xRes, int yRes)
{
	validate(xRes, yRes);
	int resolution = xRes;
	std::vector<glm::vec4> positions;
	positions.reserve(numVertices(resolution));
	std::vector<glm::vec2> templateTextureCoords = CreateTextureCoordinates(xRes, yRes);

	// Copy from 2d texture coordinates and use as template to create positions
	for (unsigned int i = 0; i < templateTextureCoords.size(); i++) {
		positions.push_back(
			glm::vec4(
				templateTextureCoords[i].x,
				templateTextureCoords[i].y,
				0,
				1));
	}

	return positions;
}


std::vector<glm::vec2> InnerClipMapGrid::CreateTextureCoordinates(int xRes, int yRes) {
	validate(xRes, yRes);
	int resolution = xRes;
	std::vector<glm::vec2> textureCoordinates;
	textureCoordinates.reserve(numVertices(resolution));

	// Build the bottom part of the clipmap geometry
	for (int y = -1; y < resolution + 2; y++) {
		for (int x = -1; x < resolution + 2; x++) {
			textureCoordinates.push_back(glm::vec2(
				static_cast<float>(x) / resolution,
				static_cast<float>(y) / resolution));
		}
	}

	return textureCoordinates;
}

std::vector<glm::vec3> InnerClipMapGrid::CreateNormals(int xRes, int yRes) {
	validate(xRes, yRes);
	int resolution = xRes;
	std::vector<glm::vec3> normals;
	normals.reserve(numVertices(resolution));

	for (int y = -1; y < resolution + 2; y++) {
		for (int x = -1; x < resolution + 2; x++) {
			normals.push_back(glm::vec3(0, 0, 1));
		}
	}

	return normals;
}


}// namespace openspace