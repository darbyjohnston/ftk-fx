// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/UI/MainWindow.h>

namespace ftk
{
    class Splitter;
}

namespace fx
{
    namespace app
    {
        class App;
        class ParametersPanel;
        class SceneModel;
        class TimelineBar;
        class Viewport;

        //! The main window: the viewport, the parameters beside it, and the
        //! timeline under both.
        //!
        //! A fixed layout rather than dockable panels. That is a decision that
        //! does not have to be made, and a class of bug that does not exist.
        class MainWindow : public ftk::MainWindow
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<SceneModel>&,
                const ftk::Size2I&);

            MainWindow() = default;

        public:
            virtual ~MainWindow();

            static std::shared_ptr<MainWindow> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<SceneModel>&,
                const ftk::Size2I& = ftk::Size2I(1280, 800));

            const std::shared_ptr<Viewport>& getViewport() const;

            void keyPressEvent(ftk::KeyEvent&) override;

        private:
            std::weak_ptr<SceneModel> _model;
            std::shared_ptr<Viewport> _viewport;
            std::shared_ptr<ParametersPanel> _parametersPanel;
            std::shared_ptr<TimelineBar> _timelineBar;
            std::shared_ptr<ftk::Splitter> _splitter;
        };
    }
}
