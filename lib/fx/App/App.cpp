// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/App.h>

#include <fx/App/Capture.h>
#include <fx/App/MainWindow.h>
#include <fx/App/SceneModel.h>

#include <fx/Core/Version.h>

#include <ftk/Core/DiagSystem.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void App::_init(
            const std::shared_ptr<Context>& context,
            const std::vector<std::string>& argv)
        {
            _cmdLine.captureManifest = CmdLineOption<std::string>::create(
                { "-captureManifest" },
                "Screenshot manifest file name.",
                "Testing");
            _cmdLine.captureShot = CmdLineOption<std::string>::create(
                { "-captureShot" },
                "The identifier of the shot in the manifest to capture.",
                "Testing");
            _cmdLine.captureOutput = CmdLineOption<std::string>::create(
                { "-captureOutput" },
                "The directory to write the screenshot and its sidecar to.",
                "Testing");

            ftk::App::_init(
                context,
                argv,
                "ftk-fx",
                "An interactive particle FX application.",
                {},
                {
                    _cmdLine.captureManifest,
                    _cmdLine.captureShot,
                    _cmdLine.captureOutput
                },
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

        int App::getExitCode() const
        {
            return _exitCode;
        }

        void App::_diagInit()
        {
            // feather-tk already samples frame time, triangles, glyphs and
            // widget counts. These are the numbers on this side of it: what the
            // solve costs, and what the cache is holding.
            auto diagSystem = _context->getSystem<DiagSystem>();
            std::weak_ptr<SceneModel> weak(_sceneModel);
            diagSystem->addSampler(
                "fx Sim/Particles: {0}",
                [weak]
                {
                    auto model = weak.lock();
                    return model ?
                        static_cast<int64_t>(model->getParticleCount()) :
                        0;
                });
            diagSystem->addSampler(
                "fx Sim/Time: {0}ms",
                [weak]
                {
                    auto model = weak.lock();
                    return model ? model->getSimTime() : 0;
                });
            diagSystem->addSampler(
                "fx Cache/Memory: {0}MB",
                [weak]
                {
                    auto model = weak.lock();
                    return model ?
                        static_cast<int64_t>(
                            model->observeCacheByteCount()->get() / (1024 * 1024)) :
                        0;
                });
            diagSystem->addSampler(
                "fx Cache/Frames: {0}",
                [weak]
                {
                    auto model = weak.lock();
                    return model ?
                        static_cast<int64_t>(model->getCachedFrameCount()) :
                        0;
                });
        }

        void App::run()
        {
            _sceneModel = SceneModel::create(_context);
            _diagInit();
            _mainWindow = MainWindow::create(
                _context,
                std::dynamic_pointer_cast<App>(shared_from_this()),
                _sceneModel,
                Size2I(1280, 800));

            if (_cmdLine.captureManifest->hasValue())
            {
                _capture = Capture::create(
                    _context,
                    std::dynamic_pointer_cast<App>(shared_from_this()),
                    std::filesystem::u8path(_cmdLine.captureManifest->getValue()),
                    _cmdLine.captureShot->hasValue() ?
                        _cmdLine.captureShot->getValue() :
                        std::string(),
                    std::filesystem::u8path(
                        _cmdLine.captureOutput->hasValue() ?
                            _cmdLine.captureOutput->getValue() :
                            std::string(".")));
                if (!_capture->begin())
                {
                    _exitCode = 1;
                    return;
                }
            }

            ftk::App::run();

            if (_capture && !_capture->succeeded())
            {
                _exitCode = 1;
            }
        }
    }
}
