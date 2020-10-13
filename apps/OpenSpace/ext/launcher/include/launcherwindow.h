/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2020                                                               *
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

#ifndef __OPENSPACE_UI_LAUNCHER___LAUNCHERWINDOW___H__
#define __OPENSPACE_UI_LAUNCHER___LAUNCHERWINDOW___H__

#include <QMainWindow>

#include <QString>
#include "profile/profileedit.h"
#include "filesystemaccess.h"
#include <openspace/scene/profile.h>
#include <openspace/engine/globals.h>
#include <optional>

class LauncherWindow : public QMainWindow {
Q_OBJECT
public:
    /**
     * Constructor for LauncherWindow class
     *
     * \param basePath The base OpenSpace installation path
     * \param profileEnabled true if profile selection combo box will be enabled
     * \param globalConfig reference to global configuration for OpenSpace settings
     * \param sgctConfigEnabled true if window selection combo box will be enabled
     * \param sgctConfigName the name of the sgct configuration function used to
     *                       generate window config (blank if file is used)
     * \param parentItem The parent that contains this (and possibly other) children
     *                   in the tree structure.
     */
    LauncherWindow(std::string basePath, bool profileEnabled,
        openspace::configuration::Configuration& globalConfig, bool sgctConfigEnabled,
        std::string sgctConfigName, QWidget* parent);

    /**
      * Returns bool for whether "start OpenSpace" was chosen when this window closed.
      * OpenSpace will not start if false
      *
      * \return true if the "start OpenSpace" button was clicked
      */
    bool wasLaunchSelected() const;

    /**
      * Returns true if both the profile and sgct window configuration were specified
      * at the command line (and so the launcher will not run).
      *
      * \return true if both profile and sgct window config were specified at CLI
      */
    bool isFullyConfiguredFromCliArgs() const;

    /**
      * Returns the selected profile name when launcher window closed
      *
      * \return name of selected profile (this is only the name without file extension
      *         and without path)
      */
    std::string selectedProfile() const;

    /**
      * Returns the selected sgct window configuration when launcher window closed
      *
      * \return name of selected profile (this is only the name without file extension
      *         and without path)
      */
    std::string selectedWindowConfig() const;

public slots:
    void openWindowEdit();
    void openWindowNew();
    void startOpenSpace();

private:
    void populateProfilesList(std::string preset);
    void populateWindowConfigsList(std::string preset);
    std::optional<openspace::Profile> loadProfileFromFile(std::string filename);
    void saveProfileToFile(const std::string& path, const openspace::Profile& p);

    FileSystemAccess _fileAccessProfiles;
    FileSystemAccess _fileAccessWinConfigs;
    FileSystemAccess _filesystemAccess;
    std::string _reportAssetsInFilesystem;
    openspace::configuration::Configuration& _globalConfig;
    QString _basePath;
    bool _launch = false;
    bool _fullyConfiguredViaCliArgs = false;
    bool _profileChangeAllowed = true;
    bool _sgctConfigChangeAllowed = true;

    QComboBox* _profileBox = nullptr;
    QComboBox* _windowConfigBox = nullptr;
};
#endif // __OPENSPACE_UI_LAUNCHER___LAUNCHERWINDOW___H__
