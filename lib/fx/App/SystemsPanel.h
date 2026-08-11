// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/IPanel.h>

#include <ftk/Core/Observable.h>

#include <vector>

namespace ftk
{
    class ButtonGroup;
    class CheckBox;
    class ToolButton;
    class VerticalLayout;
}

namespace fx
{
    namespace app
    {
        class SceneModel;

        //! The systems panel: what is in the scene, and which of it is being
        //! edited.
        //!
        //! The list is the selection. Every other panel shows the current
        //! system, so choosing one here is what points the parameter panel and
        //! the curve editor at something -- which is why this is a panel in the
        //! column rather than an editor: it is wanted at the same time as the
        //! things it drives.
        class SystemsPanel : public IPanel
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            SystemsPanel() = default;

        public:
            virtual ~SystemsPanel();

            static std::shared_ptr<SystemsPanel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

        private:
            //! Build a row per system. Only when the systems themselves have
            //! changed; picking a different one moves a highlight and does not
            //! need new widgets.
            void _rowsUpdate();

            //! Show which system is current, and which are enabled.
            void _valuesUpdate();

            struct Row
            {
                std::shared_ptr<ftk::CheckBox> enabledCheckBox;
                std::shared_ptr<ftk::ToolButton> nameButton;
            };

            std::weak_ptr<SceneModel> _model;
            std::vector<Row> _rows;
            std::shared_ptr<ftk::VerticalLayout> _listLayout;
            std::shared_ptr<ftk::ButtonGroup> _nameGroup;
            std::shared_ptr<ftk::ToolButton> _removeButton;

            //! What the rows were built from, which is what decides whether
            //! they have to be built again.
            std::vector<std::string> _names;

            //! Set while the panel is writing its own widgets, so that the
            //! callbacks those writes fire do not read back as edits.
            bool _updating = false;

            std::shared_ptr<ftk::Observer<int> > _sceneObserver;
            std::shared_ptr<ftk::Observer<int> > _parameterObserver;
            std::shared_ptr<ftk::Observer<size_t> > _currentSystemObserver;
        };
    }
}
