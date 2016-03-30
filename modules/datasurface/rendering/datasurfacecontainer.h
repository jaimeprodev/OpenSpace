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

#ifndef __DATASURFACECONTAINER_H__
#define __DATASURFACECONTAINER_H__
#include <openspace/rendering/renderable.h>

namespace openspace{
class DataSurface;

class DataSurfaceContainer : public Renderable {
public:
	DataSurfaceContainer(const ghoul::Dictionary& dictionary);
	~DataSurfaceContainer();

	bool initialize() override;
	bool deinitialize() override;

	bool isReady() const override;

	virtual void render(const RenderData& data) override;
	virtual void update(const UpdateData& data) override;

	void addDataSurface(std::string path);
	void addTextureSurface(std::string path);

	std::shared_ptr<DataSurface> dataSurface(std::string name);

private:
std::vector<std::shared_ptr<DataSurface>> _dataSurfaces;

};	
}//namespace openspace

#endif