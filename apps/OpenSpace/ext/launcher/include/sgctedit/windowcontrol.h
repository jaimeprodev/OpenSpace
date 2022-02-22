/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2022                                                               *
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
#ifndef __OPENSPACE_UI_LAUNCHER___WINDOWCONTROL___H__
#define __OPENSPACE_UI_LAUNCHER___WINDOWCONTROL___H__

#include <QWidget>

#include <sgct/config.h>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <vector>

class WindowControl : public QWidget {
Q_OBJECT
public:
    /**
     * Constructor for WindowControl class, which contains settings and configuration
     * for individual windows
     *
     * \param monitorIndex The zero-based index for monitor number that this window
     *                     resides in
     * \param windowIndex The zero-based window index
     * \param monitorDims Vector of monitor dimensions in QRect form
     * \param winColor A  QColor object for this window's unique color
    */
    WindowControl(unsigned int monitorIndex, unsigned int windowIndex,
        std::vector<QRect>& monitorDims, const QColor& winColor,
        QWidget *parent);
    ~WindowControl();
    /**
     * Sets callback function to be invoked when a window's setting changes
     *
     * \param cb Callback function that accepts the listed arg types, in order of
     *           monitorIndex, windowIndex, and windowDims (that were just changed)
     */
    void setWindowChangeCallback(std::function<void(int, int, const QRectF&)> cb);
    /**
     * Sets callback function to be invoked when a window gets its GUI checkbox selected
     *
     * \param cb Callback function that accepts the index of the window that has its
     *           WebGUI option selected
     */
    void setWebGuiChangeCallback(std::function<void(unsigned int)> cb);
    void showWindowLabel(bool show);
    QVBoxLayout* initializeLayout();
    QRectF& dimensions();
    std::string windowName() const;
    sgct::ivec2 windowSize() const;
    sgct::ivec2 windowPos() const;
    bool isDecorated() const;
    bool isSpoutSelected() const;
    bool isGuiWindow() const;
    void uncheckWebGuiOption();
    int qualitySelectedValue() const;
    unsigned int monitorNum() const;
    float fovH() const;
    float fovV() const;
    float heightOffset() const;
    enum class ProjectionIndeces {
        Planar = 0,
        Fisheye,
        SphericalMirror,
        Cylindrical,
        Equirectangular
    };
    ProjectionIndeces projectionSelectedIndex() const;

private slots:
    void onSizeXChanged(const QString& newText);
    void onSizeYChanged(const QString& newText);
    void onOffsetXChanged(const QString& newText);
    void onOffsetYChanged(const QString& newText);
    void onMonitorChanged(int newSelection);
    void onProjectionChanged(int newSelection);
    void onFullscreenClicked();
    void onSpoutSelection(int selectionState);
    void onWebGuiSelection(int selectionState);

private:
    void createWidgets(QWidget* parent);
    void updateScaledWindowDimensions();
    std::function<void(int, int, const QRectF&)> _windowChangeCallback;
    std::function<void(unsigned int)> _windowGuiCheckCallback;
    QRectF defaultWindowSizes[4] = {
        {50.f, 50.f, 1280.f, 720.f},
        {900.f, 250.f, 1280.f, 720.f},
        {1200.f, 340.f, 1280.f, 720.f},
        {50.f, 50.f, 1280.f, 720.f}
    };
    QList<QString> _monitorNames = { "Primary", "Secondary" };
    int QualityValues[10] = { 256, 512, 1024, 1536, 2048, 4096, 8192, 16384,
    32768, 65536 };
    int _lineEditWidthFixed = 50;
    float _marginFractionOfWidgetSize = 0.025f;
    unsigned int _nMonitors = 1;
    unsigned int _monIndex = 0;
    unsigned int _index = 0;
    std::vector<QRect>& _monitorResolutions;
    int _maxWindowSizePixels = 10000;
    const QColor& _colorForWindow;
    QVBoxLayout* _layoutFullWindow = nullptr;
    QLabel* _labelWinNum = nullptr;
    QLineEdit* _sizeX = nullptr;
    QLineEdit* _sizeY = nullptr;
    QLineEdit* _offsetX = nullptr;
    QLineEdit* _offsetY = nullptr;
    QRectF _windowDims;
    QPushButton* _fullscreenButton = nullptr;
    QCheckBox* _checkBoxWindowDecor = nullptr;
    QCheckBox* _checkBoxWebGui = nullptr;
    QCheckBox* _checkBoxSpoutOutput = nullptr;
    QComboBox* _comboMonitorSelect = nullptr;
    QComboBox* _comboProjection = nullptr;
    QComboBox* _comboQuality = nullptr;
    QLabel* _labelQuality = nullptr;
    QLabel* _labelFovH = nullptr;
    QLineEdit* _lineFovH = nullptr;
    QLabel* _labelFovV = nullptr;
    QLineEdit* _lineFovV = nullptr;
    QLabel* _labelHeightOffset = nullptr;
    QLineEdit* _lineHeightOffset = nullptr;
    QLineEdit* _windowName = nullptr;
};

#endif // __OPENSPACE_UI_LAUNCHER___WINDOWCONTROL___H__
