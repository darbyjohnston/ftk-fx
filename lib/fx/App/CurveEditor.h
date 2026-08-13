// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/ParameterList.h>

#include <ftk/UI/IContainer.h>

#include <ftk/Core/Observable.h>

#include <string>
#include <vector>

namespace ftk
{
    class ButtonGroup;
    class ComboBox;
    class VerticalLayout;
}

namespace fx
{
    namespace app
    {
        //! How the plot scales its vertical axis.
        enum class CurveValueMode
        {
            //! One range for every channel, so their values can be compared
            //! against each other.
            Absolute,

            //! Each channel scaled to its own range, so a curve that lives
            //! between -30 and -2 is not a flat line under one that reaches
            //! 1800. Their shapes can be compared; their values cannot.
            Normalized,

            Count,
            First = Absolute
        };

        std::vector<std::string> getCurveValueModeLabels();

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

            //! Set how the plot scales its vertical axis.
            void setValueMode(CurveValueMode);

            //! The plot, for a capture that wants to aim at a key rather
            //! than at a pixel.
            const std::shared_ptr<CurveGraph>& getGraph() const;

        private:
            //! Rebuild the channel list, which changes as parameters become
            //! animated and stop being animated.
            void _channelsUpdate();

            //! Hand the plot the channels that are animated and ticked.
            void _graphUpdate();

            std::weak_ptr<SceneModel> _model;
            std::shared_ptr<CurveGraph> _graph;
            std::shared_ptr<ftk::VerticalLayout> _channelLayout;
            std::shared_ptr<ftk::ComboBox> _valueModeComboBox;
            std::vector<ParameterInfo> _channels;

            //! The channels the artist has ticked, by path so that they
            //! survive the list being rebuilt.
            std::vector<std::string> _shown;

            //! What was animated when the rows were last built, which is what
            //! decides whether they have to be built again.
            std::vector<std::string> _animated;

            //! What the plot was last given, so that it is not handed the same
            //! channels again: doing that drops whatever is selected.
            std::vector<std::string> _graphPaths;

            std::shared_ptr<ftk::Observer<int> > _parameterObserver;
            std::shared_ptr<ftk::Observer<int> > _sceneObserver;
            std::shared_ptr<ftk::Observer<size_t> > _currentSystemObserver;
        };
    }
}
