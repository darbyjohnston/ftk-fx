// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/IPanel.h>
#include <fx/App/ParameterList.h>

#include <ftk/Core/Observable.h>

#include <map>

namespace ftk
{
    class CheckBox;
    class ComboBox;
    class FloatEditSlider;
    class FormLayout;
    class LineEdit;
    class ToolButton;
}

namespace fx
{
    namespace app
    {
        class SceneModel;
        class Panes;

        //! The parameter panel.
        class ParametersPanel : public IPanel
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<Panes>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            ParametersPanel() = default;

        public:
            virtual ~ParametersPanel();

            static std::shared_ptr<ParametersPanel> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<Panes>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

        private:
            //! One row: a slider for the value and a button that keys it.
            struct Row
            {
                ParameterInfo info;
                std::shared_ptr<ftk::FloatEditSlider> slider;
                std::shared_ptr<ftk::ToolButton> keyButton;
            };

            //! Build the controls for the current system.
            //!
            //! Called again when the selection changes, because the rows hold
            //! pointers into the system they were built from. Rebuilt rather
            //! than re-pointed: every system has the same parameters today,
            //! which would make re-pointing work, but that stops being true
            //! the moment a system can have two emitters.
            void _widgetsUpdate();

            void _addRow(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::FormLayout>&,
                const ParameterInfo&);

            //! Set a key at the playhead, or take the one that is there away.
            void _key(const ParameterInfo&, bool);

            //! Show what each parameter is worth at the playhead, and light
            //! the key button of anything keyed there.
            void _valuesUpdate();

            std::weak_ptr<SceneModel> _model;
            std::vector<Row> _rows;
            std::shared_ptr<ftk::ComboBox> _shapeComboBox;
            std::shared_ptr<ftk::CheckBox> _surfaceCheckBox;
            std::shared_ptr<ftk::ComboBox> _drawTypeComboBox;
            std::shared_ptr<ftk::FloatEditSlider> _particleSizeSlider;
            std::shared_ptr<ftk::LineEdit> _nameEdit;
            int _currentFrame = 1;

            //! Set while the panel is writing its own widgets, so that the
            //! callbacks those writes fire do not read back as edits.
            bool _updating = false;

            std::shared_ptr<ftk::Observer<int> > _currentFrameObserver;
            std::shared_ptr<ftk::Observer<int> > _sceneObserver;
            std::shared_ptr<ftk::Observer<int> > _parameterObserver;
            std::shared_ptr<ftk::Observer<float> > _particleSizeObserver;
            std::shared_ptr<ftk::Observer<size_t> > _currentSystemObserver;
        };
    }
}
