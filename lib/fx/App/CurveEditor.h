// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/ParameterList.h>

#include <ftk/UI/IContainer.h>

#include <ftk/Core/Observable.h>

namespace ftk
{
    class ButtonGroup;
    class VerticalLayout;
}

namespace fx
{
    namespace app
    {
        class CurveGraph;
        class SceneModel;

        //! The curve editor: a list of what can be animated beside a plot of
        //! what is.
        class CurveEditor : public ftk::IContainer
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            CurveEditor() = default;

        public:
            virtual ~CurveEditor();

            static std::shared_ptr<CurveEditor> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

        private:
            //! Rebuild the channel list, which changes as parameters become
            //! animated and stop being animated.
            void _channelsUpdate();

            //! Hand the plot the channels that are animated and ticked.
            void _graphUpdate();

            std::weak_ptr<SceneModel> _model;
            std::shared_ptr<CurveGraph> _graph;
            std::shared_ptr<ftk::VerticalLayout> _channelLayout;
            std::vector<ParameterInfo> _channels;

            //! The channels the artist has ticked, by path so that they
            //! survive the list being rebuilt.
            std::vector<std::string> _shown;

            //! What was animated when the rows were last built, which is what
            //! decides whether they have to be built again.
            std::vector<std::string> _animated;

            std::shared_ptr<ftk::Observer<int> > _parameterObserver;
            std::shared_ptr<ftk::Observer<int> > _sceneObserver;
        };
    }
}
