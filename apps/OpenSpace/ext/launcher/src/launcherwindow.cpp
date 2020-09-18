#include <openspace/scene/profile.h>
#include "launcherwindow.h"
#include "profileedit.h"
#include "./ui_launcherwindow.h"
#include <QPixmap>
#include <QKeyEvent>
#include "filesystemaccess.h"
#include <sstream>
#include <iostream>

LauncherWindow::LauncherWindow(std::string basePath, std::string profileName,
                               QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::LauncherWindow)
    , _fileAccess_profiles(".profile", {"./"}, true, false)
    , _fileAccess_winConfigs(".xml", {"./"}, true, false)
    , _filesystemAccess(".asset", {"scene", "global", "customization", "examples"},
                        true, true)
    , _basePath(QString::fromUtf8(basePath.c_str()))
{
    ui->setupUi(this);
    QString logoPath = _basePath + "/data/openspace-horiz-logo.png";
    QPixmap pix(logoPath);
    ui->logolabel->setPixmap(pix.scaled(400, 120, Qt::KeepAspectRatio));
    connect(ui->qBtn_start, SIGNAL(released()), this, SLOT(startOpenSpace()));
    connect(ui->newButton, SIGNAL(released()), this, SLOT(openWindow_new()));
    connect(ui->editButton, SIGNAL(released()), this, SLOT(openWindow_edit()));
    _reportAssetsInFilesystem = _filesystemAccess.useQtFileSystemModelToTraverseDir(
        QString(basePath.c_str()) + "/data/assets");
    populateProfilesList(QString(profileName.c_str()));
    populateWindowConfigsList();
}

void LauncherWindow::populateProfilesList(QString preset) {
    for (int i = 0; i < ui->comboBoxProfiles->count(); ++i) {
        ui->comboBoxProfiles->removeItem(i);
    }
    std::string reportProfiles = _fileAccess_profiles.useQtFileSystemModelToTraverseDir(
        _basePath + "/data/profiles");
    std::stringstream instream(reportProfiles);
    std::string iline;
    QStringList profilesListLine;
    while (std::getline(instream, iline)) {
        if (ui->comboBoxProfiles->findText(QString(iline.c_str())) == -1) {
            ui->comboBoxProfiles->addItem(iline.c_str());
        }
    }
    if (preset.length() > 0) {
        int presetMatchIdx = ui->comboBoxProfiles->findText(preset);
        if (presetMatchIdx != -1) {
            ui->comboBoxProfiles->setCurrentIndex(presetMatchIdx);
        }
    }
}

void LauncherWindow::populateWindowConfigsList() {
    std::string reportConfigs = _fileAccess_winConfigs.useQtFileSystemModelToTraverseDir(
        _basePath + "/config");
    std::stringstream instream(reportConfigs);
    std::string iline;
    QStringList windowConfigsListLine;
    while (std::getline(instream, iline)) {
        windowConfigsListLine << iline.c_str();
    }
    ui->comboBoxWindowConfigs->addItems(windowConfigsListLine);
}

void LauncherWindow::openWindow_new() {
    QString initialProfileSelection = ui->comboBoxProfiles->currentText();
    openspace::Profile* pData = new openspace::Profile();
    if (pData != nullptr) {
        myEditorWindow = new ProfileEdit(pData, _reportAssetsInFilesystem);
        myEditorWindow->exec();
        if (myEditorWindow->wasSaved()) {
            std::string saveProfilePath = _basePath.toUtf8().constData();
            saveProfilePath += "/data/profiles/";
            saveProfilePath += myEditorWindow->specifiedFilename() + ".profile";
            saveProfileToFile(saveProfilePath, pData);
            populateProfilesList(QString(myEditorWindow->specifiedFilename().c_str()));
        }
        else {
            populateProfilesList(initialProfileSelection);
        }
        delete pData;
    }
}

void LauncherWindow::openWindow_edit() {
    QString initialProfileSelection = ui->comboBoxProfiles->currentText();
    std::string profilePath = _basePath.toUtf8().constData();
    profilePath += "/data/profiles/";
    int selectedProfileIdx = ui->comboBoxProfiles->currentIndex();
    QString profileToSet = ui->comboBoxProfiles->itemText(selectedProfileIdx);
    std::string editProfilePath = profilePath + profileToSet.toUtf8().constData();
    editProfilePath += ".profile";
    openspace::Profile* pData;
    bool validFile = loadProfileFromFile(pData, editProfilePath);
    if (pData != nullptr && validFile) {
        myEditorWindow = new ProfileEdit(pData, _reportAssetsInFilesystem);
        myEditorWindow->setProfileName(profileToSet);
        myEditorWindow->exec();
        if (myEditorWindow->wasSaved()) {
            profilePath += myEditorWindow->specifiedFilename() + ".profile";
            saveProfileToFile(profilePath, pData);
            populateProfilesList(QString(myEditorWindow->specifiedFilename().c_str()));
        }
        else {
            populateProfilesList(initialProfileSelection);
        }
        delete pData;
    }
}

void LauncherWindow::saveProfileToFile(const std::string& path, openspace::Profile* p) {
    std::ofstream outFile;
    try {
        outFile.open(path, std::ofstream::out);
    }
    catch (const std::ofstream::failure& e) {
        displayErrorDialog(fmt::format(
            "Exception opening profile file {} for write: ({})",
            path,
            e.what()
        ));
    }

    try {
        outFile << p->serialize();
    }
    catch (const std::ofstream::failure& e) {
        displayErrorDialog(fmt::format(
            "Data write error to file: {} ({})",
            path,
            e.what()
        ));
    }

    outFile.close();
}

bool LauncherWindow::loadProfileFromFile(openspace::Profile*& p, std::string filename) {
    bool successfulLoad = true;
    std::vector<std::string> content;
    if (filename.length() > 0) {
        std::ifstream inFile;
        try {
            inFile.open(filename, std::ifstream::in);
        }
        catch (const std::ifstream::failure& e) {
            throw ghoul::RuntimeError(fmt::format(
                "Exception opening {} profile for read: ({})",
                filename,
                e.what()
            ));
        }
        std::string line;
        while (std::getline(inFile, line)) {
            content.push_back(std::move(line));
        }
    }
    try {
        p = new openspace::Profile(content);
    }
    catch (const ghoul::MissingCaseException& e) {
        displayErrorDialog(fmt::format(
            "Missing case exception in {}: {}",
            filename,
            e.what()
        ));
        successfulLoad = false;
    }
    catch (const openspace::Profile::ParsingError& e) {
        displayErrorDialog(fmt::format(
            "ParsingError exception in {}: {}, {}",
            filename,
            e.component,
            e.message
        ));
        successfulLoad = false;
    }
    catch (const ghoul::RuntimeError& e) {
        displayErrorDialog(fmt::format(
            "RuntimeError exception in {}, component {}: {}",
            filename,
            e.component,
            e.message
        ));
        successfulLoad = false;
    }
    return successfulLoad;
}

void LauncherWindow::displayErrorDialog(std::string msg) {
    //New instance of info dialog window
    _myDialog = new errordialog(QString(msg.c_str()));
    _myDialog->exec();
}

LauncherWindow::~LauncherWindow() {
    delete ui;
    delete myEditorWindow;
}

bool LauncherWindow::wasLaunchSelected() {
    return _launch;
}

std::string LauncherWindow::selectedProfile() {
    return ui->comboBoxProfiles->currentText().toUtf8().constData();
}

void LauncherWindow::startOpenSpace() {
    _launch = true;
    close();
}
