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

#include <modules/globebrowsing/tile/tileprovidermanager.h>

#include <ghoul/logging/logmanager.h>

#include "cpl_minixml.h"


namespace {
    const std::string _loggerCat = "TileProviderManager";
}


namespace openspace {

    ThreadPool TileProviderManager::tileRequestThreadPool(1);

    TileProviderManager::TileProviderManager(const ghoul::Dictionary& texDict){
        // Color Texture
        ghoul::Dictionary colorTexturesDict;
        texDict.getValue(
            LayeredTextures::TEXTURE_CATEGORY_NAMES[LayeredTextures::ColorTextures], 
            colorTexturesDict);

        TileProviderInitData colorInitData;
        colorInitData.minimumPixelSize = 1024;
        colorInitData.threads = 1;
        colorInitData.cacheSize = 500;
        colorInitData.framesUntilRequestQueueFlush = 60;
        colorInitData.preprocessTiles = false;

        initTexures(
            _layerCategories[LayeredTextures::ColorTextures],
            colorTexturesDict, colorInitData);

        // Night Texture
        ghoul::Dictionary nightTexturesDict;
        texDict.getValue(
            LayeredTextures::TEXTURE_CATEGORY_NAMES[LayeredTextures::NightTextures],
            nightTexturesDict);

        TileProviderInitData nightInitData;
        nightInitData.minimumPixelSize = 1024;
        nightInitData.threads = 1;
        nightInitData.cacheSize = 500;
        nightInitData.framesUntilRequestQueueFlush = 60;
        nightInitData.preprocessTiles = false;

        initTexures(
            _layerCategories[LayeredTextures::NightTextures],
            nightTexturesDict, nightInitData);

        // Overlays
        ghoul::Dictionary overlaysDict;
        texDict.getValue(
            LayeredTextures::TEXTURE_CATEGORY_NAMES[LayeredTextures::Overlays],
            overlaysDict);

        TileProviderInitData overlayInitData;
        overlayInitData.minimumPixelSize = 1024;
        overlayInitData.threads = 1;
        overlayInitData.cacheSize = 500;
        overlayInitData.framesUntilRequestQueueFlush = 60;
        overlayInitData.preprocessTiles = false;

        initTexures(
            _layerCategories[LayeredTextures::Overlays],
            overlaysDict,
            overlayInitData);

        // Height maps
        ghoul::Dictionary heightTexturesDict;
        texDict.getValue(
            LayeredTextures::TEXTURE_CATEGORY_NAMES[LayeredTextures::HeightMaps],
            heightTexturesDict);

        TileProviderInitData heightInitData;
        heightInitData.minimumPixelSize = 64;
        heightInitData.threads = 1;
        heightInitData.cacheSize = 500;
        heightInitData.framesUntilRequestQueueFlush = 60;
        heightInitData.preprocessTiles = true;

        initTexures(
            _layerCategories[LayeredTextures::HeightMaps],
            heightTexturesDict,
            heightInitData);

        // Water masks
        ghoul::Dictionary waterMaskDict;
        texDict.getValue(
            LayeredTextures::TEXTURE_CATEGORY_NAMES[LayeredTextures::WaterMasks],
            waterMaskDict);

        TileProviderInitData waterInitData;
        waterInitData.minimumPixelSize = 2048;
        waterInitData.threads = 1;
        waterInitData.cacheSize = 500;
        waterInitData.framesUntilRequestQueueFlush = 60;
        waterInitData.preprocessTiles = false;

        initTexures(
            _layerCategories[LayeredTextures::WaterMasks],
            waterMaskDict, waterInitData);
    }

    TileProviderManager::~TileProviderManager()
    {
    }

    void TileProviderManager::initTexures(std::vector<TileProviderWithName>& dest,
        const ghoul::Dictionary& texturesDict, const TileProviderInitData& initData)
    {
        // Create TileProviders for all textures within this category
        for (size_t i = 0; i < texturesDict.size(); i++) {
            std::string name, path;
            std::string dictKey = std::to_string(i + 1);
            ghoul::Dictionary texDict = texturesDict.value<ghoul::Dictionary>(dictKey);
            texDict.getValue("Name", name);
            texDict.getValue("FilePath", path);


            std::shared_ptr<TileProvider> tileProvider = initProvider(path, initData);

            bool enabled = dest.size() == 0; // Only enable first layer
            dest.push_back({ name, tileProvider, enabled });
        }
    }


    std::shared_ptr<TileProvider> TileProviderManager::initProvider(const std::string& file,
        const TileProviderInitData& initData)
    {
        std::shared_ptr<TileProvider> tileProvider;
        CPLXMLNode * node = CPLParseXMLFile(file.c_str());
        if (std::string(node->pszValue) == "OpenSpaceTemporalGDALDataset") {
            tileProvider = std::shared_ptr<TileProvider>(
                new TemporalTileProvider(file, initData));
            return tileProvider;
        }

        std::shared_ptr<TileDataset> tileDataset = std::shared_ptr<TileDataset>(
            new TileDataset(file, initData.minimumPixelSize, initData.preprocessTiles));

        std::shared_ptr<ThreadPool> threadPool = std::shared_ptr<ThreadPool>(
            new ThreadPool(1));

        std::shared_ptr<AsyncTileDataProvider> tileReader = std::shared_ptr<AsyncTileDataProvider>(
            new AsyncTileDataProvider(tileDataset, threadPool));

        std::shared_ptr<TileCache> tileCache = std::shared_ptr<TileCache>(new TileCache(initData.cacheSize));

        tileProvider = std::shared_ptr<TileProvider>(
            new CachingTileProvider(tileReader, tileCache, initData.framesUntilRequestQueueFlush));

        return tileProvider;
    }

    TileProviderManager::LayerCategory& TileProviderManager::getLayerCategory(LayeredTextures::TextureCategory category)
    {
        return _layerCategories[category];
    }

    void TileProviderManager::prerender() {
        for each (auto layerCategory in _layerCategories) {
            for each (auto tileProviderWithName in layerCategory) {
                if (tileProviderWithName.isActive) {
                    tileProviderWithName.tileProvider->prerender();
                }
            }
        }
    }

    const std::vector<std::shared_ptr<TileProvider> >
        TileProviderManager::getActivatedLayerCategory(LayeredTextures::TextureCategory category)
    {
        std::vector<std::shared_ptr<TileProvider> > tileProviders;
        for each (auto tileProviderWithName in _layerCategories[category]) {
            if (tileProviderWithName.isActive) {
                tileProviders.push_back(tileProviderWithName.tileProvider);
            }
        }
        return tileProviders;
    }

}  // namespace openspace
