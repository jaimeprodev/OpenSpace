/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014                                                                    *
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

#include <openspace/gui/guipropertycomponent.h>

#include <openspace/properties/scalarproperty.h>
#include <openspace/properties/vectorproperty.h>
#include <openspace/properties/stringproperty.h>
#include <openspace/properties/optionproperty.h>
#include <openspace/properties/selectionproperty.h>
#include <openspace/properties/triggerproperty.h>

#include "imgui.h"

namespace {
	const std::string _loggerCat = "GuiPropertyComponent";
	const ImVec2 size = ImVec2(350, 500);

	using namespace openspace::properties;

	void renderBoolProperty(Property* prop, const std::string& ownerName) {
		BoolProperty* p = static_cast<BoolProperty*>(prop);
		std::string name = p->guiName();

		BoolProperty::ValueType value = *p;
		ImGui::Checkbox((ownerName + "." + name).c_str(), &value);
		p->set(value);
	}

	void renderOptionProperty(Property* prop, const std::string& ownerName) {
		OptionProperty* p = static_cast<OptionProperty*>(prop);
		std::string name = p->guiName();

		int value = *p;
		std::vector<OptionProperty::Option> options = p->options();
		for (const OptionProperty::Option& o : options) {
			ImGui::RadioButton((ownerName + "." + name).c_str(), &value, o.value);
			ImGui::SameLine();
			ImGui::Text(o.description.c_str());
		}
		p->set(value);
	}

	void renderSelectionProperty(Property* prop, const std::string& ownerName) {
		SelectionProperty* p = static_cast<SelectionProperty*>(prop);
		std::string name = p->guiName();

		if (ImGui::CollapsingHeader((ownerName + "." + name).c_str())) {
			const std::vector<SelectionProperty::Option>& options = p->options();
			std::vector<int> newSelectedIndices;

			std::vector<int> selectedIndices = p->value();

			for (int i = 0; i < options.size(); ++i) {
				std::string description = options[i].description;
				bool selected = std::find(selectedIndices.begin(), selectedIndices.end(), i) != selectedIndices.end();
				ImGui::Checkbox(description.c_str(), &selected);

				if (selected)
					newSelectedIndices.push_back(i);
			}

			p->setValue(std::move(newSelectedIndices));
		}
	}

	void renderIntProperty(Property* prop, const std::string& ownerName) {
		IntProperty* p = static_cast<IntProperty*>(prop);
		std::string name = p->guiName();

		IntProperty::ValueType value = *p;
		ImGui::SliderInt((ownerName + "." + name).c_str(), &value, p->minValue(), p->maxValue());
		p->set(value);
	}

	void renderFloatProperty(Property* prop, const std::string& ownerName) {
		FloatProperty* p = static_cast<FloatProperty*>(prop);
		std::string name = p->guiName();

		FloatProperty::ValueType value = *p;
		ImGui::SliderFloat((ownerName + "." + name).c_str(), &value, p->minValue(), p->maxValue());
		p->set(value);
	}

	void renderVec2Property(Property* prop, const std::string& ownerName) {
		Vec2Property* p = static_cast<Vec2Property*>(prop);
		std::string name = p->guiName();

		Vec2Property::ValueType value = *p;

		ImGui::SliderFloat2((ownerName + "." + name).c_str(), &value.x, p->minValue().x, p->maxValue().x);
		p->set(value);
	}


	void renderVec3Property(Property* prop, const std::string& ownerName) {
		Vec3Property* p = static_cast<Vec3Property*>(prop);
		std::string name = p->guiName();

		Vec3Property::ValueType value = *p;

		ImGui::SliderFloat3((ownerName + "." + name).c_str(), &value.x, p->minValue().x, p->maxValue().x);
		p->set(value);
	}

	void renderTriggerProperty(Property* prop, const std::string& ownerName) {
		std::string name = prop->guiName();
		bool pressed = ImGui::Button((ownerName + "." + name).c_str());
		if (pressed)
			prop->set(0);
	}

}

namespace openspace {
namespace gui {

void GuiPropertyComponent::registerProperty(properties::Property* prop) {
	using namespace properties;

	std::string className = prop->className();

	if (className == "BoolProperty")
		_boolProperties.insert(prop);
	else if (className == "IntProperty")
		_intProperties.insert(prop);
	else if (className == "FloatProperty")
		_floatProperties.insert(prop);
	else if (className == "StringProperty")
		_stringProperties.insert(prop);
	else if (className == "Vec2Property")
		_vec2Properties.insert(prop);
	else if (className == "Vec3Property")
		_vec3Properties.insert(prop);
	else if (className == "OptionProperty")
		_optionProperty.insert(prop);
	else if (className == "TriggerProperty")
		_triggerProperty.insert(prop);
	else if (className == "SelectionProperty")
		_selectionProperty.insert(prop);
	else {
		LWARNING("Class name '" << className << "' not handled in GUI generation");
		return;
	}

	std::string fullyQualifiedId = prop->fullyQualifiedIdentifier();
	size_t pos = fullyQualifiedId.find('.');
	std::string owner = fullyQualifiedId.substr(0, pos);

	auto it = _propertiesByOwner.find(owner);
	if (it == _propertiesByOwner.end())
		_propertiesByOwner[owner] = { prop };
	else
		it->second.push_back(prop);

}

void GuiPropertyComponent::render() {
	using namespace openspace::properties;

	ImGui::Begin("Properties", &_isEnabled, size, 0.5f);

	//ImGui::ShowUserGuide();
	ImGui::Spacing();

	for (const auto& p : _propertiesByOwner) {
		if (ImGui::CollapsingHeader(p.first.c_str())) {
			for (properties::Property* prop : p.second) {
				if (_boolProperties.find(prop) != _boolProperties.end()) {
					renderBoolProperty(prop, p.first);
					continue;
				}

				if (_intProperties.find(prop) != _intProperties.end()) {
					renderIntProperty(prop, p.first);
					continue;
				}

				if (_floatProperties.find(prop) != _floatProperties.end()) {
					renderFloatProperty(prop, p.first);
					continue;
				}

				if (_vec2Properties.find(prop) != _vec2Properties.end()) {
					renderVec2Property(prop, p.first);
					continue;
				}

				if (_vec3Properties.find(prop) != _vec3Properties.end()) {
					renderVec3Property(prop, p.first);
					continue;
				}

				if (_optionProperty.find(prop) != _optionProperty.end()) {
					renderOptionProperty(prop, p.first);
					continue;
				}

				if (_triggerProperty.find(prop) != _triggerProperty.end()) {
					renderTriggerProperty(prop, p.first);
					continue;
				}

				if (_selectionProperty.find(prop) != _selectionProperty.end()) {
					renderSelectionProperty(prop, p.first);
					continue;
				}
			}
		}
	}

	ImGui::End();
}

} // gui
} // openspace
