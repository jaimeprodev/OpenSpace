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

#include "deltatimes.h"

#include <openspace/scene/profile.h>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <array>
#include <iostream>

namespace {
    constexpr const int MaxNumberOfKeys = 30;

    struct TimeInterval {
        int index;
        double secondsPerInterval;
        QString intervalName;
    };

    std::array<TimeInterval, 8> TimeIntervals = {
        TimeInterval{ 0, 31536000.0, "year" },
        TimeInterval{ 1, 18144000.0, "month" },
        TimeInterval{ 2, 604800.0, "week" },
        TimeInterval{ 3, 86400.0, "day" },
        TimeInterval{ 4, 3600.0, "hour" },
        TimeInterval{ 5, 60.0, "minute" },
        TimeInterval{ 6, 1.0, "second" },
    };

} // namespace

DeltaTimes::DeltaTimes(openspace::Profile* profile, QWidget *parent)
    : QDialog(parent)
    , _profile(profile)
{
    setWindowTitle("Simulation Time Increments");
    QBoxLayout* layout = new QVBoxLayout(this);
    {
        _listWidget = new QListWidget;
        connect(
            _listWidget, &QListWidget::itemSelectionChanged,
            this, &DeltaTimes::listItemSelected
        );
        _listWidget->setAutoScroll(true);
        _listWidget->setLayoutMode(QListView::SinglePass);
        layout->addWidget(_listWidget);
    }

    {
        QBoxLayout* buttonLayout = new QHBoxLayout;
        _addButton = new QPushButton("Add Entry");
        connect(_addButton, &QPushButton::clicked, this, &DeltaTimes::addDeltaTimeValue);
        buttonLayout->addWidget(_addButton);

        _removeButton = new QPushButton("Remove LastEntry");
        connect(
            _removeButton, &QPushButton::clicked,
            this, &DeltaTimes::removeDeltaTimeValue
        );
        buttonLayout->addWidget(_removeButton);

        buttonLayout->addStretch();
        layout->addLayout(buttonLayout);
    }
    {
        _adjustLabel = new QLabel("Set Simulation Time Increment for key");
        layout->addWidget(_adjustLabel);
    }
    {
        QBoxLayout* box = new QHBoxLayout;
        _seconds = new QLineEdit;
        _seconds->setValidator(new QDoubleValidator);
        connect(_seconds, &QLineEdit::textChanged, this, &DeltaTimes::valueChanged);
        box->addWidget(_seconds);

        _value = new QLabel;
        box->addWidget(_value);
        layout->addLayout(box);
    }
    
    {
        QBoxLayout* box = new QHBoxLayout;
        _saveButton = new QPushButton("Save");
        connect(
            _saveButton, &QPushButton::clicked,
            this, &DeltaTimes::saveDeltaTimeValue
        );
        box->addWidget(_saveButton);

        _discardButton = new QPushButton("Discard");
        connect(
            _discardButton, &QPushButton::clicked,
            this, &DeltaTimes::discardDeltaTimeValue
        );
        box->addWidget(_discardButton);

        box->addStretch();
        layout->addLayout(box);
    }
    {
        QFrame* line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);
    }
    {
        QBoxLayout* footer = new QHBoxLayout;
        _errorMsg = new QLabel;
        _errorMsg->setObjectName("error-message");
        _errorMsg->setWordWrap(true);
        footer->addWidget(_errorMsg);

        _buttonBox = new QDialogButtonBox;
        _buttonBox->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        connect(
            _buttonBox, &QDialogButtonBox::accepted,
            this, &DeltaTimes::parseSelections
        );
        connect(
            _buttonBox, &QDialogButtonBox::rejected,
            this, &DeltaTimes::reject
        );
        footer->addWidget(_buttonBox);
        layout->addLayout(footer);
    }


    _data = _profile->deltaTimes();

    for (size_t d = 0; d < _data.size(); ++d) {
        std::string summary = createSummaryForDeltaTime(d, true);
        _listWidget->addItem(new QListWidgetItem(QString::fromStdString(summary)));
    }

    transitionEditMode(_listWidget->count() - 1, false);
}

std::string DeltaTimes::createSummaryForDeltaTime(size_t idx, bool forListView) {
    int k = (idx%10 == 9) ? 0 : idx%10 + 1;
    k = (idx == 0) ? 1 : k;
    std::string key = std::to_string(k);

    std::string s;
    if (idx >= 20) {
        s = "CTRL + " + key;
    }
    else if (idx >= 10) {
        s = "SHIFT + " + key;
    }
    else {
        s = key;
        if (forListView) {
            s += "      ";
        }
    }

    if (forListView) {
        s += "\t" + std::to_string(_data.at(idx));
        s += "\t";
        s += timeDescription(_data.at(idx)).toStdString();
    }
    return s;
}

void DeltaTimes::listItemSelected() {
    QListWidgetItem *item = _listWidget->currentItem();
    int index = _listWidget->row(item);

    if (index < (static_cast<int>(_data.size()) - 1)) {
        _listWidget->setCurrentRow(index);
    }

    if (_data.size() > 0) {
        if (_data.at(index) == 0) {
            _seconds->clear();
        }
        else {
            _seconds->setText(QString::number(_data.at(index)));
        }
    }
    _editModeNewItem = true;
    transitionEditMode(index, true);
}

void DeltaTimes::setLabelForKey(int index, bool editMode, std::string color) {
    std::string labelS = "Set Simulation Time Increment for key";
    if (index >= _data.size()) {
        index = _data.size() - 1;
    }
    if (editMode) {
        labelS += " '";
        labelS += createSummaryForDeltaTime(index, false);
        labelS += "':";
    }
    _adjustLabel->setText(QString::fromStdString(
        "<font color='" + color + "'>" + labelS + "</font>"
    ));
}

QString DeltaTimes::timeDescription(int value) {
    if (value == 0) {
        return "";
    }

    size_t i;
    for (i = 0; i < (TimeIntervals.size() - 1); ++i) {
        if (abs(value) >= TimeIntervals[i].secondsPerInterval) {
            break;
        }
    }
    return checkForTimeDescription(i, value);
}

void DeltaTimes::valueChanged(const QString& text) {
    if (_seconds->text() == "") {
        _errorMsg->setText("");
    }
    else {
        int value = _seconds->text().toDouble();
        if (value != 0) {
            _value->setText("<font color='black'>" +
                timeDescription(_seconds->text().toDouble()) + "</font>");
            _errorMsg->setText("");
        }
    }
}

QString DeltaTimes::checkForTimeDescription(int intervalIndex, int value) {
    double amount = static_cast<double>(value)
        / TimeIntervals[intervalIndex].secondsPerInterval;
    QString description = QString::number(amount, 'g', 2);
    return description += " " + TimeIntervals[intervalIndex].intervalName + "/sec";
}

bool DeltaTimes::isLineEmpty(int index) {
    bool isEmpty = true;
    if (!_listWidget->item(index)->text().isEmpty()) {
        isEmpty = false;
    }
    if (!_data.empty() && (_data.at(0) != 0)) {
        isEmpty = false;
    }
    return isEmpty;
}

void DeltaTimes::addDeltaTimeValue() {
    int currentListSize = _listWidget->count();
    const QString messageAddValue = "  (Enter integer value below & click 'Save')";

    if ((currentListSize == 1) && (isLineEmpty(0))) {
        // Special case where list is "empty" but really has one line that is blank.
        // This is done because QListWidget does not seem to like having its sole
        // remaining item being removed.
        _data.at(0) = 0;
        _listWidget->item(0)->setText(messageAddValue);
    }
    else if (_data.size() < MaxNumberOfKeys) {
        _data.push_back(0);
        _listWidget->addItem(new QListWidgetItem(messageAddValue));
    }
    else {
        _errorMsg->setText("Exceeded maximum amount of simulation time increments");
    }
    _listWidget->setCurrentRow(_listWidget->count() - 1);
    _seconds->setFocus(Qt::OtherFocusReason);
    _editModeNewItem = true;
}

void DeltaTimes::saveDeltaTimeValue() {
    QListWidgetItem *item = _listWidget->currentItem();
    if (item != nullptr) {
        int index = _listWidget->row(item);
        if (_data.size() > 0) {
            _data.at(index) = _seconds->text().toDouble();
            std::string summary = createSummaryForDeltaTime(index, true);
            _listWidget->item(index)->setText(QString::fromStdString(summary));
            //setLabelForKey(index, true, "black");
            transitionEditMode(index, false);
            _editModeNewItem = false;
        }
    }
}

void DeltaTimes::discardDeltaTimeValue(void) {
    listItemSelected();
    transitionEditMode(_listWidget->count() - 1, false);
    if (_editModeNewItem && !_data.empty() && _data.back() == 0) {
        removeDeltaTimeValue();
    }
    _editModeNewItem = false;
}

void DeltaTimes::removeDeltaTimeValue() {
    if (_listWidget->count() > 0) {
        if (_listWidget->count() == 1) {
            _data.at(0) = 0;
            _listWidget->item(0)->setText("");
        }
        else {
            delete _listWidget->takeItem(_listWidget->count() - 1);
            if (!_data.empty()) {
                _data.pop_back();
            }
        }
    }
    _listWidget->clearSelection();
    transitionEditMode(_listWidget->count() - 1, false);
}

void DeltaTimes::transitionEditMode(int index, bool state) {
    _listWidget->setEnabled(!state);
    _addButton->setEnabled(!state);
    _removeButton->setEnabled(!state);
    _buttonBox->setEnabled(!state);

    _saveButton->setEnabled(state);
    _discardButton->setEnabled(state);
    _adjustLabel->setEnabled(state);
    _seconds->setEnabled(state);
    _errorMsg->clear();

    if (state) {
        _seconds->setFocus(Qt::OtherFocusReason);
        setLabelForKey(index, true, "black");
    }
    else {
        _addButton->setFocus(Qt::OtherFocusReason);
        setLabelForKey(index, false, "light gray");
        _value->clear();
    }
    _errorMsg->clear();
}

void DeltaTimes::parseSelections() {
    if ((_data.size() == 1) && (_data.at(0) == 0)) {
        _data.clear();
    }
    int finalNonzeroIndex = _data.size() - 1;
    for (; finalNonzeroIndex >= 0; --finalNonzeroIndex) {
        if (_data.at(finalNonzeroIndex) != 0) {
            break;
        }
    }
    std::vector<double> tempDt;
    for (size_t i = 0; i < (finalNonzeroIndex + 1); ++i) {
        tempDt.push_back(_data[i]);
    }
    _profile->setDeltaTimes(tempDt);
    accept();
}

void DeltaTimes::keyPressEvent(QKeyEvent* evt) {
    if (evt->key() == Qt::Key_Enter || evt->key() == Qt::Key_Return) {
        if (_editModeNewItem) {
            saveDeltaTimeValue();
        }
        else {
            addDeltaTimeValue();
        }
        return;
    }
    else if (evt->key() == Qt::Key_Escape) {
        if (_editModeNewItem) {
            discardDeltaTimeValue();
            return;
        }
    }
    QDialog::keyPressEvent(evt);
}
