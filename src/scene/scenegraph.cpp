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

#include <openspace/scene/scenegraph.h>

#include <openspace/engine/openspaceengine.h>
#include <openspace/scene/scenegraphnode.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/interaction/interactionhandler.h>

#include <ghoul/filesystem/filesystem.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/lua/lua_helper.h>

#include <stack>
#include <unordered_map>

#ifdef _MSC_VER
#ifdef OPENSPACE_ENABLE_VLD
#include <vld.h>
#endif
#endif

namespace {
    const std::string _loggerCat = "SceneGraph";
    const std::string _moduleExtension = ".mod";
    const std::string _defaultCommonDirectory = "common";
    const std::string _commonModuleToken = "${COMMON_MODULE}";

    const std::string KeyPathScene = "ScenePath";
    const std::string KeyModules = "Modules";
    const std::string KeyCommonFolder = "CommonFolder";
    const std::string KeyPathModule = "ModulePath";
}

namespace openspace {

SceneGraph::SceneGraphNodeInternal::~SceneGraphNodeInternal() {
    delete node;
}

SceneGraph::SceneGraph()
    : _rootNode(nullptr)
{}

SceneGraph::~SceneGraph() {
    clear();
}

void SceneGraph::clear() {
    // Untested ---abock
    for (SceneGraphNodeInternal* n : _nodes)
        delete n;

    _nodes.clear();
    _rootNode = nullptr;
}

bool SceneGraph::loadFromFile(const std::string& sceneDescription) {
    clear(); // Move this to a later stage to retain a proper scenegraph when the loading fails ---abock

    std::string absSceneFile = absPath(sceneDescription);

    // See if scene file exists
    using RawPath = ghoul::filesystem::FileSystem::RawPath;
    if (!FileSys.fileExists(absSceneFile, RawPath::Yes)) {
        LERROR("Could not load scene file '" << absSceneFile << "'. " <<
            "File not found");
        return false;
    }
    LINFO("Loading SceneGraph from file '" << absSceneFile << "'");

    // Load dictionary
    ghoul::Dictionary sceneDictionary;
    try {
        ghoul::lua::loadDictionaryFromFile(absSceneFile, sceneDictionary);
    }
    catch (...) {
        return false;
    }

    std::string sceneDescriptionDirectory = ghoul::filesystem::File(
        absSceneFile,
        ghoul::filesystem::File::RawPath::Yes
    ).directoryName();
    std::string sceneDirectory(".");
    sceneDictionary.getValue(KeyPathScene, sceneDirectory);

    // The scene path could either be an absolute or relative path to the description
    // paths directory
    std::string relativeCandidate = sceneDescriptionDirectory +
        ghoul::filesystem::FileSystem::PathSeparator + sceneDirectory;
    std::string absoluteCandidate = absPath(sceneDirectory);

    if (FileSys.directoryExists(relativeCandidate))
        sceneDirectory = relativeCandidate;
    else if (FileSys.directoryExists(absoluteCandidate))
        sceneDirectory = absoluteCandidate;
    else {
        LERROR("The '" << KeyPathScene << "' pointed to a "
            "path '" << sceneDirectory << "' that did not exist");
        return false;
    }

    ghoul::Dictionary moduleDictionary;
    bool success = sceneDictionary.getValue(KeyModules, moduleDictionary);
    if (!success)
        // There are no modules that are loaded
        return true;

    lua_State* state = ghoul::lua::createNewLuaState();
    OsEng.scriptEngine().initializeLuaState(state);

    // Get the common directory
    bool commonFolderSpecified = sceneDictionary.hasKey(KeyCommonFolder);
    bool commonFolderCorrectType = sceneDictionary.hasKeyAndValue<std::string>(KeyCommonFolder);

    if (commonFolderSpecified) {
        if (commonFolderCorrectType) {
            std::string commonFolder = sceneDictionary.value<std::string>(KeyCommonFolder);
            std::string fullCommonFolder = FileSys.pathByAppendingComponent(
                sceneDirectory,
                commonFolder
            );
            if (!FileSys.directoryExists(fullCommonFolder))
                LERROR("Specified common folder '" << fullCommonFolder << "' did not exist");
            else {
                if (!commonFolder.empty()) {
                    FileSys.registerPathToken(_commonModuleToken, commonFolder);
                    size_t nKeys = moduleDictionary.size();
                    moduleDictionary.setValue(std::to_string(nKeys + 1), commonFolder);
                }
            }
        }
        else
            LERROR("Specification for 'common' folder has invalid type");
    }

    std::vector<std::string> keys = moduleDictionary.keys();

    std::map<std::string, std::vector<std::string>> dependencies;
    std::map<std::string, std::string> parents;

    _rootNode = new SceneGraphNode;
    _rootNode->setName(SceneGraphNode::RootNodeName);
    SceneGraphNodeInternal* internalRoot = new SceneGraphNodeInternal;
    internalRoot->node = _rootNode;
    _nodes.push_back(internalRoot);

    std::sort(keys.begin(), keys.end());
    ghoul::filesystem::Directory oldDirectory = FileSys.currentDirectory();
    for (const std::string& key : keys) {
        std::string moduleName = moduleDictionary.value<std::string>(key);
        std::string modulePath = FileSys.pathByAppendingComponent(sceneDirectory, moduleName);

        if (!FileSys.directoryExists(modulePath)) {
            LERROR("Could not load module '" << moduleName << "'. Directory did not exist");
            continue;
        }

        std::string moduleFile = FileSys.pathByAppendingComponent(
            modulePath,
            moduleName + _moduleExtension
        );

        if (!FileSys.fileExists(moduleFile)) {
            LERROR("Could not load module file '" << moduleFile << "'. File did not exist");
            continue;
        }

        ghoul::Dictionary moduleDictionary;
        try {
            ghoul::lua::loadDictionaryFromFile(moduleFile, moduleDictionary, state);
        }
        catch (...) {
            continue;
        }

        std::vector<std::string> keys = moduleDictionary.keys();
        for (const std::string& key : keys) {
            if (!moduleDictionary.hasValue<ghoul::Dictionary>(key)) {
                LERROR("SceneGraphNode '" << key << "' is not a table in module '"
                                             << moduleFile << "'");
                continue;
            }

            ghoul::Dictionary element;
            std::string nodeName;
            std::string parentName;

            moduleDictionary.getValue(key, element);
            element.setValue(KeyPathModule, modulePath);

            element.getValue(SceneGraphNode::KeyName, nodeName);
            element.getValue(SceneGraphNode::KeyParentName, parentName);

            FileSys.setCurrentDirectory(modulePath);
            SceneGraphNode* node = SceneGraphNode::createFromDictionary(element);
            if (node == nullptr) {
                LERROR("Error loading SceneGraphNode '" << nodeName << "' in module '" << moduleName << "'");
                continue;
                //clear();
                //return false;
            }

            dependencies[nodeName].push_back(parentName);
            parents[nodeName] = parentName;
            // Also include loaded dependencies

            if (element.hasKey(SceneGraphNode::KeyDependencies)) {
                if (element.hasValue<ghoul::Dictionary>(SceneGraphNode::KeyDependencies)) {
                    ghoul::Dictionary nodeDependencies;
                    element.getValue(SceneGraphNode::KeyDependencies, nodeDependencies);

                    std::vector<std::string> keys = nodeDependencies.keys();
                    for (const std::string& key : keys) {
                        std::string value = nodeDependencies.value<std::string>(key);
                        dependencies[nodeName].push_back(value);
                    }
                }
                else {
                    LERROR("Dependencies did not have the corrent type");
                }
            }


            SceneGraphNodeInternal* internalNode = new SceneGraphNodeInternal;
            internalNode->node = node;
            _nodes.push_back(internalNode);
        }
    }
    ghoul::lua::destroyLuaState(state);
    FileSys.setCurrentDirectory(oldDirectory);

    for (SceneGraphNodeInternal* node : _nodes) {
        if (node->node == _rootNode)
            continue;
        std::string parent = parents[node->node->name()];
        SceneGraphNode* parentNode = sceneGraphNode(parent);
        if (parentNode == nullptr) {
            LERROR("Could not find parent '" << parent << "' for '" << node->node->name() << "'");
        }

        node->node->setParent(parentNode);
        parentNode->addChild(node->node);
        
    }

    // Setup dependencies
    for (SceneGraphNodeInternal* node : _nodes) {
        std::vector<std::string> nodeDependencies = dependencies[node->node->name()];

        for (const std::string& dep : nodeDependencies) {
            SceneGraphNodeInternal* n = nodeByName(dep);
            if (n == nullptr) {
                LERROR("Dependent node '" << dep << "' was not loaded for '" <<node->node->name() << "'");
                continue;
            }
            node->outgoingEdges.push_back(n);
            n->incomingEdges.push_back(node);
        }
    }

    std::vector<SceneGraphNodeInternal*> nodesToDelete;
    for (SceneGraphNodeInternal* node : _nodes) {
        if (!nodeIsDependentOnRoot(node)) {
            LERROR("Node '" << node->node->name() << "' has no direct connection to Root.");
            nodesToDelete.push_back(node);
        }
    }

    for (SceneGraphNodeInternal* node : nodesToDelete) {
        _nodes.erase(std::find(_nodes.begin(), _nodes.end(), node));
        delete node;
    }

    bool s = sortTopologically();
    if (!s) {
        LERROR("Topological sort failed");
        return false;
    }
    
    return true;
}

bool SceneGraph::nodeIsDependentOnRoot(SceneGraphNodeInternal* node) {
    if (node->node->name() == SceneGraphNode::RootNodeName)
        return true;
    else {
        for (SceneGraphNodeInternal* n : node->outgoingEdges) {
            bool dep = nodeIsDependentOnRoot(n);
            if (dep)
                return true;
        }
        return false;
    }
}

bool SceneGraph::sortTopologically() {
    if (_nodes.empty())
        return true;

    // Only the Root node can have an in-degree of 0
    SceneGraphNodeInternal* root = nodeByName(SceneGraphNode::RootNodeName);
    ghoul_assert(root != nullptr, "Could not find Root node");

    std::stack<SceneGraphNodeInternal*> zeroInDegreeNodes;
    zeroInDegreeNodes.push(root);

    std::unordered_map<SceneGraphNodeInternal*, size_t> inDegrees;
    for (SceneGraphNodeInternal* node : _nodes)
        inDegrees[node] = node->outgoingEdges.size();
        //inDegrees[node] = node->incomingEdges.size();
    
    _topologicalSortedNodes.clear();
    _topologicalSortedNodes.reserve(_nodes.size());
    while (!zeroInDegreeNodes.empty()) {
        SceneGraphNodeInternal* node = zeroInDegreeNodes.top();

        _topologicalSortedNodes.push_back(node->node);
        zeroInDegreeNodes.pop();

        //for (SceneGraphNodeInternal* n : node->outgoingEdges) {
        for (SceneGraphNodeInternal* n : node->incomingEdges) {
            inDegrees[n] -= 1;
            if (inDegrees[n] == 0)
                zeroInDegreeNodes.push(n);
        }

    }
    
    return true;
}

bool SceneGraph::addSceneGraphNode(SceneGraphNode* node) {
    // @TODO rework this ---abock
    ghoul_assert(node, "Node must not be nullptr");

    SceneGraphNodeInternal* internalNode = new SceneGraphNodeInternal;
    internalNode->node = node;

    auto it = std::find_if(
        _nodes.begin(),
        _nodes.end(),
        [node](SceneGraphNodeInternal* i) {
            return i->node == node->parent();
        }
    );

    if (it == _nodes.end()) {
        LERROR("Parent node was not found");
        delete internalNode;
        return false;
    }

    (*it)->incomingEdges.push_back(internalNode);
    internalNode->outgoingEdges.push_back(*it);

    _nodes.push_back(internalNode);
    sortTopologically();

    return true;
}

bool SceneGraph::removeSceneGraphNode(SceneGraphNode* node) {
    // @TODO rework this ---abock
    ghoul_assert(node, "Node must not be nullptr");

    auto it = std::find_if(
        _nodes.begin(),
        _nodes.end(),
        [node](SceneGraphNodeInternal* i) {
            return i->node == node;
        }
    );

    if (it == _nodes.end()) {
        LERROR("The node '" << node->name() << "' did not exist in the scenegraph");
        return false;
    }

    // Remove internal node from the list of nodes
    SceneGraphNodeInternal* internalNode = *it;
    _nodes.erase(it);

    if (OsEng.interactionHandler().focusNode() == node)
        OsEng.interactionHandler().setFocusNode(node->parent());

    sortTopologically();

#if 0
    SceneGraphNodeInternal* parentInternalNode = nodeByName(node->parent()->name());
    ghoul_assert(parentInternalNode, "Could not find internal parent node");

    // Reparent its children to its parent
    for (SceneGraphNode* c : node->children())
        c->setParent(node->parent());

    // Reset the dependencies accordingly
    // VERY untested ---abock
    for (SceneGraphNodeInternal* c : internalNode->incomingEdges) {
        parentInternalNode->outgoingEdges.insert(parentInternalNode->outgoingEdges.end(), c->outgoingEdges.begin(), c->outgoingEdges.end());
        parentInternalNode->incomingEdges.insert(parentInternalNode->incomingEdges.end(), c->incomingEdges.begin(), c->incomingEdges.end());
    }

#endif

    return true;
}

SceneGraph::SceneGraphNodeInternal* SceneGraph::nodeByName(const std::string& name) {
    auto it = std::find_if(
        _nodes.begin(),
        _nodes.end(),
        [name](SceneGraphNodeInternal* node) {
            return node->node->name() == name;
        }
    );

    if (it == _nodes.end())
        return nullptr;
    else
        return *it;
}

const std::vector<SceneGraphNode*>& SceneGraph::nodes() const {
    return _topologicalSortedNodes;
}

SceneGraphNode* SceneGraph::rootNode() const {
    return _rootNode;
}

SceneGraphNode* SceneGraph::sceneGraphNode(const std::string& name) const {
    auto it = std::find_if(
        _nodes.begin(),
        _nodes.end(),
        [name](SceneGraphNodeInternal* node) {
            return node->node->name() == name;
        }
    );
    if (it != _nodes.end())
        return (*it)->node;
    else
        return nullptr;
}

} // namespace openspace
