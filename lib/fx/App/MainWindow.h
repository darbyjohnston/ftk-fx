// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/ViewOptions.h>

#include <ftk/UI/MainWindow.h>

#include <map>

namespace ftk
{
    class Action;
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
        class Views;

        //! The main window: the viewports, the parameters beside them, and the
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

            const std::shared_ptr<Views>& getViews() const;

            void keyPressEvent(ftk::KeyEvent&) override;

        private:
            void _createViewMenu(const std::shared_ptr<ftk::Context>&);

            std::weak_ptr<SceneModel> _model;
            std::shared_ptr<Views> _views;
            std::shared_ptr<ParametersPanel> _parametersPanel;
            std::shared_ptr<TimelineBar> _timelineBar;
            std::shared_ptr<ftk::Splitter> _splitter;

            std::map<ViewLayout, std::shared_ptr<ftk::Action> > _layoutActions;
            std::shared_ptr<ftk::Observer<ViewLayout> > _layoutObserver;
        };
    }
}
