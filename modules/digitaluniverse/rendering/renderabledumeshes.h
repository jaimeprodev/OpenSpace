/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2017                                                               *
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

#ifndef __OPENSPACE_MODULE_DIGITALUNIVERSE___RENDERABLEDUMESHES___H__
#define __OPENSPACE_MODULE_DIGITALUNIVERSE___RENDERABLEDUMESHES___H__

#include <openspace/rendering/renderable.h>

#include <openspace/properties/optionproperty.h>
#include <openspace/properties/stringproperty.h>
#include <openspace/properties/scalar/boolproperty.h>
#include <openspace/properties/scalar/floatproperty.h>
#include <openspace/properties/vector/vec3property.h>

#include <ghoul/opengl/ghoul_gl.h>

namespace ghoul::filesystem { 
    class File; 
}

namespace ghoul::opengl {
    class ProgramObject;
    class Texture;
} // namespace ghoul::opengl

namespace openspace {

    namespace documentation { struct Documentation; }

    class RenderableDUMeshes : public Renderable {
    public:
        explicit RenderableDUMeshes(const ghoul::Dictionary& dictionary);
        ~RenderableDUMeshes() = default;

        void initialize() override;
        void deinitialize() override;

        bool isReady() const override;

        void render(const RenderData& data, RendererTasks& rendererTask) override;
        void update(const UpdateData& data) override;

        static documentation::Documentation Documentation();

    private:
        
        enum Unit {
            Meter = 0,
            Kilometer = 1,
            Parsec = 2,
            Kiloparsec = 3,
            Megaparsec = 4,
            Gigaparsec = 5,

            GigalightYears = 6
        };

        void createDataSlice();

        bool loadData();
        bool readSpeckFile();
        bool loadCachedFile(const std::string& file);
        bool saveCachedFile(const std::string& file) const;

        bool _dataIsDirty;

        properties::FloatProperty _alphaValue;
        properties::FloatProperty _scaleFactor;
        properties::Vec3Property _pointColor;
        

        std::unique_ptr<ghoul::opengl::ProgramObject> _program;

        std::string _speckFile;

        Unit _unit;

        std::vector<double> _slicedData;
        std::vector<float> _fullData;
        int _nValuesPerAstronomicalObject;

        GLuint _vao;
        GLuint _vbo;
    };


} // namespace openspace

#endif // __OPENSPACE_MODULE_DIGITALUNIVERSE___RENDERABLEDUMESHES___H__
