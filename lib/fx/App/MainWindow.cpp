// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/MainWindow.h>

#include <fx/App/App.h>
#include <fx/App/ParametersPanel.h>
#include <fx/App/SceneModel.h>
#include <fx/App/TimelineBar.h>
#include <fx/App/Viewport.h>
#include <fx/App/Views.h>

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

            _views = Views::create(context, model);
            _parametersPanel = ParametersPanel::create(context, model, _views);
            setScreenshotTag(_parametersPanel, "MainWindow.Parameters");

            _splitter = Splitter::create(context, Orientation::Horizontal);
            _splitter->setSplit(.78F);
            _views->setParent(_splitter);
            _parametersPanel->setParent(_splitter);

            auto layout = VerticalLayout::create(context);
            layout->setSpacingRole(SizeRole::None);
            _splitter->setParent(layout);
            Divider::create(context, Orientation::Vertical, layout);
            _timelineBar = TimelineBar::create(context, model, layout);
            setScreenshotTag(_timelineBar, "MainWindow.Timeline");
            setWidget(layout);

            _createViewMenu(context);
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

        const std::shared_ptr<Views>& MainWindow::getViews() const
        {
            return _views;
        }

        void MainWindow::_createViewMenu(const std::shared_ptr<Context>& context)
        {
            // Added to the menu bar the base window already made, rather than
            // replacing it, so the File and Window menus it provides stay.
            auto menu = getMenuBar()->addMenu("View");

            std::weak_ptr<Views> viewsWeak(_views);
            const std::vector<KeyShortcut> shortcuts =
            {
                KeyShortcut(Key::_1, commandKeyModifier),
                KeyShortcut(Key::_2, commandKeyModifier),
                KeyShortcut(Key::_3, commandKeyModifier),
                KeyShortcut(Key::_4, commandKeyModifier)
            };
            const auto labels = getViewLayoutLabels();
            for (size_t i = 0; i < labels.size(); ++i)
            {
                const ViewLayout layout = static_cast<ViewLayout>(i);
                auto action = Action::create(
                    labels[i],
                    shortcuts[i],
                    [viewsWeak, layout](bool value)
                    {
                        // Only ever switching to an arrangement. Unchecking the
                        // current one would leave no arrangement at all, so the
                        // observer below puts the tick straight back.
                        if (value)
                        {
                            if (auto views = viewsWeak.lock())
                            {
                                views->setLayout(layout);
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
                [viewsWeak]
                {
                    if (auto views = viewsWeak.lock())
                    {
                        views->getCurrent()->frameView();
                    }
                }));
            menu->addAction(Action::create(
                "Zoom In",
                "ViewZoomIn",
                KeyShortcut(Key::Equals, commandKeyModifier),
                [viewsWeak]
                {
                    if (auto views = viewsWeak.lock())
                    {
                        views->getCurrent()->zoomIn();
                    }
                }));
            menu->addAction(Action::create(
                "Zoom Out",
                "ViewZoomOut",
                KeyShortcut(Key::Minus, commandKeyModifier),
                [viewsWeak]
                {
                    if (auto views = viewsWeak.lock())
                    {
                        views->getCurrent()->zoomOut();
                    }
                }));

            _layoutObserver = Observer<ViewLayout>::create(
                _views->observeLayout(),
                [this](ViewLayout value)
                {
                    for (const auto& i : _layoutActions)
                    {
                        i.second->setChecked(i.first == value);
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
