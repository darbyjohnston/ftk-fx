// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Parameter.h>

#include <ftk/UI/IWidget.h>

namespace ftk
{
    class FormLayout;
    class ScrollWidget;
}

namespace fx
{
    namespace app
    {
        class SceneModel;
        class Viewport;

        //! The parameter panel.
        class ParametersPanel : public ftk::IWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<Viewport>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            ParametersPanel() = default;

        public:
            virtual ~ParametersPanel();

            static std::shared_ptr<ParametersPanel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<Viewport>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            //! Add a slider driving a parameter's constant value.
            //!
            //! Only the constant: dragging a slider on an animated parameter
            //! should set a key, and there is no curve editor to set it with
            //! yet. Wiring that up here before the editor exists would be
            //! guessing at how the two talk to each other.
            void _addSlider(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::FormLayout>&,
                const std::string& label,
                core::Parameter&,
                float min,
                float max);

            std::weak_ptr<SceneModel> _model;
            std::shared_ptr<ftk::ScrollWidget> _scrollWidget;
        };
    }
}
