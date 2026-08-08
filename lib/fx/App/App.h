// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/UI/App.h>

#include <ftk/Core/CmdLine.h>

namespace fx
{
    namespace app
    {
        class Capture;
        class MainWindow;
        class SceneModel;

        //! Application.
        class App : public ftk::App
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::vector<std::string>& argv);

            App() = default;

        public:
            ~App();

            static std::shared_ptr<App> create(
                const std::shared_ptr<ftk::Context>&,
                const std::vector<std::string>&);

            const std::shared_ptr<SceneModel>& getSceneModel() const;
            const std::shared_ptr<MainWindow>& getMainWindow() const;

            //! Get the process exit code. Non-zero when a screenshot capture
            //! was asked for and did not produce its outputs -- a capture that
            //! quietly failed would leave the documentation showing the last
            //! run's pictures.
            int getExitCode() const;

            void run() override;

        private:
            //! Command line options.
            //!
            //! Screenshots are described by a manifest rather than by an option
            //! per thing worth capturing: those options multiply, and they end
            //! up a second way of setting what the manifest already covers.
            struct CmdLine
            {
                std::shared_ptr<ftk::CmdLineOption<std::string> > captureManifest;
                std::shared_ptr<ftk::CmdLineOption<std::string> > captureShot;
                std::shared_ptr<ftk::CmdLineOption<std::string> > captureOutput;
            };
            CmdLine _cmdLine;

            std::shared_ptr<SceneModel> _sceneModel;
            std::shared_ptr<MainWindow> _mainWindow;
            std::shared_ptr<Capture> _capture;
            int _exitCode = 0;
        };
    }
}
