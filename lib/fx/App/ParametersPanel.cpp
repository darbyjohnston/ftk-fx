// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/ParametersPanel.h>

#include <fx/App/SceneModel.h>
#include <fx/App/Viewport.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEditSlider.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void ParametersPanel::_addSlider(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<FormLayout>& layout,
            const std::string& label,
            core::Parameter& parameter,
            float min,
            float max)
        {
            auto slider = FloatEditSlider::create(context);
            slider->setRange(min, max);
            slider->setValue(parameter.getConstant());
            slider->setDefault(parameter.getConstant());
            std::weak_ptr<SceneModel> weak(_model);
            core::Parameter* p = &parameter;
            slider->setCallback(
                [weak, p](float value)
                {
                    p->setConstant(value);
                    if (auto model = weak.lock())
                    {
                        model->parameterChanged();
                    }
                });
            layout->addRow(label + ":", slider);
        }

        void ParametersPanel::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<Viewport>& viewport,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "fx::app::ParametersPanel", parent);
            _model = model;

            auto layout = VerticalLayout::create(context);
            layout->setSpacingRole(SizeRole::None);

            // Held by raw reference into the system, which is safe because the
            // model outlives the panel and the system is a member of it rather
            // than something that can be replaced.
            auto& emitter = model->getSystem().getEmitter();
            auto& forces = model->getSystem().getForces();
            std::weak_ptr<SceneModel> weak(model);

            auto emitterLayout = FormLayout::create(context);
            emitterLayout->setMarginRole(SizeRole::MarginSmall);
            _addSlider(context, emitterLayout, "Rate", emitter.rate, 0.F, 2000.F);
            _addSlider(context, emitterLayout, "Speed", emitter.speed, 0.F, 30.F);
            _addSlider(context, emitterLayout, "Speed variance", emitter.speedVariance, 0.F, 1.F);
            _addSlider(context, emitterLayout, "Spread", emitter.spread, 0.F, 180.F);
            _addSlider(context, emitterLayout, "Lifespan", emitter.lifespan, .1F, 10.F);
            _addSlider(context, emitterLayout, "Lifespan variance", emitter.lifespanVariance, 0.F, 1.F);
            auto seedSlider = IntEditSlider::create(context);
            seedSlider->setRange(1, 100);
            seedSlider->setValue(static_cast<int>(emitter.seed));
            seedSlider->setTooltip("Re-roll every random choice this emitter makes");
            sim::PointEmitter* emitterPtr = &emitter;
            seedSlider->setCallback(
                [weak, emitterPtr](int value)
                {
                    emitterPtr->seed = static_cast<uint64_t>(value);
                    if (auto model = weak.lock())
                    {
                        model->parameterChanged();
                    }
                });
            emitterLayout->addRow("Seed:", seedSlider);
            auto emitterBellows = Bellows::create(context, "Emitter", layout);
            emitterBellows->setWidget(emitterLayout);
            emitterBellows->setOpen(true);

            auto forcesLayout = FormLayout::create(context);
            forcesLayout->setMarginRole(SizeRole::MarginSmall);
            _addSlider(context, forcesLayout, "Gravity", forces.gravity.y, -50.F, 50.F);
            _addSlider(context, forcesLayout, "Drag", forces.drag, 0.F, 4.F);
            auto substepsSlider = IntEditSlider::create(context);
            substepsSlider->setRange(1, 8);
            substepsSlider->setValue(model->getSystem().getSubsteps());
            substepsSlider->setTooltip(
                "Solver steps per frame. More is smoother and slower");
            substepsSlider->setCallback(
                [weak](int value)
                {
                    if (auto model = weak.lock())
                    {
                        model->getSystem().setSubsteps(value);
                        model->parameterChanged();
                    }
                });
            forcesLayout->addRow("Substeps:", substepsSlider);
            auto forcesBellows = Bellows::create(context, "Forces", layout);
            forcesBellows->setWidget(forcesLayout);
            forcesBellows->setOpen(true);

            auto displayLayout = FormLayout::create(context);
            displayLayout->setMarginRole(SizeRole::MarginSmall);
            auto pointSizeSlider = FloatEditSlider::create(context);
            pointSizeSlider->setRange(1.F, 16.F);
            pointSizeSlider->setValue(viewport->getPointSize());
            std::weak_ptr<Viewport> viewportWeak(viewport);
            pointSizeSlider->setCallback(
                [viewportWeak](float value)
                {
                    if (auto viewport = viewportWeak.lock())
                    {
                        viewport->setPointSize(value);
                    }
                });
            displayLayout->addRow("Point size:", pointSizeSlider);
            auto displayBellows = Bellows::create(context, "Display", layout);
            displayBellows->setWidget(displayLayout);
            displayBellows->setOpen(true);

            _scrollWidget = ScrollWidget::create(
                context,
                ScrollType::Both,
                shared_from_this());
            _scrollWidget->setWidget(layout);
        }

        ParametersPanel::~ParametersPanel()
        {}

        std::shared_ptr<ParametersPanel> ParametersPanel::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<Viewport>& viewport,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ParametersPanel>(new ParametersPanel);
            out->_init(context, model, viewport, parent);
            return out;
        }

        Size2I ParametersPanel::getSizeHint() const
        {
            return _scrollWidget->getSizeHint();
        }

        void ParametersPanel::setGeometry(const Box2I& value)
        {
            IWidget::setGeometry(value);
            _scrollWidget->setGeometry(value);
        }
    }
}
