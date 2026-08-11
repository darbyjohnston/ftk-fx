// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/EditorOptions.h>
#include <fx/App/Panels.h>

#include <ftk/UI/MainWindow.h>

#include <filesystem>
#include <map>
#include <string>

namespace ftk
{
    class Action;
    class ActionGroup;
    class Splitter;
}

namespace fx
{
    namespace app
    {
        class App;
        class Panels;
        class SceneModel;
        class TimelineBar;
        class Editors;

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

            const std::shared_ptr<Editors>& getEditors() const;
            const std::shared_ptr<Panels>& getPanels() const;

            //! Set where the panels column starts, as a fraction of the width.
            void setSplit(float);

            //! Click at a position, the way a person would.
            //!
            //! For the screenshot harness. Everything else it can do drives the
            //! models directly, which is how a crash in the interface -- a
            //! callback that took apart the widget calling it -- passed every
            //! shot while the application fell over on the first click.
            void click(const ftk::V2I& pos, int modifiers = 0);

            //! Drag from one position to another, the way a person would.
            //! Press, move along the path, release. `release` false leaves
            //! the button down, which is how a shot catches what a gesture
            //! looks like while it is happening rather than after it.
            void drag(
                const std::vector<ftk::V2I>& path,
                int modifiers = 0,
                bool release = true);

            void keyPressEvent(ftk::KeyEvent&) override;

        private:
            void _createFileMenu(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);
            void _createEditMenu(const std::shared_ptr<ftk::Context>&);
            void _createLayoutMenu(const std::shared_ptr<ftk::Context>&);
            void _createCameraMenu(const std::shared_ptr<ftk::Context>&);
            void _createToolBar(const std::shared_ptr<ftk::Context>&);

            //! Ask for a path and save to it.
            void _saveAs();

            //! Report a failure to the artist rather than only to the log.
            void _error(const std::string&);

            //! Put the file's name and whether it is modified in the title
            //! bar, which is where everything else puts it.
            void _titleUpdate();
            void _createPanelsMenu(const std::shared_ptr<ftk::Context>&);

            std::weak_ptr<SceneModel> _model;
            std::shared_ptr<Editors> _editors;
            std::shared_ptr<Panels> _panels;
            std::shared_ptr<TimelineBar> _timelineBar;
            std::shared_ptr<ftk::Splitter> _splitter;
            std::shared_ptr<ftk::VerticalLayout> _layout;
            std::shared_ptr<ftk::Observer<std::filesystem::path> > _pathObserver;
            std::shared_ptr<ftk::Observer<bool> > _modifiedObserver;
            //! The actions the tool bar shares with the menus, so that one
            //! object carries the icon, the tooltip and the enabled state and
            //! the two cannot disagree.
            std::shared_ptr<ftk::Action> _newAction;
            std::shared_ptr<ftk::Action> _openAction;
            std::shared_ptr<ftk::Action> _saveAction;
            std::shared_ptr<ftk::Action> _frameAction;
            std::shared_ptr<ftk::Action> _zoomInAction;
            std::shared_ptr<ftk::Action> _zoomOutAction;
            std::shared_ptr<ftk::Action> _undoAction;
            std::shared_ptr<ftk::Action> _redoAction;
            std::shared_ptr<ftk::Observer<bool> > _hasUndoObserver;
            std::shared_ptr<ftk::Observer<bool> > _hasRedoObserver;

            std::map<EditorLayout, std::shared_ptr<ftk::Action> > _layoutActions;
            std::shared_ptr<ftk::ActionGroup> _layoutGroup;
            std::shared_ptr<ftk::Observer<EditorLayout> > _layoutObserver;

            std::map<std::string, std::shared_ptr<ftk::Action> > _panelActions;
            std::shared_ptr<ftk::Action> _panelTabsAction;
            std::shared_ptr<ftk::ListObserver<std::string> > _openPanelsObserver;
            std::shared_ptr<ftk::Observer<PanelStyle> > _panelStyleObserver;
        };
    }
}
