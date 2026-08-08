// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/UI/App.h>

#include <ftk/Core/CmdLine.h>

namespace fx
{
    namespace app
    {
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

            void run() override;

        private:
            //! Command line options.
            struct CmdLine
            {
                //! The frame to start on. There to make the application
                //! checkable without a person at the keyboard: a screenshot of
                //! frame one shows a handful of particles and proves nothing.
                std::shared_ptr<ftk::CmdLineOption<int> > frame;
            };
            CmdLine _cmdLine;

            std::shared_ptr<SceneModel> _sceneModel;
            std::shared_ptr<MainWindow> _mainWindow;
        };
    }
}
