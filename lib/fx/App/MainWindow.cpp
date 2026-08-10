// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/MainWindow.h>

#include <fx/App/App.h>
#include <fx/App/Panels.h>
#include <fx/App/SceneModel.h>
#include <fx/App/TimelineBar.h>
#include <fx/App/Pane.h>
#include <fx/App/Viewport.h>
#include <fx/App/Panes.h>

#include <ftk/UI/Action.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/FileBrowser.h>
#include <ftk/UI/Menu.h>
#include <ftk/UI/MenuBar.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Splitter.h>

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

            _panes = Panes::create(context, model);
            _panels = Panels::create(context, model, _panes);
            setScreenshotTag(_panels, "MainWindow.Panels");

            _splitter = Splitter::create(context, Orientation::Horizontal);
            _splitter->setSplit(.66F);
            _panes->setParent(_splitter);
            _panels->setParent(_splitter);

            auto layout = VerticalLayout::create(context);
            layout->setSpacingRole(SizeRole::None);
            _splitter->setParent(layout);
            Divider::create(context, Orientation::Vertical, layout);
            _timelineBar = TimelineBar::create(context, model, layout);
            setScreenshotTag(_timelineBar, "MainWindow.Timeline");
            setWidget(layout);

            _createFileMenu(context, app);
            _createEditMenu(context);
            _createLayoutMenu(context);
            _createCameraMenu(context);
            _createPanelsMenu(context);

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

        const std::shared_ptr<Panes>& MainWindow::getPanes() const
        {
            return _panes;
        }

        const std::shared_ptr<Panels>& MainWindow::getPanels() const
        {
            return _panels;
        }

        void MainWindow::setSplit(float value)
        {
            _splitter->setSplit(value);
        }

        void MainWindow::click(const V2I& pos, int modifiers)
        {
            _cursorEnter(true);
            _cursorPos(pos);
            _mouseButton(MouseButton::Left, true, modifiers);
            _mouseButton(MouseButton::Left, false, modifiers);
        }

        void MainWindow::drag(const V2I& from, const V2I& to, int modifiers)
        {
            _cursorEnter(true);
            _cursorPos(from);
            _mouseButton(MouseButton::Left, true, modifiers);
            // Moved in steps rather than jumped. A widget that treats a drag
            // as one gesture and a widget that treats every move as a separate
            // edit look the same from one event, and the difference is the
            // whole question for undo.
            const int steps = 8;
            for (int i = 1; i <= steps; ++i)
            {
                _cursorPos(V2I(
                    from.x + (to.x - from.x) * i / steps,
                    from.y + (to.y - from.y) * i / steps));
            }
            _mouseButton(MouseButton::Left, false, modifiers);
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

            menu->addAction(Action::create(
                "New",
                KeyShortcut(Key::N, commandKeyModifier),
                [weak]
                {
                    if (auto model = weak.lock())
                    {
                        model->newScene();
                    }
                }));

            menu->addAction(Action::create(
                "Open",
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
                }));

            menu->addAction(Action::create(
                "Save",
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
                }));

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
                KeyShortcut(Key::Z, commandKeyModifier),
                [weak] { if (auto model = weak.lock()) model->undo(); });
            menu->addAction(_undoAction);

            _redoAction = Action::create(
                "Redo",
                KeyShortcut(
                    Key::Z,
                    static_cast<int>(commandKeyModifier) |
                    static_cast<int>(KeyModifier::Shift)),
                [weak] { if (auto model = weak.lock()) model->redo(); });
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

        void MainWindow::_createCameraMenu(const std::shared_ptr<Context>& context)
        {
            // Its own menu rather than part of Layout. Layout is how the panes
            // are arranged; these move the camera in one of them, which is a
            // different question that happened to share a menu because that
            // menu used to be called View.
            //
            // Not in the panes' own menus, where a 3D application would
            // normally put them and where "which pane?" would answer itself:
            // the window dispatches shortcuts through its own menu bar only,
            // so an action in a pane's menu would have no key attached to it.
            auto menu = Menu::create(context);
            getMenuBar()->insertMenu(3, "Camera", menu);
            std::weak_ptr<Panes> panesWeak(_panes);

            menu->addAction(Action::create(
                "Frame",
                "ViewFrame",
                Key::Backspace,
                [panesWeak]
                {
                    if (auto panes = panesWeak.lock())
                    {
                        // Only when the current pane is showing a viewport.
                        // There is no camera to frame in a spreadsheet.
                        if (auto viewport = panes->getCurrent()->getViewport())
                        {
                            viewport->frameView();
                        }
                    }
                }));
            menu->addAction(Action::create(
                "Frame All",
                KeyShortcut(Key::Backspace, commandKeyModifier),
                [panesWeak]
                {
                    if (auto panes = panesWeak.lock())
                    {
                        const int count = getPaneCount(panes->getLayout());
                        for (int i = 0; i < count; ++i)
                        {
                            if (auto viewport = panes->getPane(i)->getViewport())
                            {
                                viewport->frameView();
                            }
                        }
                    }
                }));
            menu->addDivider();

            menu->addAction(Action::create(
                "Zoom In",
                "ViewZoomIn",
                KeyShortcut(Key::Equals, commandKeyModifier),
                [panesWeak]
                {
                    if (auto panes = panesWeak.lock())
                    {
                        // Only when the current pane is showing a viewport.
                        // There is nothing to zoom in a spreadsheet.
                        if (auto viewport = panes->getCurrent()->getViewport())
                        {
                            viewport->zoomIn();
                        }
                    }
                }));
            menu->addAction(Action::create(
                "Zoom Out",
                "ViewZoomOut",
                KeyShortcut(Key::Minus, commandKeyModifier),
                [panesWeak]
                {
                    if (auto panes = panesWeak.lock())
                    {
                        // Only when the current pane is showing a viewport.
                        // There is nothing to zoom in a spreadsheet.
                        if (auto viewport = panes->getCurrent()->getViewport())
                        {
                            viewport->zoomOut();
                        }
                    }
                }));

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

            std::weak_ptr<Panes> panesWeak(_panes);
            const std::vector<KeyShortcut> shortcuts =
            {
                KeyShortcut(Key::_1, commandKeyModifier),
                KeyShortcut(Key::_2, commandKeyModifier),
                KeyShortcut(Key::_3, commandKeyModifier),
                KeyShortcut(Key::_4, commandKeyModifier)
            };
            const auto labels = getPaneLayoutLabels();
            for (size_t i = 0; i < labels.size(); ++i)
            {
                const PaneLayout layout = static_cast<PaneLayout>(i);
                auto action = Action::create(
                    labels[i],
                    shortcuts[i],
                    [panesWeak, layout](bool value)
                    {
                        // Only ever switching to an arrangement. Unchecking the
                        // current one would leave no arrangement at all, so the
                        // observer below puts the tick straight back.
                        if (value)
                        {
                            if (auto panes = panesWeak.lock())
                            {
                                panes->setLayout(layout);
                            }
                        }
                    });
                action->setTooltip(labels[i] + " viewport layout");
                _layoutActions[layout] = action;
                menu->addAction(action);
            }

            _layoutObserver = Observer<PaneLayout>::create(
                _panes->observeLayout(),
                [this](PaneLayout value)
                {
                    for (const auto& i : _layoutActions)
                    {
                        i.second->setChecked(i.first == value);
                    }
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
