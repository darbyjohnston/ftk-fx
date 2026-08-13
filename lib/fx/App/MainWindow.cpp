// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/MainWindow.h>

#include <fx/App/App.h>
#include <fx/App/Panels.h>
#include <fx/App/SceneModel.h>
#include <fx/App/TimelineBar.h>
#include <fx/App/Editor.h>
#include <fx/App/Viewport.h>
#include <fx/App/Editors.h>

#include <ftk/UI/ActionGroup.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/Spacer.h>
#include <ftk/UI/FileBrowser.h>
#include <ftk/UI/Menu.h>
#include <ftk/UI/MenuBar.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Splitter.h>
#include <ftk/UI/ToolBar.h>

#include <ftk/Core/Format.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void MainWindow::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<SceneModel>& model,
            const Size2I& size)
        {
            ftk::MainWindow::_init(context, app, size);
            _model = model;

            _editors = Editors::create(context, model);
            _panels = Panels::create(context, model, _editors);
            setScreenshotTag(_panels, "MainWindow.Panels");

            _splitter = Splitter::create(context, Orientation::Horizontal);
            _splitter->setSplit(.66F);
            _editors->setParent(_splitter);
            _panels->setParent(_splitter);

            _layout = VerticalLayout::create(context);
            _layout->setSpacingRole(SizeRole::None);
            _splitter->setParent(_layout);
            Divider::create(context, Orientation::Vertical, _layout);
            _timelineBar = TimelineBar::create(context, model, _layout);
            setScreenshotTag(_timelineBar, "MainWindow.Timeline");
            setWidget(_layout);

            _createFileMenu(context, app);
            _createEditMenu(context);
            _createLayoutMenu(context);
            _createCameraMenu(context);
            _createPanelsMenu(context);
            _createTransformMenu(context);
            _createToolBar(context);

            _pathObserver = Observer<std::filesystem::path>::create(
                model->observePath(),
                [this](const std::filesystem::path&) { _titleUpdate(); });
            _modifiedObserver = Observer<bool>::create(
                model->observeModified(),
                [this](bool) { _titleUpdate(); });
        }

        MainWindow::~MainWindow()
        {}

        std::shared_ptr<MainWindow> MainWindow::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<SceneModel>& model,
            const Size2I& size)
        {
            auto out = std::shared_ptr<MainWindow>(new MainWindow);
            out->_init(context, app, model, size);
            return out;
        }

        const std::shared_ptr<Editors>& MainWindow::getEditors() const
        {
            return _editors;
        }

        const std::shared_ptr<Panels>& MainWindow::getPanels() const
        {
            return _panels;
        }

        void MainWindow::setSplit(float value)
        {
            _splitter->setSplit(value);
        }


        namespace
        {
            const std::string extension = ".fx";
            const std::vector<std::string> extensions = { extension };
        }

        void MainWindow::_createFileMenu(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<App>& app)
        {
            // The framework's File menu holds Exit and nothing else. Replaced
            // rather than appended to, so the file actions come before Exit
            // instead of after it, and put back where it was so File is still
            // the first menu.
            auto menu = Menu::create(context);
            std::weak_ptr<SceneModel> weak(_model);
            std::weak_ptr<MainWindow> windowWeak(
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));

            _newAction = Action::create(
                "New",
                "FileNew",
                KeyShortcut(Key::N, commandKeyModifier),
                [weak]
                {
                    if (auto model = weak.lock())
                    {
                        model->newScene();
                    }
                });
            _newAction->setTooltip("Start again from an empty scene");
            menu->addAction(_newAction);

            _openAction = Action::create(
                "Open",
                "FileOpen",
                KeyShortcut(Key::O, commandKeyModifier),
                [this, weak, context]
                {
                    ftk::FileBrowserOpenOptions options;
                    options.title = "Open Scene";
                    options.extensions = extensions;
                    options.extensionsLabel = "Scenes";
                    context->getSystem<ftk::FileBrowserSystem>()->open(
                        std::dynamic_pointer_cast<IWindow>(shared_from_this()),
                        [this, weak](const ftk::Path& path)
                        {
                            if (auto model = weak.lock())
                            {
                                try
                                {
                                    model->open(path.get());
                                }
                                catch (const std::exception& e)
                                {
                                    _error(e.what());
                                }
                            }
                        },
                        options);
                });
            _openAction->setTooltip("Open a scene");
            menu->addAction(_openAction);

            _saveAction = Action::create(
                "Save",
                "FileSave",
                KeyShortcut(Key::S, commandKeyModifier),
                [this, weak]
                {
                    auto model = weak.lock();
                    if (!model)
                        return;
                    // A scene that has never been saved has nowhere to save
                    // to, so Save becomes Save As the first time.
                    if (model->getPath().empty())
                    {
                        _saveAs();
                        return;
                    }
                    try
                    {
                        model->save(model->getPath());
                    }
                    catch (const std::exception& e)
                    {
                        _error(e.what());
                    }
                });
            _saveAction->setTooltip("Save the scene");
            menu->addAction(_saveAction);

            menu->addAction(Action::create(
                "Save As",
                KeyShortcut(
                    Key::S,
                    static_cast<int>(commandKeyModifier) |
                    static_cast<int>(KeyModifier::Shift)),
                [this] { _saveAs(); }));

            menu->addDivider();

            std::weak_ptr<App> appWeak(app);
            menu->addAction(Action::create(
                "Exit",
                KeyShortcut(Key::Q, commandKeyModifier),
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->exit();
                    }
                }));

            getMenuBar()->removeMenu("File");
            getMenuBar()->insertMenu(0, "File", menu);

            _titleUpdate();
        }

        void MainWindow::_saveAs()
        {
            auto context = getContext();
            auto model = _model.lock();
            if (!context || !model)
                return;
            ftk::FileBrowserOpenOptions options;
            options.title = "Save Scene";
            options.mode = ftk::FileBrowserMode::Save;
            options.extensions = extensions;
            options.extensionsLabel = "Scenes";
            std::weak_ptr<SceneModel> weak(model);
            context->getSystem<ftk::FileBrowserSystem>()->open(
                std::dynamic_pointer_cast<IWindow>(shared_from_this()),
                [this, weak](const ftk::Path& value)
                {
                    auto model = weak.lock();
                    if (!model)
                        return;
                    // Typing a name without an extension is the common case,
                    // and a scene called "smoke" that will not appear in the
                    // browser's own filter is not helpful.
                    std::filesystem::path path = value.get();
                    if (path.extension().empty())
                    {
                        path += extension;
                    }
                    try
                    {
                        model->save(path);
                    }
                    catch (const std::exception& e)
                    {
                        _error(e.what());
                    }
                },
                options);
        }

        void MainWindow::_error(const std::string& what)
        {
            if (auto context = getContext())
            {
                context->getSystem<ftk::DialogSystem>()->message(
                    "Error",
                    what,
                    std::dynamic_pointer_cast<IWindow>(shared_from_this()));
                context->log("fx::app::MainWindow", what, LogType::Error);
            }
        }

        void MainWindow::_titleUpdate()
        {
            auto model = _model.lock();
            if (!model)
                return;
            const std::filesystem::path& path = model->getPath();
            const std::string name = path.empty() ?
                std::string("Untitled") :
                path.filename().string();
            setTitle(ftk::Format("{0}{1} - ftk-fx").
                arg(name).
                arg(model->isModified() ? "*" : ""));
        }

        void MainWindow::_createEditMenu(const std::shared_ptr<Context>& context)
        {
            auto menu = Menu::create(context);
            std::weak_ptr<SceneModel> weak(_model);

            _undoAction = Action::create(
                "Undo",
                "Undo",
                KeyShortcut(Key::Z, commandKeyModifier),
                [weak] { if (auto model = weak.lock()) model->undo(); });
            _undoAction->setTooltip("Undo the last edit");
            menu->addAction(_undoAction);

            _redoAction = Action::create(
                "Redo",
                "Redo",
                KeyShortcut(
                    Key::Z,
                    static_cast<int>(commandKeyModifier) |
                    static_cast<int>(KeyModifier::Shift)),
                [weak] { if (auto model = weak.lock()) model->redo(); });
            _redoAction->setTooltip("Redo the last undone edit");
            menu->addAction(_redoAction);

            // Beside File rather than after Panels: an artist looking for undo
            // looks in the second menu.
            getMenuBar()->insertMenu(1, "Edit", menu);

            auto model = _model.lock();
            if (!model)
                return;
            _hasUndoObserver = Observer<bool>::create(
                model->observeHasUndo(),
                [this](bool value) { _undoAction->setEnabled(value); });
            _hasRedoObserver = Observer<bool>::create(
                model->observeHasRedo(),
                [this](bool value) { _redoAction->setEnabled(value); });
        }

        void MainWindow::_createTransformMenu(const std::shared_ptr<Context>& context)
        {
            // Its own menu, next to Camera: one moves the thing and the other
            // moves the eye, which is the distinction an artist already has.
            //
            // On the menu bar rather than in the viewports, even though the
            // keys are where a DCC user's hand already is. The mode is one
            // for the application, so there is no viewport for a menu item to
            // have to name -- and the window only dispatches shortcuts
            // through its own menu bar, so a key needs an action here to hang
            // off.
            auto menu = Menu::create(context);
            getMenuBar()->insertMenu(4, "Transform", menu);

            std::weak_ptr<SceneModel> weak(_model);
            const std::vector<KeyShortcut> shortcuts =
            {
                KeyShortcut(Key::W),
                KeyShortcut(Key::E),
                KeyShortcut(Key::R)
            };
            const std::vector<std::string> tooltips =
            {
                "Move the current system",
                "Rotate the current system",
                "Scale the current system"
            };
            const std::vector<std::string> icons =
            {
                "Translate", "Rotate", "Scale"
            };
            _gizmoGroup = ActionGroup::create(ActionGroupType::Radio);
            const auto labels = getGizmoModeLabels();
            for (size_t i = 0; i < labels.size(); ++i)
            {
                const GizmoMode mode = static_cast<GizmoMode>(i);
                // No callback of its own. An action in a radio group is
                // checkable, and a checkable action reached by its shortcut
                // gets its *checked* callback, while one reached by a click
                // gets its plain one -- so a plain callback here worked from
                // the tool bar and did nothing from the keyboard. The group
                // watches the checked state, which both paths go through.
                auto action = Action::create(
                    labels[i],
                    icons[i],
                    shortcuts[i],
                    std::function<void(void)>());
                action->setTooltip(tooltips[i]);
                _gizmoActions[mode] = action;
                _gizmoGroup->addAction(action);
                menu->addAction(action);
            }

            _gizmoGroup->setCheckedCallback(
                [weak](int index, bool value)
                {
                    if (!value)
                        return;
                    if (auto model = weak.lock())
                    {
                        model->setGizmoMode(static_cast<GizmoMode>(index));
                    }
                });

            auto model = _model.lock();
            if (!model)
                return;
            _gizmoModeObserver = Observer<GizmoMode>::create(
                model->observeGizmoMode(),
                [this](GizmoMode value)
                {
                    _gizmoGroup->setChecked(static_cast<int>(value));
                });
        }

        void MainWindow::_createCameraMenu(const std::shared_ptr<Context>& context)
        {
            // Its own menu rather than part of Layout. Layout is how the
            // editors are arranged; these move the camera in one of them, which
            // is a different question that happened to share a menu because
            // that menu used to be called View.
            //
            // Not in the editors' own menus, where a 3D application would
            // normally put them and where "which editor?" would answer itself:
            // the window dispatches shortcuts through its own menu bar only,
            // so an action in an editor's menu would have no key attached to
            // it.
            auto menu = Menu::create(context);
            getMenuBar()->insertMenu(3, "Camera", menu);
            std::weak_ptr<Editors> editorsWeak(_editors);

            _frameAction = Action::create(
                "Frame",
                "ViewFrame",
                Key::Backspace,
                [editorsWeak]
                {
                    if (auto editors = editorsWeak.lock())
                    {
                        // Only when the current editor is showing a viewport.
                        // There is no camera to frame in a spreadsheet.
                        if (auto viewport = editors->getCurrent()->getViewport())
                        {
                            viewport->frameView();
                        }
                    }
                });
            _frameAction->setTooltip("Fit the current view to the particles");
            menu->addAction(_frameAction);

            menu->addAction(Action::create(
                "Frame All",
                KeyShortcut(Key::Backspace, commandKeyModifier),
                [editorsWeak]
                {
                    if (auto editors = editorsWeak.lock())
                    {
                        const int count = getEditorCount(editors->getLayout());
                        for (int i = 0; i < count; ++i)
                        {
                            if (auto viewport = editors->getEditor(i)->getViewport())
                            {
                                viewport->frameView();
                            }
                        }
                    }
                }));
            menu->addDivider();

            _zoomInAction = Action::create(
                "Zoom In",
                "ViewZoomIn",
                KeyShortcut(Key::Equals, commandKeyModifier),
                [editorsWeak]
                {
                    if (auto editors = editorsWeak.lock())
                    {
                        // Only when the current editor is showing a viewport.
                        // There is nothing to zoom in a spreadsheet.
                        if (auto viewport = editors->getCurrent()->getViewport())
                        {
                            viewport->zoomIn();
                        }
                    }
                });
            _zoomInAction->setTooltip("Zoom the current view in");
            menu->addAction(_zoomInAction);

            _zoomOutAction = Action::create(
                "Zoom Out",
                "ViewZoomOut",
                KeyShortcut(Key::Minus, commandKeyModifier),
                [editorsWeak]
                {
                    if (auto editors = editorsWeak.lock())
                    {
                        // Only when the current editor is showing a viewport.
                        // There is nothing to zoom in a spreadsheet.
                        if (auto viewport = editors->getCurrent()->getViewport())
                        {
                            viewport->zoomOut();
                        }
                    }
                });
            _zoomOutAction->setTooltip("Zoom the current view out");
            menu->addAction(_zoomOutAction);
        }

        void MainWindow::_createToolBar(const std::shared_ptr<Context>& context)
        {
            // The same Action objects the menus hold, so an icon, a tooltip or
            // an enabled state is written once: undo greys out in both places
            // because there is only one place.
            auto toolBar = ToolBar::create(context);
            // Off the edges. Without it the first and last buttons sit hard
            // against the window frame and the divider above the editors,
            // which reads as the toolbar having been cropped rather than
            // laid out.
            toolBar->setMarginRole(SizeRole::MarginInside);
            // addWidget rather than parenting to the tool bar: it is a
            // container that manages one widget, and anything else parented to
            // it is never given a geometry.
            // A divider with room either side of it. On its own it reads as
            // one more thing in the row rather than as a gap between groups:
            // the eye needs the space more than it needs the line.
            const auto divide = [context, toolBar]
            {
                auto before = Spacer::create(context, Orientation::Horizontal);
                before->setSpacingRole(SizeRole::Spacing);
                toolBar->addWidget(before);
                toolBar->addWidget(
                    Divider::create(context, Orientation::Horizontal));
                auto after = Spacer::create(context, Orientation::Horizontal);
                after->setSpacingRole(SizeRole::Spacing);
                toolBar->addWidget(after);
            };
            toolBar->addAction(_newAction);
            toolBar->addAction(_openAction);
            toolBar->addAction(_saveAction);
            divide();
            toolBar->addAction(_undoAction);
            toolBar->addAction(_redoAction);
            divide();
            for (size_t i = 0; i < static_cast<size_t>(EditorLayout::Count); ++i)
            {
                toolBar->addAction(_layoutActions[static_cast<EditorLayout>(i)]);
            }
            divide();
            for (size_t i = 0; i < static_cast<size_t>(GizmoMode::Count); ++i)
            {
                toolBar->addAction(
                    _gizmoActions[static_cast<GizmoMode>(i)]);
            }
            divide();
            toolBar->addAction(_frameAction);
            toolBar->addAction(_zoomInAction);
            toolBar->addAction(_zoomOutAction);
            setScreenshotTag(toolBar, "MainWindow.ToolBar");

            // Built after the menus, because the actions come from them, and
            // then moved to the top of the window where a tool bar goes.
            auto divider = Divider::create(context, Orientation::Vertical, _layout);
            toolBar->setParent(_layout);
            _layout->moveToBack(divider);
            _layout->moveToBack(toolBar);
        }

        void MainWindow::_createLayoutMenu(const std::shared_ptr<Context>& context)
        {
            // Added to the menu bar the base window already made, rather than
            // replacing it, so the File and Window menus it provides stay.
            // Inserted rather than appended: the framework's Window menu is
            // added second and belongs at the end, not in the middle of the
            // application's own.
            auto menu = Menu::create(context);
            getMenuBar()->insertMenu(2, "Layout", menu);

            std::weak_ptr<Editors> editorsWeak(_editors);
            const std::vector<KeyShortcut> shortcuts =
            {
                KeyShortcut(Key::_1, commandKeyModifier),
                KeyShortcut(Key::_2, commandKeyModifier),
                KeyShortcut(Key::_3, commandKeyModifier),
                KeyShortcut(Key::_4, commandKeyModifier)
            };
            _layoutGroup = ActionGroup::create(ActionGroupType::Radio);
            const auto labels = getEditorLayoutLabels();
            for (size_t i = 0; i < labels.size(); ++i)
            {
                const EditorLayout layout = static_cast<EditorLayout>(i);
                const std::vector<std::string> icons =
                {
                    "LayoutSingle", "LayoutTwo", "LayoutThree", "LayoutFour"
                };
                // One of four. The group keeps them exclusive, draws the tick
                // and stops the current one being un-picked.
                // As with the transform modes: a radio action reached by
                // its shortcut runs its checked callback, not this one, so
                // the group's callback below is what does the work.
                auto action = Action::create(
                    labels[i],
                    icons[i],
                    shortcuts[i],
                    std::function<void(void)>());
                action->setTooltip(labels[i] + " viewport layout");
                _layoutActions[layout] = action;
                _layoutGroup->addAction(action);
                menu->addAction(action);
            }

            _layoutGroup->setCheckedCallback(
                [editorsWeak](int index, bool value)
                {
                    if (!value)
                        return;
                    if (auto editors = editorsWeak.lock())
                    {
                        editors->setLayout(static_cast<EditorLayout>(index));
                    }
                });

            _layoutObserver = Observer<EditorLayout>::create(
                _editors->observeLayout(),
                [this](EditorLayout value)
                {
                    _layoutGroup->setChecked(static_cast<int>(value));
                });
        }

        void MainWindow::_createPanelsMenu(const std::shared_ptr<Context>& context)
        {
            auto menu = Menu::create(context);
            getMenuBar()->insertMenu(4, "Panels", menu);
            std::weak_ptr<Panels> panelsWeak(_panels);
            for (const auto& name : _panels->getPanelNames())
            {
                auto action = Action::create(
                    name,
                    [panelsWeak, name](bool value)
                    {
                        if (auto panels = panelsWeak.lock())
                        {
                            panels->setOpen(name, value);
                        }
                    });
                _panelActions[name] = action;
                menu->addAction(action);
            }

            menu->addDivider();

            _panelTabsAction = Action::create(
                "Tabs",
                [panelsWeak](bool value)
                {
                    if (auto panels = panelsWeak.lock())
                    {
                        panels->setStyle(
                            value ? PanelStyle::Tabs : PanelStyle::Column);
                    }
                });
            _panelTabsAction->setTooltip(
                "Show one panel at a time with a tab bar, rather than stacked");
            menu->addAction(_panelTabsAction);

            _panelStyleObserver = Observer<PanelStyle>::create(
                _panels->observeStyle(),
                [this](PanelStyle value)
                {
                    _panelTabsAction->setChecked(PanelStyle::Tabs == value);
                });

            // The panel's own close button and the tab bar's change the same
            // list, so the ticks follow what is open rather than what was last
            // picked from here.
            _openPanelsObserver = ListObserver<std::string>::create(
                _panels->observeOpen(),
                [this](const std::vector<std::string>& value)
                {
                    for (const auto& i : _panelActions)
                    {
                        i.second->setChecked(
                            std::find(value.begin(), value.end(), i.first) !=
                            value.end());
                    }
                });
        }

        void MainWindow::keyPressEvent(KeyEvent& event)
        {
            // The transport keys, on the window rather than on the timeline, so
            // they work wherever the focus is.
            auto model = _model.lock();
            if (model && 0 == event.modifiers)
            {
                switch (event.key)
                {
                case Key::Space:
                    event.accept = true;
                    model->setPlaying(!model->isPlaying());
                    return;
                case Key::Left:
                    event.accept = true;
                    model->framePrev();
                    return;
                case Key::Right:
                    event.accept = true;
                    model->frameNext();
                    return;
                case Key::Home:
                    event.accept = true;
                    model->frameStart();
                    return;
                case Key::End:
                    event.accept = true;
                    model->frameEnd();
                    return;
                default: break;
                }
            }
            ftk::MainWindow::keyPressEvent(event);
        }
    }
}
