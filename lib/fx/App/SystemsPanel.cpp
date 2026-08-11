// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/SystemsPanel.h>

#include <fx/App/SceneModel.h>

#include <ftk/UI/ButtonGroup.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ToolButton.h>

#include <ftk/Core/Format.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void SystemsPanel::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            IPanel::_init(
                context,
                "Systems",
                "Settings",
                "fx::app::SystemsPanel",
                parent);
            _model = model;

            // Radio: there is always exactly one current system, and clicking
            // the one that is already current leaves it current rather than
            // leaving the panels pointed at nothing.
            _nameGroup = ButtonGroup::create(context, ButtonGroupType::Radio);
            std::weak_ptr<SceneModel> weak(_model);
            _nameGroup->setCheckedCallback(
                [this, weak](int index, bool value)
                {
                    if (_updating || !value)
                        return;
                    if (auto model = weak.lock())
                    {
                        model->setCurrentSystem(index);
                    }
                });

            _listLayout = VerticalLayout::create(context);
            _listLayout->setMarginRole(SizeRole::MarginSmall);
            _listLayout->setSpacingRole(SizeRole::SpacingTool);

            auto addButton = ToolButton::create(context);
            addButton->setIcon("SystemAdd");
            addButton->setTooltip("Add a system");
            addButton->setClickedCallback(
                [weak]
                {
                    if (auto model = weak.lock())
                    {
                        model->addSystem();
                    }
                });

            auto duplicateButton = ToolButton::create(context);
            duplicateButton->setIcon("SystemDuplicate");
            duplicateButton->setTooltip("Duplicate the current system");
            duplicateButton->setClickedCallback(
                [weak]
                {
                    if (auto model = weak.lock())
                    {
                        model->duplicateSystem(model->getCurrentSystem());
                    }
                });

            _removeButton = ToolButton::create(context);
            _removeButton->setIcon("SystemRemove");
            _removeButton->setTooltip("Remove the current system");
            _removeButton->setClickedCallback(
                [weak]
                {
                    if (auto model = weak.lock())
                    {
                        model->removeSystem(model->getCurrentSystem());
                    }
                });

            auto buttonLayout = HorizontalLayout::create(context);
            buttonLayout->setMarginRole(SizeRole::MarginSmall);
            buttonLayout->setSpacingRole(SizeRole::SpacingTool);
            addButton->setParent(buttonLayout);
            duplicateButton->setParent(buttonLayout);
            _removeButton->setParent(buttonLayout);

            auto layout = VerticalLayout::create(context);
            layout->setSpacingRole(SizeRole::None);
            _listLayout->setParent(layout);
            Divider::create(context, Orientation::Vertical, layout);
            buttonLayout->setParent(layout);
            _setContent(layout);

            _rowsUpdate();

            _sceneObserver = Observer<int>::create(
                model->observeSceneChanged(),
                [this](int) { _rowsUpdate(); });
            // A rename or an enable arrives as a parameter edit rather than as
            // a new scene, and so does every step of a slider drag: the rows
            // are only rebuilt when the names actually differ.
            _parameterObserver = Observer<int>::create(
                model->observeParameterChanged(),
                [this](int) { _rowsUpdate(); });
            _currentSystemObserver = Observer<size_t>::create(
                model->observeCurrentSystem(),
                [this](size_t) { _valuesUpdate(); });
        }

        SystemsPanel::~SystemsPanel()
        {}

        std::shared_ptr<SystemsPanel> SystemsPanel::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SystemsPanel>(new SystemsPanel);
            out->_init(context, model, parent);
            return out;
        }

        void SystemsPanel::_rowsUpdate()
        {
            auto model = _model.lock();
            auto context = getContext();
            if (!model || !context)
                return;

            std::vector<std::string> names;
            for (size_t i = 0; i < model->getSystemCount(); ++i)
            {
                names.push_back(model->getSystem(i).getName());
            }
            if (names == _names)
            {
                _valuesUpdate();
                return;
            }
            _names = names;

            // Copied, because setParent(nullptr) erases from the very list
            // getChildren() returns a reference to.
            const auto children = _listLayout->getChildren();
            for (const auto& child : children)
            {
                child->setParent(nullptr);
            }
            _rows.clear();
            _nameGroup->clearButtons();

            std::weak_ptr<SceneModel> weak(_model);
            for (size_t i = 0; i < names.size(); ++i)
            {
                Row row;

                row.enabledCheckBox = CheckBox::create(context);
                row.enabledCheckBox->setTooltip("Solve this system");
                row.enabledCheckBox->setCheckedCallback(
                    [this, weak, i](bool value)
                    {
                        if (_updating)
                            return;
                        if (auto model = weak.lock())
                        {
                            model->setSystemEnabled(i, value);
                        }
                    });

                // A button rather than a label, so that the current system is
                // shown the way every other current thing in the application
                // is: checked.
                row.nameButton = ToolButton::create(context, names[i]);
                row.nameButton->setCheckable(true);
                row.nameButton->setHStretch(Stretch::Expanding);
                _nameGroup->addButton(row.nameButton);

                auto rowLayout = HorizontalLayout::create(context, _listLayout);
                rowLayout->setSpacingRole(SizeRole::SpacingTool);
                row.enabledCheckBox->setParent(rowLayout);
                row.nameButton->setParent(rowLayout);
                setScreenshotTag(rowLayout, Format("Systems.{0}").arg(i));

                _rows.push_back(row);
            }

            _valuesUpdate();
        }

        void SystemsPanel::_valuesUpdate()
        {
            auto model = _model.lock();
            if (!model)
                return;

            _updating = true;
            const size_t current = model->getCurrentSystem();
            for (size_t i = 0; i < _rows.size() && i < model->getSystemCount(); ++i)
            {
                _rows[i].enabledCheckBox->setChecked(
                    model->getSystem(i).isEnabled());
                _rows[i].nameButton->setChecked(i == current);
            }
            // The last system is never removed, so the button says so rather
            // than the click doing nothing.
            _removeButton->setEnabled(model->getSystemCount() > 1);
            _updating = false;
        }
    }
}
