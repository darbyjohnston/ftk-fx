// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/App.h>

#include <fx/App/MainWindow.h>
#include <fx/App/SceneModel.h>

#include <fx/Core/Version.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void App::_init(
            const std::shared_ptr<Context>& context,
            const std::vector<std::string>& argv)
        {
            _cmdLine.frame = CmdLineOption<int>::create(
                { "-frame" },
                "Start on this frame.",
                "Testing Options");

            ftk::App::_init(
                context,
                argv,
                "ftk-fx",
                "An interactive particle FX application.",
                {},
                { _cmdLine.frame },
                AppFiles{ "ftk-fx", "ftk-fx" });
        }

        App::~App()
        {}

        std::shared_ptr<App> App::create(
            const std::shared_ptr<Context>& context,
            const std::vector<std::string>& argv)
        {
            auto out = std::shared_ptr<App>(new App);
            out->_init(context, argv);
            return out;
        }

        const std::shared_ptr<SceneModel>& App::getSceneModel() const
        {
            return _sceneModel;
        }

        const std::shared_ptr<MainWindow>& App::getMainWindow() const
        {
            return _mainWindow;
        }

        void App::run()
        {
            _sceneModel = SceneModel::create(_context);
            if (_cmdLine.frame->hasValue())
            {
                _sceneModel->setCurrentFrame(_cmdLine.frame->getValue());
            }
            _mainWindow = MainWindow::create(
                _context,
                std::dynamic_pointer_cast<App>(shared_from_this()),
                _sceneModel,
                Size2I(1280, 800));
            ftk::App::run();
        }
    }
}
