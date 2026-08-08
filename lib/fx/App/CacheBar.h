// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Cache.h>

#include <ftk/UI/IMouseWidget.h>

namespace fx
{
    namespace app
    {
        class SceneModel;

        //! The cache bar.
        //!
        //! One cell per frame, coloured by what the cache holds, with the
        //! playhead drawn over it. Clicking scrubs, so this is the timeline as
        //! well as the read-out: the artist should never have to work out which
        //! frames are cached from how the application behaves.
        class CacheBar : public ftk::IMouseWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            CacheBar() = default;

        public:
            virtual ~CacheBar();

            static std::shared_ptr<CacheBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;

        private:
            //! Get the frame under a position in the widget.
            int _getFrame(int x) const;

            //! Get the box the given frame's cell occupies.
            ftk::Box2I _getCellBox(int frame) const;

            std::weak_ptr<SceneModel> _model;
            ftk::RangeI _range = ftk::RangeI(1, 120);
            int _currentFrame = 1;
            std::vector<core::FrameState> _states;
            ftk::Size2I _sizeHint;

            std::shared_ptr<ftk::Observer<ftk::RangeI> > _rangeObserver;
            std::shared_ptr<ftk::Observer<int> > _currentFrameObserver;
            std::shared_ptr<ftk::ListObserver<core::FrameState> > _statesObserver;
        };
    }
}
