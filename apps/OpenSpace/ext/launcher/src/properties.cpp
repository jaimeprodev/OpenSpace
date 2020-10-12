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

#include "properties.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <iostream>

namespace {
    const openspace::Profile::Property kBlank {
        openspace::Profile::Property::SetType::SetPropertyValue,
        "",
        ""
    };
} // namespace

Properties::Properties(openspace::Profile* profile, QWidget *parent)
    : QDialog(parent)
    , _profile(profile)
    , _data(_profile->properties())
{
    setWindowTitle("Set Property Values");

    QBoxLayout* layout = new QVBoxLayout(this);
    {
        _list = new QListWidget;
        connect(
            _list, &QListWidget::itemSelectionChanged,
            this, &Properties::listItemSelected
        );
        for (size_t i = 0; i < _data.size(); ++i) {
            _list->addItem(new QListWidgetItem(createOneLineSummary(_data[i])));
        }
        layout->addWidget(_list);
    }
    {
        QBoxLayout* box = new QHBoxLayout;
        _addButton = new QPushButton("Add New");
        connect(_addButton, &QPushButton::clicked, this, &Properties::listItemAdded);
        box->addWidget(_addButton);

        _removeButton = new QPushButton("Remove");
        connect(_removeButton, &QPushButton::clicked, this, &Properties::listItemRemove);
        box->addWidget(_removeButton);

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
        _commandLabel = new QLabel("Property Set Command");
        layout->addWidget(_commandLabel);

        _commandCombo = new QComboBox;
        _commandCombo->addItems({ "SetPropertyValue", "SetPropertyValueSingle" });
        layout->addWidget(_commandCombo);

        _propertyLabel = new QLabel("Property");
        layout->addWidget(_propertyLabel);
        _propertyEdit = new QLineEdit;
        _propertyEdit->setToolTip("Exact string is required for the property to be set");
        layout->addWidget(_propertyEdit);

        _valueLabel = new QLabel("Value to Set");
        layout->addWidget(_valueLabel);
        _valueEdit = new QLineEdit;
        layout->addWidget(_valueEdit);

        {
            QBoxLayout* box = new QHBoxLayout;
            _saveButton = new QPushButton("Save");
            connect(_saveButton, &QPushButton::clicked, this, &Properties::listItemSave);
            box->addWidget(_saveButton);

            _cancelButton = new QPushButton("Cancel");
            connect(
                _cancelButton, &QPushButton::clicked,
                this, &Properties::listItemCancelSave
            );
            box->addWidget(_cancelButton);

            box->addStretch();

            layout->addLayout(box);
        }
    }
    {
        QBoxLayout* footerLayout = new QHBoxLayout;

        _errorMsg = new QLabel;
        _errorMsg->setObjectName("error-message");
        _errorMsg->setWordWrap(true);
        footerLayout->addWidget(_errorMsg);

        _buttonBox = new QDialogButtonBox;
        _buttonBox->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);

        connect(
            _buttonBox, &QDialogButtonBox::accepted,
            this, &Properties::parseSelections
        );
        QObject::connect(
            _buttonBox, &QDialogButtonBox::rejected,
            this, &Properties::reject
        );
        footerLayout->addWidget(_buttonBox);
        layout->addLayout(footerLayout);
    }

    transitionFromEditMode();
}

QString Properties::createOneLineSummary(openspace::Profile::Property p) {
    QString summary = QString(p.name.c_str());
    summary += " = ";
    summary += QString(p.value.c_str());
    summary += " (SetPropertyValue";
    if (p.setType == openspace::Profile::Property::SetType::SetPropertyValueSingle) {
        summary += "Single";
    }
    summary += ")";
    return summary;
}

void Properties::listItemSelected() {
    QListWidgetItem* item = _list->currentItem();
    int index = _list->row(item);

    if (_data.size() > 0) {
        openspace::Profile::Property& p = _data[index];
        if (p.setType == openspace::Profile::Property::SetType::SetPropertyValue) {
            _commandCombo->setCurrentIndex(0);
        }
        else {
            _commandCombo->setCurrentIndex(1);
        }
        _propertyEdit->setText(QString::fromStdString(p.name));
        _valueEdit->setText(QString::fromStdString(p.value));
    }
    transitionToEditMode();
}

bool Properties::isLineEmpty(int index) {
    bool isEmpty = true;
    if (!_list->item(index)->text().isEmpty()) {
        isEmpty = false;
    }
    if (!_data.empty() && !_data.at(0).name.empty()) {
        isEmpty = false;
    }
    return isEmpty;
}

void Properties::listItemAdded(void) {
    int currentListSize = _list->count();

     if ((currentListSize == 1) && (isLineEmpty(0))) {
         //Special case where list is "empty" but really has one line that is blank.
         // This is done because QListWidget does not seem to like having its sole
         // remaining item being removed.
         _data.at(0) = kBlank;
         _list->item(0)->setText("  (Enter details below & click 'Save')");
         _list->setCurrentRow(0);
         transitionToEditMode();
     }
     else {
         _data.push_back(kBlank);
         _list->addItem(new QListWidgetItem("  (Enter details below & click 'Save')"));
         //Scroll down to that blank line highlighted
         _list->setCurrentRow(_list->count() - 1);
     }

    // Blank-out the 2 text fields, set combo box to index 0
    _commandCombo->setCurrentIndex(0);
    _propertyEdit->setText(QString(_data.back().name.c_str()));
    _valueEdit->setText(QString(_data.back().value.c_str()));
    _commandCombo->setFocus(Qt::OtherFocusReason);
    _editModeNewItem = true;
}

void Properties::listItemSave(void) {
    if (!areRequiredFormsFilled()) {
        return;
    }

    QListWidgetItem* item = _list->currentItem();
    int index = _list->row(item);

    if ( _data.size() > 0) {
        if (_commandCombo->currentIndex() == 0) {
            _data[index].setType
                = openspace::Profile::Property::SetType::SetPropertyValue;
        }
        else {
            _data[index].setType
                = openspace::Profile::Property::SetType::SetPropertyValueSingle;
        }
        _data[index].name = _propertyEdit->text().toStdString();
        _data[index].value = _valueEdit->text().toStdString();
        _list->item(index)->setText(createOneLineSummary(_data[index]));
    }
    transitionFromEditMode();
    _editModeNewItem = false;
}

bool Properties::areRequiredFormsFilled() {
    bool requiredFormsFilled = true;
    QString errors;
    if (_propertyEdit->text().length() == 0) {
        errors += "Missing property name";
        requiredFormsFilled = false;
    }
    if (_valueEdit->text().length() == 0) {
        if (errors.length() > 0) {
            errors += ", ";
        }
        errors += "Missing value";
        requiredFormsFilled = false;
    }
    _errorMsg->setText("<font color='red'>" + errors + "</font>");
    return requiredFormsFilled;
}

void Properties::listItemCancelSave(void) {
    listItemSelected();
    transitionFromEditMode();
    if (_editModeNewItem) {
        if (_data.size() > 0) {
            if (_data.back().name.length() == 0 || _data.back().value.length() == 0) {
                listItemRemove();
            }
        }
    }
    _editModeNewItem = false;
}

void Properties::listItemRemove(void) {
    if (_list->count() > 0) {
        if (_list->currentRow() >= 0 && _list->currentRow() < _list->count()) {
            if (_list->count() == 1) {
                //Special case where last remaining item is being removed (QListWidget
                // doesn't like the final item being removed so instead clear it)
                _data.at(0) = kBlank;
                _list->item(0)->setText("");
            }
            else {
                int index = _list->currentRow();
                if (index >= 0 && index < _list->count()) {
                    delete _list->takeItem(index);
                    if (_data.size() > 0) {
                        _data.erase(_data.begin() + index);
                    }
                }
            }
        }
    }
    transitionFromEditMode();
}

void Properties::transitionToEditMode(void) {
    _list->setDisabled(true);
    _addButton->setDisabled(true);
    _removeButton->setDisabled(true);
    _saveButton->setDisabled(true);
    _cancelButton->setDisabled(true);
    _buttonBox->setDisabled(true);

    _commandLabel->setText("<font color='black'>Property Set Command</font>");
    _propertyLabel->setText("<font color='black'>Property</font>");
    _valueLabel->setText("<font color='black'>Value to set</font>");
    editBoxDisabled(false);
    _errorMsg->setText("");
}

void Properties::transitionFromEditMode(void) {
    _list->setDisabled(false);
    _addButton->setDisabled(false);
    _removeButton->setDisabled(false);
    _saveButton->setDisabled(false);
    _cancelButton->setDisabled(false);
    _buttonBox->setDisabled(false);

    _commandLabel->setText("<font color='light gray'>Property Set Command</font>");
    _propertyLabel->setText("<font color='light gray'>Property</font>");
    _valueLabel->setText("<font color='light gray'>Value to set</font>");
    editBoxDisabled(true);
    _errorMsg->setText("");
}

void Properties::editBoxDisabled(bool disabled) {
    _commandLabel->setDisabled(disabled);
    _commandCombo->setDisabled(disabled);
    _propertyLabel->setDisabled(disabled);
    _propertyEdit->setDisabled(disabled);
    _valueLabel->setDisabled(disabled);
    _valueEdit->setDisabled(disabled);
    _saveButton->setDisabled(disabled);
    _cancelButton->setDisabled(disabled);
}

void Properties::parseSelections() {
    // Handle case with only one remaining but empty line
    if ((_data.size() == 1) && (_data.at(0).name.compare("") == 0)) {
        _data.clear();
    }
    _profile->setProperties(_data);
    accept();
}

void Properties::keyPressEvent(QKeyEvent* evt) {
    if(evt->key() == Qt::Key_Enter || evt->key() == Qt::Key_Return) {
        if (_editModeNewItem) {
            listItemSave();
        }
        return;
    }
    else if(evt->key() == Qt::Key_Escape) {
        if (_editModeNewItem) {
            listItemCancelSave();
            return;
        }
    }
    QDialog::keyPressEvent(evt);
}

