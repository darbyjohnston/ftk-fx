// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/ParametersPanel.h>

#include <fx/App/SceneModel.h>
#include <fx/App/Editors.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/FloatEditShuttle.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEditSlider.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/Spacer.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ToolButton.h>

#include <algorithm>

using namespace ftk;

namespace fx
{
    namespace app
    {
        std::shared_ptr<IWidget> ParametersPanel::Row::getWidget() const
        {
            return slider ?
                std::static_pointer_cast<IWidget>(slider) :
                std::static_pointer_cast<IWidget>(shuttle);
        }

        void ParametersPanel::Row::setValue(float value)
        {
            if (slider) slider->setValue(value);
            else shuttle->setValue(value);
        }

        void ParametersPanel::Row::setEnabled(bool value)
        {
            getWidget()->setEnabled(value);
        }

        void ParametersPanel::Row::setRangeFor(float value)
        {
            const float lo = std::min(value, info.min);
            const float hi = std::max(value, info.max);
            if (slider) slider->setRange(lo, hi);
            else shuttle->setRange(lo, hi);
        }

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

            // Otherwise a row is as wide as the widest thing in its own group,
            // so Transform's controls stop short of Emitter's and the column
            // reads as two panels that failed to line up.
            if (ParameterControl::Shuttle == info.control)
            {
                row.shuttle = FloatEditShuttle::create(context);
                row.shuttle->setRange(info.min, info.max);
                row.shuttle->setStep(info.step);
                row.shuttle->setValue(info.parameter->getConstant());
                row.shuttle->setDefault(info.defaultValue);
            }
            else
            {
                row.slider = FloatEditSlider::create(context);
                row.slider->setHStretch(Stretch::Expanding);
                row.slider->setRange(info.min, info.max);
                row.slider->setValue(info.parameter->getConstant());
                row.slider->setDefault(info.defaultValue);
            }
            std::weak_ptr<SceneModel> weak(_model);
            const ParameterInfo captured = info;
            const auto valueCallback = 
                [this, weak, captured](float value)
                {
                    if (_updating)
                        return;
                    auto model = weak.lock();
                    if (!model)
                        return;
                    // Opened here rather than from the press callback. A
                    // slider reports the value first and the press state
                    // second, so a drag's opening change has already happened
                    // by the time anything says a drag is under way -- and
                    // recording it separately makes the first undo land in the
                    // middle of the gesture.
                    model->beginEdit();
                    const sim::System before = model->getSystem();
                    setValue(*captured.parameter, _currentFrame, value);
                    model->systemChanged("Set " + captured.name, before);
                    _valuesUpdate();
                };

            // Closed when the value changed with the mouse up, which is a
            // typed value or the end of a drag. Both are one edit.
            const auto pressedCallback =
                [weak, captured](float, bool pressed)
                {
                    if (auto model = weak.lock())
                    {
                        if (!pressed)
                        {
                            model->endEdit("Set " + captured.name);
                        }
                    }
                };
            if (row.shuttle)
            {
                row.shuttle->setCallback(valueCallback);
                row.shuttle->setPressedCallback(pressedCallback);
            }
            else
            {
                row.slider->setCallback(valueCallback);
                row.slider->setPressedCallback(pressedCallback);
            }

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
            hLayout->setHStretch(Stretch::Expanding);
            row.getWidget()->setParent(hLayout);
            row.keyButton->setParent(hLayout);
            // A slider grows into the width the row is given and its button
            // follows the end of it. A shuttle does not grow -- it is the
            // size it is -- so the row's spare width has to go somewhere
            // after the button rather than before it, or the button ends up
            // against the far edge with the control it belongs to a hand's
            // width away.
            if (row.shuttle)
            {
                auto spacer = Spacer::create(
                    context, Orientation::Horizontal, hLayout);
                spacer->setHStretch(Stretch::Expanding);
            }
            layout->addRow(info.name + ":", hLayout);
            // Tagged so a shot can find the control rather than guess where it
            // is; a drag on one is the only way to reach the press callbacks.
            setScreenshotTag(row.getWidget(), "Parameters." + info.name);
            setScreenshotTag(row.keyButton, "Parameters." + info.name + ".Key");

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
            if (auto model = _model.lock())
            {
                if (_nameEdit)
                {
                    _nameEdit->setText(model->getSystem().getName());
                }
                const auto& emitter = model->getSystem().getEmitter();
                _shapeComboBox->setCurrentIndex(static_cast<int>(emitter.shape));
                _surfaceCheckBox->setChecked(emitter.surface);
                // The size only means something for a shape that has one.
                for (auto& row : _rows)
                {
                    if (0 == row.info.name.compare(0, 4, "Size"))
                    {
                        row.setEnabled(sim::hasVolume(emitter.shape));
                    }
                }
                _surfaceCheckBox->setEnabled(sim::hasVolume(emitter.shape));
                _drawTypeComboBox->setCurrentIndex(
                    static_cast<int>(model->getDrawType()));
            }
            for (auto& row : _rows)
            {
                const core::Parameter* parameter = row.info.parameter;
                const float value = parameter->getValue(_currentFrame);
                // The range a slider spans is a guess about what is useful,
                // not a limit on what is allowed -- a manipulator can drag an
                // emitter well past fifty units, and a curve can be keyed
                // anywhere. Opened up to hold whatever the value actually is,
                // and closed again when it comes back, so the number shown is
                // the number in the scene rather than the end of the track.
                row.setRangeFor(value);
                row.setValue(value);
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
            const std::shared_ptr<Editors>& editors,
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

            _widgetsUpdate();

            _currentFrameObserver = Observer<int>::create(
                model->observeCurrentFrame(),
                [this](int value)
                {
                    _currentFrame = value;
                    _valuesUpdate();
                });

            // Opening a scene replaces every value at once, and the sliders
            // would otherwise go on showing the ones from the scene before. It
            // also means a system may have appeared or gone, so the controls
            // are rebuilt rather than refreshed.
            _sceneObserver = Observer<int>::create(
                model->observeSceneChanged(),
                [this](int) { _widgetsUpdate(); });

            // And an undo needs a refresh too. The panel used to refresh only
            // on its own edits, so undoing one left the slider showing the
            // value the viewport had already stopped using.
            _parameterObserver = Observer<int>::create(
                model->observeParameterChanged(),
                [this](int) { _valuesUpdate(); });

            _currentSystemObserver = Observer<size_t>::create(
                model->observeCurrentSystem(),
                [this](size_t) { _widgetsUpdate(); });

            _particleSizeObserver = Observer<float>::create(
                model->observeParticleSize(),
                [this](float value)
                {
                    if (_particleSizeSlider)
                    {
                        _particleSizeSlider->setValue(value);
                    }
                });
        }

        void ParametersPanel::_widgetsUpdate()
        {
            auto model = _model.lock();
            auto context = getContext();
            if (!model || !context)
                return;
            _rows.clear();

            auto layout = VerticalLayout::create(context);
            // A border's width between the bellows, which is enough for the
            // panel behind them to show through as a hairline. Packed with
            // no gap they read as one long list with headings in it rather
            // than as the separate groups they are.
            layout->setSpacingRole(SizeRole::Border);

            std::weak_ptr<SceneModel> weak(model);

            // The rows come from the shared list rather than from a second
            // copy of it here, so the panel and the curve editor cannot
            // disagree about what exists or what it is called. The pointers in
            // it stay good because the model assigns into the systems it
            // already holds rather than replacing them -- see
            // SceneModel::_setSystems -- and this rebuilds when the list of
            // systems changes underneath them.
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

            // Renamed here rather than in the systems list. The list is what
            // picks a system; this panel is what edits the one that is picked,
            // and the name is one of the things it has.
            auto nameEdit = LineEdit::create(context);
            nameEdit->setHStretch(Stretch::Expanding);
            setScreenshotTag(nameEdit, "Parameters.Name");
            nameEdit->setText(model->getSystem().getName());
            nameEdit->setCallbackOnFocusLost(true);
            nameEdit->setTooltip("What this system is called");
            nameEdit->setCallback(
                [weak](const std::string& value)
                {
                    if (auto model = weak.lock())
                    {
                        model->setSystemName(model->getCurrentSystem(), value);
                    }
                });
            _nameEdit = nameEdit;
            auto systemLayout = FormLayout::create(context);
            systemLayout->setMarginRole(SizeRole::MarginSmall);
            systemLayout->addRow("Name:", nameEdit);
            auto systemBellows = Bellows::create(context, "System", layout);
            systemBellows->setWidget(systemLayout);
            systemBellows->setOpen(true);

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
                        if (!pressed) model->endEdit("Set Seed");
                    }
                });
            seedSlider->setCallback(
                [weak](int value)
                {
                    if (auto model = weak.lock())
                    {
                        model->beginEdit();
                        const sim::System before = model->getSystem();
                        model->getSystem().getEmitter().seed =
                            static_cast<uint64_t>(value);
                        model->systemChanged("Set Seed", before);
                    }
                });
            groups["Emitter"]->addRow("Seed:", seedSlider);

            // The shape is not a parameter -- it changes what the other values
            // mean rather than being one of them -- so it is a combo box among
            // the sliders, like the seed.
            auto shapeComboBox = ComboBox::create(
                context, sim::getEmitterShapeLabels());
            shapeComboBox->setCurrentIndex(
                static_cast<int>(model->getSystem().getEmitter().shape));
            shapeComboBox->setTooltip("Where in the emitter particles are born");
            shapeComboBox->setIndexCallback(
                [weak](int value)
                {
                    if (auto model = weak.lock())
                    {
                        const sim::System before = model->getSystem();
                        model->getSystem().getEmitter().shape =
                            static_cast<sim::EmitterShape>(value);
                        model->systemChanged("Set Shape", before);
                    }
                });
            groups["Emitter"]->addRow("Shape:", shapeComboBox);

            auto surfaceCheckBox = CheckBox::create(context, "Surface");
            surfaceCheckBox->setChecked(model->getSystem().getEmitter().surface);
            surfaceCheckBox->setTooltip(
                "Born on the shape rather than anywhere inside it");
            surfaceCheckBox->setCheckedCallback(
                [weak](bool value)
                {
                    if (auto model = weak.lock())
                    {
                        const sim::System before = model->getSystem();
                        model->getSystem().getEmitter().surface = value;
                        model->systemChanged("Set Surface", before);
                    }
                });
            groups["Emitter"]->addRow("", surfaceCheckBox);
            _shapeComboBox = shapeComboBox;
            _surfaceCheckBox = surfaceCheckBox;

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
                        if (!pressed) model->endEdit("Set Substeps");
                    }
                });
            substepsSlider->setCallback(
                [weak](int value)
                {
                    if (auto model = weak.lock())
                    {
                        model->beginEdit();
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
            auto particleSizeSlider = FloatEditSlider::create(context);
            particleSizeSlider->setRange(1.F, 16.F);
            particleSizeSlider->setValue(model->getParticleSize());
            particleSizeSlider->setDefault(model->getParticleSize());
            particleSizeSlider->setCallback(
                [weak](float value)
                {
                    if (auto model = weak.lock())
                    {
                        model->beginEdit();
                        model->setParticleSize(value);
                    }
                });
            _particleSizeSlider = particleSizeSlider;
            particleSizeSlider->setPressedCallback(
                [weak](float, bool pressed)
                {
                    if (auto model = weak.lock())
                    {
                        if (!pressed) model->endEdit("Set Particle Size");
                    }
                });
            displayLayout->addRow("Particle size:", particleSizeSlider);

            auto drawTypeComboBox = ComboBox::create(context, getDrawTypeLabels());
            drawTypeComboBox->setCurrentIndex(
                static_cast<int>(model->getDrawType()));
            drawTypeComboBox->setTooltip(
                "Draw the particles as flat discs or as shaded spheres");
            drawTypeComboBox->setIndexCallback(
                [weak](int value)
                {
                    if (auto model = weak.lock())
                    {
                        model->setDrawType(static_cast<DrawType>(value));
                    }
                });
            displayLayout->addRow("Draw:", drawTypeComboBox);
            _drawTypeComboBox = drawTypeComboBox;
            auto displayBellows = Bellows::create(context, "Display", layout);
            displayBellows->setWidget(displayLayout);
            displayBellows->setOpen(true);

            _setContent(layout);
            _valuesUpdate();
        }

        ParametersPanel::~ParametersPanel()
        {}

        std::shared_ptr<ParametersPanel> ParametersPanel::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<Editors>& editors,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ParametersPanel>(new ParametersPanel);
            out->_init(context, model, editors, parent);
            return out;
        }
    }
}
