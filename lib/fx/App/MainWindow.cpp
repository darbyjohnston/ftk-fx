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
#include <ftk/UI/Divider.h>
#include <ftk/UI/Menu.h>
#include <ftk/UI/MenuBar.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Splitter.h>

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
            _splitter->setSplit(.78F);
            _panes->setParent(_splitter);
            _panels->setParent(_splitter);

            auto layout = VerticalLayout::create(context);
            layout->setSpacingRole(SizeRole::None);
            _splitter->setParent(layout);
            Divider::create(context, Orientation::Vertical, layout);
            _timelineBar = TimelineBar::create(context, model, layout);
            setScreenshotTag(_timelineBar, "MainWindow.Timeline");
            setWidget(layout);

            _createLayoutMenu(context);
            _createPanelsMenu(context);
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
            _cursorPos(to);
            _mouseButton(MouseButton::Left, false, modifiers);
        }

        void MainWindow::_createLayoutMenu(const std::shared_ptr<Context>& context)
        {
            // Added to the menu bar the base window already made, rather than
            // replacing it, so the File and Window menus it provides stay.
            auto menu = getMenuBar()->addMenu("Layout");

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

            menu->addDivider();

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
                "Zoom In",
                "ViewZoomIn",
                KeyShortcut(Key::Equals, commandKeyModifier),
                [panesWeak]
                {
                    if (auto panes = panesWeak.lock())
                    {
                        // Only when the current pane is showing a viewport.
                        // There is no camera to frame in a spreadsheet.
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
                        // There is no camera to frame in a spreadsheet.
                        if (auto viewport = panes->getCurrent()->getViewport())
                        {
                            viewport->zoomOut();
                        }
                    }
                }));

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
            auto menu = getMenuBar()->addMenu("Panels");
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
