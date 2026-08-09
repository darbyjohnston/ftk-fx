// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Cache.h>

#include <ftk/UI/IContainer.h>
#include <ftk/UI/RowLayout.h>

#include <ftk/Core/ObservableList.h>

namespace ftk
{
    class CheckBox;
    class IntEdit;
    class Label;
    class ToolButton;
}

namespace fx
{
    namespace app
    {
        class CacheBar;
        class SceneModel;

        //! The transport controls and the cache bar.
        class TimelineBar : public ftk::IContainer
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            TimelineBar() = default;

        public:
            virtual ~TimelineBar();

            static std::shared_ptr<TimelineBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);


        private:
            void _statusUpdate();
            void _lockUpdate();

            std::weak_ptr<SceneModel> _model;
            int _currentFrame = 1;
            size_t _particleCount = 0;
            size_t _cacheByteCount = 0;
            std::vector<core::FrameState> _states;
            ftk::RangeI _range = ftk::RangeI(1, 120);

            std::shared_ptr<ftk::VerticalLayout> _layout;
            std::shared_ptr<ftk::ToolButton> _playButton;
            std::shared_ptr<ftk::IntEdit> _frameEdit;
            std::shared_ptr<ftk::CheckBox> _lockCheckBox;
            std::shared_ptr<ftk::Label> _statusLabel;
            std::shared_ptr<CacheBar> _cacheBar;

            std::shared_ptr<ftk::Observer<ftk::RangeI> > _rangeObserver;
            std::shared_ptr<ftk::Observer<int> > _currentFrameObserver;
            std::shared_ptr<ftk::Observer<bool> > _playingObserver;
            std::shared_ptr<ftk::Observer<size_t> > _cacheByteCountObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<const core::Frame> > >
                _frameObserver;
            std::shared_ptr<ftk::ListObserver<core::FrameState> > _statesObserver;
        };
    }
}
