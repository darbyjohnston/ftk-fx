// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/CacheBar.h>

#include <fx/App/SceneModel.h>

#include <ftk/UI/DrawUtil.h>

#include <ftk/Core/Math.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void CacheBar::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(context, "fx::app::CacheBar", parent);
            setHStretch(Stretch::Expanding);
            _setMousePressEnabled(true);
            _model = model;

            _rangeObserver = Observer<RangeI>::create(
                model->observeRange(),
                [this](const RangeI& value)
                {
                    _range = value;
                    setDrawUpdate();
                });

            _currentFrameObserver = Observer<int>::create(
                model->observeCurrentFrame(),
                [this](int value)
                {
                    _currentFrame = value;
                    setDrawUpdate();
                });

            _statesObserver = ListObserver<core::FrameState>::create(
                model->observeCacheStates(),
                [this](const std::vector<core::FrameState>& value)
                {
                    _states = value;
                    setDrawUpdate();
                });
        }

        CacheBar::~CacheBar()
        {}

        std::shared_ptr<CacheBar> CacheBar::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<CacheBar>(new CacheBar);
            out->_init(context, model, parent);
            return out;
        }

        Size2I CacheBar::getSizeHint() const
        {
            return _sizeHint;
        }

        void CacheBar::sizeHintEvent(const SizeHintEvent& event)
        {
            const int handle = event.style->getSizeRole(
                SizeRole::Handle,
                event.displayScale);
            _sizeHint.w = handle * 8;
            _sizeHint.h = handle;
        }

        int CacheBar::_getFrame(int x) const
        {
            const Box2I& g = getGeometry();
            if (g.w() <= 0)
                return _range.min();
            const int count = _range.max() - _range.min() + 1;
            const int i = (x - g.min.x) * count / g.w();
            return clamp(_range.min() + i, _range.min(), _range.max());
        }

        Box2I CacheBar::_getCellBox(int frame) const
        {
            const Box2I& g = getGeometry();
            const int count = _range.max() - _range.min() + 1;
            const int i = frame - _range.min();
            // Both edges are computed from the frame index rather than the
            // width being divided, so the cells tile exactly and there is no
            // rounding gap between them.
            const int x0 = g.min.x + i * g.w() / count;
            const int x1 = g.min.x + (i + 1) * g.w() / count;
            return Box2I(x0, g.min.y, std::max(1, x1 - x0), g.h());
        }

        void CacheBar::drawEvent(const Box2I& drawRect, const DrawEvent& event)
        {
            const Box2I& g = getGeometry();
            event.render->drawRect(g, event.style->getColorRole(ColorRole::Well));

            // One pass per state rather than one draw per frame: the cells of a
            // colour are almost always contiguous, and this keeps a 1000 frame
            // range to a handful of draw calls.
            std::vector<Box2I> simulated;
            std::vector<Box2I> locked;
            const int count = static_cast<int>(_states.size());
            for (int i = 0; i < count; ++i)
            {
                switch (_states[i])
                {
                case core::FrameState::Simulated:
                    simulated.push_back(_getCellBox(_range.min() + i));
                    break;
                case core::FrameState::Locked:
                    locked.push_back(_getCellBox(_range.min() + i));
                    break;
                default: break;
                }
            }
            event.render->drawRects(simulated, Color4F(.22F, .42F, .27F));
            event.render->drawRects(locked, Color4F(.24F, .38F, .58F));

            const Box2I playhead = _getCellBox(_currentFrame);
            event.render->drawRect(
                Box2I(playhead.min.x, g.min.y, std::max(2, playhead.w()), g.h()),
                event.style->getColorRole(ColorRole::Text));
        }

        void CacheBar::mousePressEvent(MouseClickEvent& event)
        {
            IMouseWidget::mousePressEvent(event);
            if (auto model = _model.lock())
            {
                model->setCurrentFrame(_getFrame(event.pos.x));
            }
        }

        void CacheBar::mouseMoveEvent(MouseMoveEvent& event)
        {
            IMouseWidget::mouseMoveEvent(event);
            if (_isMousePressed())
            {
                if (auto model = _model.lock())
                {
                    model->setCurrentFrame(_getFrame(event.pos.x));
                }
            }
        }
    }
}
