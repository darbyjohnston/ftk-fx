// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/IPanel.h>

#include <fx/Core/Parameter.h>

namespace ftk
{
    class FormLayout;
}

namespace fx
{
    namespace app
    {
        class SceneModel;
        class Views;

        //! The parameter panel.
        class ParametersPanel : public IPanel
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<Views>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            ParametersPanel() = default;

        public:
            virtual ~ParametersPanel();

            static std::shared_ptr<ParametersPanel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<Views>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

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
        };
    }
}
