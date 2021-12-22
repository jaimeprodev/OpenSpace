/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2021                                                               *
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

#ifndef __OPENSPACE_UI_LAUNCHER___ASSETS___H__
#define __OPENSPACE_UI_LAUNCHER___ASSETS___H__

#include <QDialog>

#include "assettreemodel.h"

class QTextEdit;
class QTreeView;

class AssetsDialog final : public QDialog {
Q_OBJECT
public:
    /**
     * Constructor for assets class
     *
     * \param profile The #openspace::Profile object containing all data of the
     *                new or imported profile.
     * \param assetBasePath The path to the folder in which all of the assets are living
     * \param userAssetBasePath The path to the folder in which the users' assets are
     *        living
     * \param parent Pointer to parent Qt widget
     */
    AssetsDialog(QWidget* parent, openspace::Profile* profile,
        const std::string& assetBasePath, const std::string& userAssetBasePath);

private slots:
    void parseSelections();
    void selected(const QModelIndex&);

private:
    void createWidgets();
    /**
     * Creates a text summary of all assets and their paths
     *
     * \return the #std::string summary
     */
    QString createTextSummary();
    void openAssetEditor(const std::string& asset);

    openspace::Profile* _profile = nullptr;
    AssetTreeModel _assetTreeModel;
    QTreeView* _assetTree = nullptr;
    QTextEdit* _summary = nullptr;
};

#endif // __OPENSPACE_UI_LAUNCHER___ASSETS___H__
