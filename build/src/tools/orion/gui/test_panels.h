// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include <ui/panel.h>
#include <ui/label.h>
#include <ui/layout.h>
#include <ui/compositor.h>
#include <core/util.h>

#include "parameter.h"

class NumberPanel : public gui::Label
{
public:
	NumberPanel(gui::Panel* parent)
		: gui::Label(parent)
		, cursor_index(0)
		, is_captured(0)
		, value(0.0f)
		, step(0.01f)
	{
	}

	virtual void update(gui::Compositor* compositor, float delta_seconds) override;
	virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;
	virtual void handle_event(gui::EventArgs& args) override;

	float value;
	float step;
	gui::Point down_position;
	uint8_t is_captured;
	uint32_t cursor_index;

	gemini::Delegate<void(float)> value_changed;
};


class ParameterVec3Widget : public gui::Panel
{
public:
	Parameter* target;

	gui::Label* labels[3];
	NumberPanel* panels[3];

	void update_target_parameter(float)
	{
		glm::vec3* value = reinterpret_cast<glm::vec3*>(target->value);
		*value = glm::vec3(panels[0]->value, panels[1]->value, panels[2]->value);
	}

	ParameterVec3Widget(gui::Panel* parent, Parameter* parameter)
		: gui::Panel(parent)
		, target(parameter)
	{
		set_background_color(gemini::Color(0.5f, 0.5f, 0.5f));

		gui::HorizontalLayout* layout = new gui::HorizontalLayout();
		labels[0] = new gui::Label(this);
		labels[0]->set_background_color(gemini::Color(1.0f, 0.0f, 0.0f));
		labels[0]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		labels[0]->set_font("debug", 24);
		labels[0]->set_text("X:");
		layout->add_panel(labels[0]);
		panels[0] = new NumberPanel(this);
		panels[0]->set_font("debug", 16);
		panels[0]->set_text("0");
		panels[0]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		layout->add_panel(panels[0]);
		panels[0]->value_changed = MAKE_MEMBER_DELEGATE(void(float), ParameterVec3Widget, &ParameterVec3Widget::update_target_parameter, this);

		labels[1] = new gui::Label(this);
		labels[1]->set_background_color(gemini::Color(0.0f, 1.0f, 0.0f));
		labels[1]->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
		labels[1]->set_font("debug", 16);
		labels[1]->set_text("Y:");
		layout->add_panel(labels[1]);
		panels[1] = new NumberPanel(this);
		panels[1]->set_font("debug", 16);
		panels[1]->set_text("0");
		panels[1]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		layout->add_panel(panels[1]);
		panels[1]->value_changed = MAKE_MEMBER_DELEGATE(void(float), ParameterVec3Widget, &ParameterVec3Widget::update_target_parameter, this);

		labels[2] = new gui::Label(this);
		labels[2]->set_background_color(gemini::Color(0.0f, 0.0f, 1.0f));
		labels[2]->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
		labels[2]->set_font("debug", 16);
		labels[2]->set_text("Z:");
		layout->add_panel(labels[2]);
		panels[2] = new NumberPanel(this);
		panels[2]->set_font("debug", 16);
		panels[2]->set_text("0");
		panels[2]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		layout->add_panel(panels[2]);
		panels[2]->value_changed = MAKE_MEMBER_DELEGATE(void(float), ParameterVec3Widget, &ParameterVec3Widget::update_target_parameter, this);

		set_layout(layout);
	}

	void parameter_changed()
	{
		glm::vec3* vector3 = reinterpret_cast<glm::vec3*>(target->value);
		panels[0]->value = vector3->x;
		panels[1]->value = vector3->y;
		panels[2]->value = vector3->z;

		panels[0]->set_text(core::str::format("%2.4f", panels[0]->value));
		panels[1]->set_text(core::str::format("%2.4f", panels[1]->value));
		panels[2]->set_text(core::str::format("%2.4f", panels[2]->value));
	}
};



class ParameterQuatWidget : public gui::Panel
{
public:
	Parameter* target;

	gui::Label* labels[3];
	NumberPanel* panels[3];

	void update_target_parameter(float)
	{
		// convert euler angles to quat
		glm::quat* value = reinterpret_cast<glm::quat*>(target->value);

		glm::vec3 euler_angles(
			mathlib::degrees_to_radians(panels[0]->value),
			mathlib::degrees_to_radians(panels[1]->value),
			mathlib::degrees_to_radians(panels[2]->value)
		);
		*value = glm::quat(euler_angles);
		LOGV("q: %2.2f, %2.2f, %2.2f, %2.2f\n", value->x, value->y, value->z, value->w);
	}

	ParameterQuatWidget(gui::Panel* parent, Parameter* parameter)
		: gui::Panel(parent)
		, target(parameter)
	{
		set_background_color(gemini::Color(0.5f, 0.5f, 0.5f));

		gui::HorizontalLayout* layout = new gui::HorizontalLayout();
		labels[0] = new gui::Label(this);
		labels[0]->set_background_color(gemini::Color(1.0f, 0.0f, 0.0f));
		labels[0]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		labels[0]->set_font("debug", 24);
		labels[0]->set_text("X:");
		layout->add_panel(labels[0]);
		panels[0] = new NumberPanel(this);
		panels[0]->set_font("debug", 16);
		panels[0]->set_text("0");
		panels[0]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		layout->add_panel(panels[0]);
		panels[0]->value_changed = MAKE_MEMBER_DELEGATE(void(float), ParameterQuatWidget, &ParameterQuatWidget::update_target_parameter, this);

		labels[1] = new gui::Label(this);
		labels[1]->set_background_color(gemini::Color(0.0f, 1.0f, 0.0f));
		labels[1]->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
		labels[1]->set_font("debug", 16);
		labels[1]->set_text("Y:");
		layout->add_panel(labels[1]);
		panels[1] = new NumberPanel(this);
		panels[1]->set_font("debug", 16);
		panels[1]->set_text("0");
		panels[1]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		layout->add_panel(panels[1]);
		panels[1]->value_changed = MAKE_MEMBER_DELEGATE(void(float), ParameterQuatWidget, &ParameterQuatWidget::update_target_parameter, this);

		labels[2] = new gui::Label(this);
		labels[2]->set_background_color(gemini::Color(0.0f, 0.0f, 1.0f));
		labels[2]->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
		labels[2]->set_font("debug", 16);
		labels[2]->set_text("Z:");
		layout->add_panel(labels[2]);
		panels[2] = new NumberPanel(this);
		panels[2]->set_font("debug", 16);
		panels[2]->set_text("0");
		panels[2]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		layout->add_panel(panels[2]);
		panels[2]->value_changed = MAKE_MEMBER_DELEGATE(void(float), ParameterQuatWidget, &ParameterQuatWidget::update_target_parameter, this);

		set_layout(layout);
	}

	void parameter_changed()
	{
		glm::quat* quaternion = reinterpret_cast<glm::quat*>(target->value);

		*quaternion = glm::normalize(*quaternion);
		glm::vec3 euler_angles = glm::eulerAngles(*quaternion);

		panels[0]->value = mathlib::radians_to_degrees(euler_angles.x);
		panels[1]->value = mathlib::radians_to_degrees(euler_angles.y);
		panels[2]->value = mathlib::radians_to_degrees(euler_angles.z);

		panels[0]->set_text(core::str::format("%2.4f", panels[0]->value));
		panels[1]->set_text(core::str::format("%2.4f", panels[1]->value));
		panels[2]->set_text(core::str::format("%2.4f", panels[2]->value));
	}
};