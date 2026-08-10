// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/ParametersPanel.h>

#include <fx/App/SceneModel.h>
#include <fx/App/Panes.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEditSlider.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ToolButton.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        // Every row gets a default, and so a reset button. The three that had
        // none looked like rows that could not be reset rather than rows whose
        // author forgot, which is worse than not offering it anywhere.
        void ParametersPanel::_addRow(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<FormLayout>& layout,
            const ParameterInfo& info)
        {
            Row row;
            row.info = info;

            row.slider = FloatEditSlider::create(context);
            row.slider->setRange(info.min, info.max);
            row.slider->setValue(info.parameter->getConstant());
            row.slider->setDefault(info.defaultValue);
            std::weak_ptr<SceneModel> weak(_model);
            const ParameterInfo captured = info;
            row.slider->setCallback(
                [this, weak, captured](float value)
                {
                    if (_updating)
                        return;
                    auto model = weak.lock();
                    if (!model)
                        return;
                    const sim::System before = model->getSystem();
                    // Dragging an animated parameter moves the key at the
                    // playhead rather than throwing the animation away.
                    // Silently discarding a curve because a slider moved is
                    // not something anyone would ask for.
                    if (core::Parameter::Type::Curve == captured.parameter->getType())
                    {
                        core::Curve curve = captured.parameter->getCurve();
                        core::Key key;
                        key.frame = _currentFrame;
                        key.value = value;
                        curve.addKey(key);
                        captured.parameter->setCurve(curve);
                    }
                    else
                    {
                        captured.parameter->setConstant(value);
                    }
                    model->systemChanged("Set " + captured.name, before);
                    _valuesUpdate();
                });

            // The slider says when it was taken hold of and let go, so a drag
            // is one undo step.
            row.slider->setPressedCallback(
                [weak, captured](float, bool pressed)
                {
                    if (auto model = weak.lock())
                    {
                        if (pressed)
                        {
                            model->beginEdit();
                        }
                        else
                        {
                            model->endEdit("Set " + captured.name);
                        }
                    }
                });

            row.keyButton = ToolButton::create(context);
            row.keyButton->setIcon("Key");
            row.keyButton->setCheckable(true);
            row.keyButton->setTooltip(
                "Set a key at the current frame, or remove the one there");
            row.keyButton->setCheckedCallback(
                [this, captured](bool value)
                {
                    if (_updating)
                        return;
                    _key(captured, value);
                });

            // The button beside the slider rather than in a column of its own,
            // so the form's second column stays one widget wide and the rows
            // that have no key button still line up.
            auto hLayout = HorizontalLayout::create(context);
            hLayout->setSpacingRole(SizeRole::SpacingTool);
            row.slider->setParent(hLayout);
            row.keyButton->setParent(hLayout);
            layout->addRow(info.name + ":", hLayout);

            _rows.push_back(row);
        }

        void ParametersPanel::_key(const ParameterInfo& info, bool value)
        {
            auto model = _model.lock();
            if (!model)
                return;
            const sim::System before = model->getSystem();
            core::Parameter* parameter = info.parameter;
            if (value)
            {
                core::Curve curve =
                    core::Parameter::Type::Curve == parameter->getType() ?
                    parameter->getCurve() :
                    core::Curve();
                core::Key key;
                key.frame = _currentFrame;
                key.value = parameter->getValue(_currentFrame);
                curve.addKey(key);
                parameter->setCurve(curve);
            }
            else if (core::Parameter::Type::Curve == parameter->getType())
            {
                core::Curve curve = parameter->getCurve();
                const auto& keys = curve.getKeys();
                for (size_t i = 0; i < keys.size(); ++i)
                {
                    if (keys[i].frame == _currentFrame)
                    {
                        curve.removeKey(i);
                        break;
                    }
                }
                // The last key gone means the parameter is not animated any
                // more. It returns to the constant it was holding all along
                // rather than to whatever an empty curve evaluates to.
                if (curve.getKeys().empty())
                {
                    parameter->setConstant(parameter->getConstant());
                }
                else
                {
                    parameter->setCurve(curve);
                }
            }
            model->systemChanged(
                (value ? "Key " : "Unkey ") + info.name,
                before);
            _valuesUpdate();
        }

        void ParametersPanel::_valuesUpdate()
        {
            _updating = true;
            for (auto& row : _rows)
            {
                const core::Parameter* parameter = row.info.parameter;
                row.slider->setValue(parameter->getValue(_currentFrame));
                bool keyed = false;
                if (core::Parameter::Type::Curve == parameter->getType())
                {
                    for (const auto& key : parameter->getCurve().getKeys())
                    {
                        if (key.frame == _currentFrame)
                        {
                            keyed = true;
                            break;
                        }
                    }
                }
                row.keyButton->setChecked(keyed);
            }
            _updating = false;
        }

        void ParametersPanel::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<Panes>& panes,
            const std::shared_ptr<IWidget>& parent)
        {
            IPanel::_init(
                context,
                "Parameters",
                "Settings",
                "fx::app::ParametersPanel",
                parent);
            _model = model;
            _currentFrame = model->getCurrentFrame();

            auto layout = VerticalLayout::create(context);
            layout->setSpacingRole(SizeRole::None);

            std::weak_ptr<SceneModel> weak(model);

            // The rows come from the shared list rather than from a second
            // copy of it here, so the panel and the curve editor cannot
            // disagree about what exists or what it is called. The pointers in
            // it are safe because the model outlives the panel and its system
            // is a member rather than something that can be replaced.
            std::map<std::string, std::shared_ptr<FormLayout> > groups;
            std::vector<std::string> groupOrder;
            for (const auto& info : getParameters(model->getSystem()))
            {
                auto i = groups.find(info.group);
                if (i == groups.end())
                {
                    auto groupLayout = FormLayout::create(context);
                    groupLayout->setMarginRole(SizeRole::MarginSmall);
                    i = groups.insert({ info.group, groupLayout }).first;
                    groupOrder.push_back(info.group);
                }
                _addRow(context, i->second, info);
            }

            // The two rows that are not parameters, and so cannot be keyed:
            // both change the recipe rather than a value in it.
            auto seedSlider = IntEditSlider::create(context);
            seedSlider->setRange(1, 100);
            seedSlider->setValue(static_cast<int>(model->getSystem().getEmitter().seed));
            seedSlider->setDefault(static_cast<int>(model->getSystem().getEmitter().seed));
            seedSlider->setTooltip("Re-roll every random choice this emitter makes");
            seedSlider->setPressedCallback(
                [weak](int, bool pressed)
                {
                    if (auto model = weak.lock())
                    {
                        pressed ? model->beginEdit() : model->endEdit("Set Seed");
                    }
                });
            seedSlider->setCallback(
                [weak](int value)
                {
                    if (auto model = weak.lock())
                    {
                        const sim::System before = model->getSystem();
                        model->getSystem().getEmitter().seed =
                            static_cast<uint64_t>(value);
                        model->systemChanged("Set Seed", before);
                    }
                });
            groups["Emitter"]->addRow("Seed:", seedSlider);

            auto substepsSlider = IntEditSlider::create(context);
            substepsSlider->setRange(1, 8);
            substepsSlider->setValue(model->getSystem().getSubsteps());
            substepsSlider->setDefault(model->getSystem().getSubsteps());
            substepsSlider->setTooltip(
                "Solver steps per frame. More is smoother and slower");
            substepsSlider->setPressedCallback(
                [weak](int, bool pressed)
                {
                    if (auto model = weak.lock())
                    {
                        pressed ? model->beginEdit() : model->endEdit("Set Substeps");
                    }
                });
            substepsSlider->setCallback(
                [weak](int value)
                {
                    if (auto model = weak.lock())
                    {
                        const sim::System before = model->getSystem();
                        model->getSystem().setSubsteps(value);
                        model->systemChanged("Set Substeps", before);
                    }
                });
            groups["Forces"]->addRow("Substeps:", substepsSlider);

            for (const auto& group : groupOrder)
            {
                auto bellows = Bellows::create(context, group, layout);
                bellows->setWidget(groups[group]);
                bellows->setOpen(true);
            }

            auto displayLayout = FormLayout::create(context);
            displayLayout->setMarginRole(SizeRole::MarginSmall);
            auto pointSizeSlider = FloatEditSlider::create(context);
            pointSizeSlider->setRange(1.F, 16.F);
            pointSizeSlider->setValue(model->getPointSize());
            pointSizeSlider->setDefault(model->getPointSize());
            pointSizeSlider->setCallback(
                [weak](float value)
                {
                    if (auto model = weak.lock())
                    {
                        model->setPointSize(value);
                    }
                });
            _pointSizeObserver = Observer<float>::create(
                model->observePointSize(),
                [pointSizeSlider](float value)
                {
                    pointSizeSlider->setValue(value);
                });
            pointSizeSlider->setPressedCallback(
                [weak](float, bool pressed)
                {
                    if (auto model = weak.lock())
                    {
                        pressed ?
                            model->beginEdit() :
                            model->endEdit("Set Point Size");
                    }
                });
            displayLayout->addRow("Point size:", pointSizeSlider);
            auto displayBellows = Bellows::create(context, "Display", layout);
            displayBellows->setWidget(displayLayout);
            displayBellows->setOpen(true);

            _setContent(layout);

            _currentFrameObserver = Observer<int>::create(
                model->observeCurrentFrame(),
                [this](int value)
                {
                    _currentFrame = value;
                    _valuesUpdate();
                });

            // Opening a scene replaces every value at once, and the sliders
            // would otherwise go on showing the ones from the scene before.
            _sceneObserver = Observer<int>::create(
                model->observeSceneChanged(),
                [this](int) { _valuesUpdate(); });

            // And so does an undo. The panel used to refresh only on its own
            // edits, so undoing one left the slider showing the value the
            // viewport had already stopped using.
            _parameterObserver = Observer<int>::create(
                model->observeParameterChanged(),
                [this](int) { _valuesUpdate(); });

            _pointSizeObserver = Observer<float>::create(
                model->observePointSize(),
                [pointSizeSlider](float value)
                {
                    pointSizeSlider->setValue(value);
                });
        }

        ParametersPanel::~ParametersPanel()
        {}

        std::shared_ptr<ParametersPanel> ParametersPanel::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<Panes>& panes,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ParametersPanel>(new ParametersPanel);
            out->_init(context, model, panes, parent);
            return out;
        }
    }
}
