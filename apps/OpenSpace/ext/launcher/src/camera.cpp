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

#include "camera.h"

#include <openspace/scene/profile.h>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QTabWidget> 

namespace {
    constexpr const int CameraTypeNav = 0;
    constexpr const int CameraTypeGeo = 1;

    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;

    bool inNumericalRange(QLineEdit* le, float min, float max) {
        QString s = le->text();
        bool validConversion = false;
        float value = s.toFloat(&validConversion);
        if (!validConversion) {
            return false;
        }
        if (value < min || value > max) {
            return false;
        }
        return true;
    }
} // namespace

Camera::Camera(openspace::Profile* profile, QWidget *parent)
    : QDialog(parent)
    , _profile(profile)
{
    setWindowTitle("Set Camera Position");

    QBoxLayout* layout = new QVBoxLayout(this);
    _tabWidget = new QTabWidget;
    connect(_tabWidget, &QTabWidget::tabBarClicked, this, &Camera::tabSelect);
    _tabWidget->addTab(createNavStateWidget(), "Navigation State");
    _tabWidget->addTab(createGeoWidget(), "Geo State");
    layout->addWidget(_tabWidget);

    {
        QFrame* line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);
    }

    {
        QWidget* footer = new QWidget;
        QBoxLayout* footerLayout = new QHBoxLayout(footer);

        _errorMsg = new QLabel;
        _errorMsg->setObjectName("error-message");
        _errorMsg->setWordWrap(true);
        footerLayout->addWidget(_errorMsg);

        QDialogButtonBox* buttons = new QDialogButtonBox;
        buttons->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        connect(buttons, &QDialogButtonBox::accepted, this, &Camera::approved);
        connect(buttons, &QDialogButtonBox::rejected, this, &Camera::reject);
        footerLayout->addWidget(buttons);

        layout->addWidget(footer);
    }

    if (_profile->camera().has_value()) {
        openspace::Profile::CameraType type = _profile->camera().value();
        std::visit(overloaded {
            [this](const openspace::Profile::CameraNavState& nav) {
                _tabWidget->setCurrentIndex(CameraTypeNav);
                _navState.anchor->setText(QString::fromStdString(nav.anchor));
                _navState.aim->setText(QString::fromStdString(*nav.aim));
                _navState.refFrame->setText(QString::fromStdString(nav.referenceFrame));
                _navState.positionX->setText(QString::number(nav.position.x));
                _navState.positionY->setText(QString::number(nav.position.y));
                _navState.positionZ->setText(QString::number(nav.position.z));
                if (nav.up.has_value()) {
                    _navState.upX->setText(QString::number(nav.up.value().x));
                    _navState.upY->setText(QString::number(nav.up.value().y));
                    _navState.upZ->setText(QString::number(nav.up.value().z));
                }
                else {
                    _navState.upX->clear();
                    _navState.upY->clear();
                    _navState.upZ->clear();
                }
                if (nav.yaw.has_value()) {
                    _navState.yaw->setText(QString::number(*nav.yaw));
                }
                else {
                    _navState.yaw->clear();
                }
                if (nav.pitch.has_value()) {
                    _navState.pitch->setText(QString::number(*nav.pitch));
                }
                else {
                    _navState.pitch->clear();
                }
                tabSelect(CameraTypeNav);
            },
            [this](const openspace::Profile::CameraGoToGeo& geo) {
                _tabWidget->setCurrentIndex(CameraTypeGeo);
                _geoState.anchor->setText(QString::fromStdString(geo.anchor));
                _geoState.latitude->setText(QString::number(geo.latitude));
                _geoState.longitude->setText(QString::number(geo.longitude));
                if (geo.altitude.has_value()) {
                    _geoState.altitude->setText(QString::number(*geo.altitude));
                }
                else {
                    _geoState.altitude->clear();
                }
                tabSelect(CameraTypeGeo);
            }
        }, type);
    }
    else {
        _tabWidget->setCurrentIndex(CameraTypeNav);
        _navState.anchor->clear();
        _navState.aim->clear();
        _navState.refFrame->clear();
        _navState.positionX->clear();
        _navState.positionY->clear();
        _navState.positionZ->clear();
        _navState.upX->clear();
        _navState.upY->clear();
        _navState.upZ->clear();
        _navState.yaw->clear();
        _navState.pitch->clear();

        _geoState.anchor->clear();
        _geoState.latitude->clear();
        _geoState.longitude->clear();
        _geoState.altitude->clear();
    }
}

QWidget* Camera::createNavStateWidget() {
    QWidget* box = new QWidget;
    QGridLayout* layout = new QGridLayout(box);

    layout->addWidget(new QLabel("Anchor:"), 0, 0);
    _navState.anchor = new QLineEdit;
    _navState.anchor->setToolTip("Anchor camera to this node");
    layout->addWidget(_navState.anchor, 0, 1);

    layout->addWidget(new QLabel("Aim:"), 1, 0);
    _navState.aim = new QLineEdit;
    _navState.aim->setToolTip(
        "[OPTIONAL] If specified, camera will be aimed at this node while keeping the "
        "anchor node in the same view location"
    );
    layout->addWidget(_navState.aim, 1, 1);

    layout->addWidget(new QLabel("Reference Frame:"), 2, 0);
    _navState.refFrame = new QLineEdit;
    _navState.refFrame->setToolTip("[OPTIONAL] Camera location in reference to this frame");
    layout->addWidget(_navState.refFrame, 2, 1);

    layout->addWidget(new QLabel("Position:"), 3, 0);
    {
        QWidget* posBox = new QWidget;
        QBoxLayout* posLayout = new QHBoxLayout(posBox);
        posLayout->setContentsMargins(0, 0, 0, 0);
        posLayout->addWidget(new QLabel("X"));
        _navState.positionX = new QLineEdit;
        _navState.positionX->setValidator(new QDoubleValidator);
        _navState.positionX->setToolTip("Camera position vector (x)");
        posLayout->addWidget(_navState.positionX);

        posLayout->addWidget(new QLabel("Y"));
        _navState.positionY = new QLineEdit;
        _navState.positionY->setValidator(new QDoubleValidator);
        _navState.positionY->setToolTip("Camera position vector (y)");
        posLayout->addWidget(_navState.positionY);

        posLayout->addWidget(new QLabel("Z"));
        _navState.positionZ = new QLineEdit;
        _navState.positionZ->setValidator(new QDoubleValidator);
        _navState.positionZ->setToolTip("Camera position vector (z)");
        posLayout->addWidget(_navState.positionZ);
        layout->addWidget(posBox, 3, 1);
    }

    layout->addWidget(new QLabel("Up:"), 4, 0);
    {
        QWidget* upBox = new QWidget;
        QBoxLayout* upLayout = new QHBoxLayout(upBox);
        upLayout->setContentsMargins(0, 0, 0, 0);
        upLayout->addWidget(new QLabel("X"));
        _navState.upX = new QLineEdit;
        _navState.upX->setValidator(new QDoubleValidator);
        _navState.upX->setToolTip("[OPTIONAL] Camera up vector (x)");
        upLayout->addWidget(_navState.upX);

        upLayout->addWidget(new QLabel("Y"));
        _navState.upY = new QLineEdit;
        _navState.upY->setValidator(new QDoubleValidator);
        _navState.upY->setToolTip("[OPTIONAL] Camera up vector (y)");
        upLayout->addWidget(_navState.upY);

        upLayout->addWidget(new QLabel("Z"));
        _navState.upZ = new QLineEdit;
        _navState.upZ->setValidator(new QDoubleValidator);
        _navState.upZ->setToolTip("[OPTIONAL] Camera up vector (z)");
        upLayout->addWidget(_navState.upZ);
        layout->addWidget(upBox, 4, 1);
    }

    layout->addWidget(new QLabel("Yaw angle:"), 5, 0);
    _navState.yaw = new QLineEdit;
    _navState.yaw->setValidator(new QDoubleValidator);
    _navState.yaw->setToolTip("[OPTIONAL] yaw angle +/- 360 degrees");
    layout->addWidget(_navState.yaw, 5, 1);

    layout->addWidget(new QLabel("Pitch angle:"), 6, 0);
    _navState.pitch = new QLineEdit;
    _navState.pitch->setValidator(new QDoubleValidator);
    _navState.pitch->setToolTip("[OPTIONAL] pitch angle +/- 360 degrees");
    layout->addWidget(_navState.pitch, 6, 1);

    return box;
}

QWidget* Camera::createGeoWidget() {
    QWidget* box = new QWidget;
    QGridLayout* layout = new QGridLayout(box);

    layout->addWidget(new QLabel("Anchor:"), 0, 0);
    _geoState.anchor = new QLineEdit;
    _geoState.anchor->setToolTip("Anchor camera to this globe (planet/moon)");
    layout->addWidget(_geoState.anchor, 0, 1);

    layout->addWidget(new QLabel("Latitude"), 1, 0);
    _geoState.latitude = new QLineEdit;
    _geoState.latitude->setValidator(new QDoubleValidator);
    _geoState.latitude->setToolTip("Latitude of camera focus point (+/- 90 degrees)");
    layout->addWidget(_geoState.latitude, 1, 1);

    layout->addWidget(new QLabel("Longitude"), 2, 0);
    _geoState.longitude = new QLineEdit;
    _geoState.longitude->setValidator(new QDoubleValidator);
    _geoState.longitude->setToolTip("Longitude of camera focus point (+/- 180 degrees)");
    layout->addWidget(_geoState.longitude, 2, 1);

    layout->addWidget(new QLabel("Altitude"), 3, 0);
    _geoState.altitude = new QLineEdit;
    _geoState.altitude->setValidator(new QDoubleValidator);
    _geoState.altitude->setToolTip("[OPTIONAL] Altitude of camera (meters)");
    //altitude->setPlaceholderText("optional");
    layout->addWidget(_geoState.altitude, 3, 1);

    return box;
}

bool Camera::areRequiredFormsFilledAndValid() {
    bool allFormsOk = true;
    _errorMsg->clear();

    if (_tabWidget->currentIndex() == CameraTypeNav) {
        if (_navState.anchor->text().isEmpty()) {
            allFormsOk = false;
            addErrorMsg("Anchor is empty");
        }
        if (_navState.positionX->text().isEmpty()) {
            allFormsOk = false;
            addErrorMsg("Position X is empty");
        }
        if (_navState.positionY->text().isEmpty()) {
            allFormsOk = false;
            addErrorMsg("Position Y is empty");
        }
        if (_navState.positionZ->text().isEmpty()) {
            allFormsOk = false;
            addErrorMsg("Position Z is empty");
        }
        int upVectorCount = 0;
        if (_navState.upX->text().isEmpty()) {
            allFormsOk = false;
            addErrorMsg("Up X is empty");
        }
        if (_navState.upY->text().isEmpty()) {
            allFormsOk = false;
            addErrorMsg("Up Y is empty");
        }
        if (_navState.upZ->text().isEmpty()) {
            allFormsOk = false;
            addErrorMsg("Up Z is empty");
        }
        if (!_navState.yaw->text().isEmpty()) {
            if (!inNumericalRange(_navState.yaw, -360.0, 360.0)) {
                allFormsOk = false;
                addErrorMsg("Yaw value is not in +/- 360.0 range");
            }
        }
        if (!_navState.pitch->text().isEmpty()) {
            if (!inNumericalRange(_navState.pitch, -360.0, 360.0)) {
                allFormsOk = false;
                addErrorMsg("Pitch value is not in +/- 360.0 range");
            }
        }
    }

    if (_tabWidget->currentIndex() == CameraTypeGeo) {
        if (_geoState.anchor->text().isEmpty()) {
            allFormsOk = false;
            addErrorMsg("Anchor is empty");
        }
        if (!inNumericalRange(_geoState.latitude, -90.0, 90.0)) {
            allFormsOk = false;
            addErrorMsg("Latitude value is not in +/- 90.0 range");
        }
        if (!inNumericalRange(_geoState.longitude, -180.0, 180.0)) {
            allFormsOk = false;
            addErrorMsg("Longitude value is not in +/- 180.0 range");
        }
    }
    return allFormsOk;
}

void Camera::addErrorMsg(QString errorDescription) {
    QString contents = _errorMsg->text();
    if (contents.length() > 0) {
        contents += ", ";
    }
    contents += errorDescription;
    _errorMsg->setText(contents);
}

void Camera::approved() {
    if (!areRequiredFormsFilledAndValid()) {
        return;
    }

    if (_tabWidget->currentIndex() == CameraTypeNav) {
        openspace::Profile::CameraNavState nav;
        nav.anchor = _navState.anchor->text().toUtf8().constData();
        nav.aim = _navState.aim->text().toUtf8().constData();
        nav.referenceFrame = _navState.refFrame->text().toUtf8().constData();
        nav.position.x = _navState.positionX->text().toDouble();
        nav.position.y = _navState.positionY->text().toDouble();
        nav.position.z = _navState.positionZ->text().toDouble();
        if (!_navState.upX->text().isEmpty() &&
            !_navState.upY->text().isEmpty() &&
            !_navState.upZ->text().isEmpty())
        {
            glm::dvec3 u = {
                _navState.upX->text().toDouble(),
                _navState.upY->text().toDouble(),
                _navState.upZ->text().toDouble()
            };
            nav.up = u;
        }
        else {
            nav.up = std::nullopt;
        }
        if (!_navState.yaw->text().isEmpty()) {
            nav.yaw = _navState.yaw->text().toDouble();
        }
        else {
            nav.yaw = std::nullopt;
        }
        if (!_navState.pitch->text().isEmpty()) {
            nav.pitch = _navState.pitch->text().toDouble();
        }
        else {
            nav.pitch = std::nullopt;
        }
        _profile->setCamera(nav);
    }
    else if (_tabWidget->currentIndex() == CameraTypeGeo) {
        openspace::Profile::CameraGoToGeo geo;
        geo.anchor = _geoState.anchor->text().toUtf8().constData();
        geo.latitude = _geoState.latitude->text().toDouble();
        geo.longitude = _geoState.longitude->text().toDouble();
        if (!_geoState.altitude->text().isEmpty()) {
            geo.altitude = _geoState.altitude->text().toDouble();
        }
        _profile->setCamera(geo);
    }

    accept();
}

void Camera::tabSelect(int tabIndex) {
    _errorMsg->clear();

    if (tabIndex == 0) {
        _navState.anchor->setFocus(Qt::OtherFocusReason);
    }
    else if (tabIndex == 1) {
        _geoState.anchor->setFocus(Qt::OtherFocusReason);
    }
    else {
        throw std::logic_error("Unknown tab index");
    }
}
