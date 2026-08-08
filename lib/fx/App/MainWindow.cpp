// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/MainWindow.h>

#include <fx/App/App.h>
#include <fx/App/ParametersPanel.h>
#include <fx/App/SceneModel.h>
#include <fx/App/TimelineBar.h>
#include <fx/App/Viewport.h>

#include <ftk/UI/Divider.h>
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

            _viewport = Viewport::create(context, model);
            setScreenshotTag(_viewport, "MainWindow.Viewport");
            _parametersPanel = ParametersPanel::create(context, model, _viewport);
            setScreenshotTag(_parametersPanel, "MainWindow.Parameters");

            _splitter = Splitter::create(context, Orientation::Horizontal);
            _splitter->setSplit(.78F);
            _viewport->setParent(_splitter);
            _parametersPanel->setParent(_splitter);

            auto layout = VerticalLayout::create(context);
            layout->setSpacingRole(SizeRole::None);
            _splitter->setParent(layout);
            Divider::create(context, Orientation::Vertical, layout);
            _timelineBar = TimelineBar::create(context, model, layout);
            setScreenshotTag(_timelineBar, "MainWindow.Timeline");
            setWidget(layout);
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

        const std::shared_ptr<Viewport>& MainWindow::getViewport() const
        {
            return _viewport;
        }

        void MainWindow::keyPressEvent(KeyEvent& event)
        {
            // The transport keys, on the window rather than on the timeline, so
            // they work wherever the focus is. Menus and their shortcuts arrive
            // when there is something to put in a menu.
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
                case Key::Backspace:
                    event.accept = true;
                    _viewport->frameView();
                    return;
                default: break;
                }
            }
            ftk::MainWindow::keyPressEvent(event);
        }
    }
}
