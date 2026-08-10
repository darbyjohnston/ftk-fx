// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/CurveEditorPrivate.h>

#include <fx/App/SceneModel.h>

#include <ftk/UI/DrawUtil.h>

#include <ftk/Core/Math.h>

#include <algorithm>
#include <cmath>

using namespace ftk;

namespace fx
{
    namespace app
    {
        namespace
        {
            //! In the order the channel list hands them out, so a channel
            //! keeps its colour whatever else is shown.
            const std::vector<Color4F> channelColors =
            {
                Color4F(.36F, .70F, .95F),
                Color4F(.95F, .55F, .35F),
                Color4F(.55F, .85F, .45F),
                Color4F(.90F, .45F, .70F),
                Color4F(.85F, .80F, .40F),
                Color4F(.65F, .60F, .95F),
                Color4F(.40F, .85F, .80F),
                Color4F(.90F, .65F, .45F)
            };

            Color4F channelColor(size_t i)
            {
                return channelColors[i % channelColors.size()];
            }
        }

        void CurveGraph::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(context, "fx::app::CurveGraph", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);
            setAcceptsKeyFocus(true);
            _setMousePressEnabled(true);
            _model = model;
            _range = model->getRange();
            _currentFrame = model->getCurrentFrame();

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
            // An edit that does not change which channels exist still changes
            // where the keys are, and the editor only hands the plot its
            // channels when the set differs -- so nothing else would ask for
            // this redraw.
            _parameterObserver = Observer<int>::create(
                model->observeParameterChanged(),
                [this](int)
                {
                    if (!_dragging)
                    {
                        _valueRangeUpdate();
                    }
                    setDrawUpdate();
                });
            _sceneObserver = Observer<int>::create(
                model->observeSceneChanged(),
                [this](int)
                {
                    // The keys the selection referred to are gone.
                    _hasSelection = false;
                    _dragging = false;
                    setDrawUpdate();
                });
        }

        CurveGraph::~CurveGraph()
        {}

        std::shared_ptr<CurveGraph> CurveGraph::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<CurveGraph>(new CurveGraph);
            out->_init(context, model, parent);
            return out;
        }

        void CurveGraph::setChannels(const std::vector<ParameterInfo>& value)
        {
            _channels = value;
            _hasSelection = false;
            _dragging = false;
            _valueRangeUpdate();
            setDrawUpdate();
        }

        void CurveGraph::_valueRangeUpdate()
        {
            float min = 0.F;
            float max = 0.F;
            bool first = true;
            for (const auto& info : _channels)
            {
                if (core::Parameter::Type::Curve != info.parameter->getType())
                    continue;
                for (const auto& key : info.parameter->getCurve().getKeys())
                {
                    min = first ? key.value : std::min(min, key.value);
                    max = first ? key.value : std::max(max, key.value);
                    first = false;
                }
            }
            if (first)
            {
                _valueRange = RangeF(0.F, 1.F);
                return;
            }
            // A little air above and below, and a range that is never zero
            // high: a curve with one key is a horizontal line, and it should
            // be a horizontal line through the middle rather than a division
            // by zero.
            const float pad = std::max((max - min) * .1F, std::max(std::abs(max), 1.F) * .1F);
            _valueRange = RangeF(min - pad, max + pad);
        }

        V2F CurveGraph::_toPos(double frame, float value) const
        {
            const Box2I g = margin(getGeometry(), -_size.margin);
            const RangeI& r = _range;
            const double frames = std::max(1, r.max() - r.min());
            const RangeF& v = _valueRange;
            const float span = std::max(v.max() - v.min(), .0001F);
            return V2F(
                g.min.x + (frame - r.min()) / frames * g.w(),
                // Value up, pixels down.
                g.max.y - (value - v.min()) / span * g.h());
        }

        double CurveGraph::_toFrame(int x) const
        {
            const Box2I g = margin(getGeometry(), -_size.margin);
            if (g.w() <= 0)
                return _range.min();
            const double frames = _range.max() - _range.min();
            return _range.min() + (x - g.min.x) / static_cast<double>(g.w()) * frames;
        }

        float CurveGraph::_toValue(int y) const
        {
            const Box2I g = margin(getGeometry(), -_size.margin);
            if (g.h() <= 0)
                return 0.F;
            const RangeF& v = _valueRange;
            return v.min() +
                (g.max.y - y) / static_cast<float>(g.h()) * (v.max() - v.min());
        }

        bool CurveGraph::_hit(const V2I& pos, size_t& channel, size_t& key) const
        {
            const int reach = _size.handle;
            int best = reach * reach;
            bool found = false;
            for (size_t c = 0; c < _channels.size(); ++c)
            {
                const auto* parameter = _channels[c].parameter;
                if (core::Parameter::Type::Curve != parameter->getType())
                    continue;
                const auto& keys = parameter->getCurve().getKeys();
                for (size_t k = 0; k < keys.size(); ++k)
                {
                    const V2F p = _toPos(keys[k].frame, keys[k].value);
                    const int dx = static_cast<int>(p.x) - pos.x;
                    const int dy = static_cast<int>(p.y) - pos.y;
                    const int d = dx * dx + dy * dy;
                    if (d <= best)
                    {
                        best = d;
                        channel = c;
                        key = k;
                        found = true;
                    }
                }
            }
            return found;
        }

        void CurveGraph::_moveKey(
            size_t channel,
            size_t key,
            double frame,
            float value)
        {
            if (channel >= _channels.size())
                return;
            auto model = _model.lock();
            if (!model)
                return;
            auto* parameter = _channels[channel].parameter;
            if (core::Parameter::Type::Curve != parameter->getType())
                return;
            const sim::System before = model->getSystem();
            core::Curve curve = parameter->getCurve();
            auto keys = curve.getKeys();
            if (key >= keys.size())
                return;

            // Keys land on whole frames. Sub-frame keys are allowed by the
            // curve, but nothing in this editor can author one deliberately
            // yet, and an accidental one at 41.9973 is only ever a nuisance.
            const double f = clamp(
                std::round(frame),
                static_cast<double>(_range.min()),
                static_cast<double>(_range.max()));

            core::Key moved = keys[key];
            moved.frame = f;
            moved.value = value;
            keys.erase(keys.begin() + key);
            // Dropping a key onto another one replaces it, which is what
            // setKeys does; the selection then has to follow where it landed.
            keys.push_back(moved);
            curve.setKeys(keys);
            parameter->setCurve(curve);

            const auto& sorted = curve.getKeys();
            for (size_t i = 0; i < sorted.size(); ++i)
            {
                if (sorted[i].frame == f)
                {
                    _selectedKey = i;
                    break;
                }
            }

            // One name for the whole drag, so the moves merge into a single
            // undo step.
            model->systemChanged("Move Key", before);
        }

        Size2I CurveGraph::getSizeHint() const
        {
            return Size2I(_size.handle * 24, _size.handle * 12);
        }

        void CurveGraph::styleEvent(const StyleEvent& event)
        {
            IMouseWidget::styleEvent(event);
            if (event.hasChanges())
            {
                _size.init = true;
            }
        }

        void CurveGraph::sizeHintEvent(const SizeHintEvent& event)
        {
            IMouseWidget::sizeHintEvent(event);
            if (_size.init)
            {
                _size.init = false;
                _size.margin = event.style->getSizeRole(
                    SizeRole::MarginSmall, event.displayScale);
                _size.border = event.style->getSizeRole(
                    SizeRole::Border, event.displayScale);
                _size.handle = event.style->getSizeRole(
                    SizeRole::Handle, event.displayScale);
                _size.fontInfo = event.style->getFont(
                    FontType::Mono, event.displayScale);
                _size.fontMetrics = event.fontSystem->getMetrics(_size.fontInfo);
            }
        }

        void CurveGraph::drawEvent(const Box2I& drawRect, const DrawEvent& event)
        {
            IMouseWidget::drawEvent(drawRect, event);
            const Box2I& g = getGeometry();
            event.render->drawRect(
                g, event.style->getColorRole(ColorRole::Base));

            if (!_dragging)
            {
                _valueRangeUpdate();
            }
            const RangeF& v = _valueRange;

            // Where zero is, when zero is on screen. It is the line an artist
            // reads a curve against.
            if (v.min() < 0.F && v.max() > 0.F)
            {
                const V2F zero = _toPos(_range.min(), 0.F);
                event.render->drawRect(
                    Box2I(g.min.x, static_cast<int>(zero.y), g.w(), _size.border),
                    event.style->getColorRole(ColorRole::Border));
            }

            // The playhead, so the plot and the timeline agree about where
            // "now" is.
            const V2F playhead = _toPos(_currentFrame, v.min());
            event.render->drawRect(
                Box2I(
                    static_cast<int>(playhead.x),
                    g.min.y,
                    _size.border * 2,
                    g.h()),
                event.style->getColorRole(ColorRole::Border));

            LineOptions lineOptions;
            lineOptions.width = _size.border * 2;
            for (size_t c = 0; c < _channels.size(); ++c)
            {
                const auto* parameter = _channels[c].parameter;
                if (core::Parameter::Type::Curve != parameter->getType())
                    continue;
                const Color4F color = channelColor(c);
                const core::Curve& curve = parameter->getCurve();

                // Sampled a pixel at a time rather than drawn as segments
                // between keys: a smooth or Bezier key is a cubic, and this
                // way the plot shows whatever the solver will actually see.
                const Box2I inside = margin(g, -_size.margin);
                std::vector<std::pair<V2F, V2F> > lines;
                V2F prev;
                for (int x = 0; x <= inside.w(); ++x)
                {
                    const double frame = _toFrame(inside.min.x + x);
                    const V2F p = _toPos(frame, curve.getValue(frame));
                    if (x > 0)
                    {
                        lines.push_back(std::make_pair(prev, p));
                    }
                    prev = p;
                }
                event.render->drawLines(lines, color, lineOptions);

                const auto& keys = curve.getKeys();
                for (size_t k = 0; k < keys.size(); ++k)
                {
                    const V2F p = _toPos(keys[k].frame, keys[k].value);
                    const bool selected =
                        _hasSelection && c == _selectedChannel && k == _selectedKey;
                    const int radius = _size.handle / (selected ? 2 : 3);
                    event.render->drawMesh(
                        circle(V2I(static_cast<int>(p.x), static_cast<int>(p.y)), radius),
                        selected ?
                        event.style->getColorRole(ColorRole::Text) :
                        color);
                }
            }
        }

        void CurveGraph::mousePressEvent(MouseClickEvent& event)
        {
            IMouseWidget::mousePressEvent(event);
            takeKeyFocus();
            size_t channel = 0;
            size_t key = 0;
            if (_hit(event.pos, channel, key))
            {
                _selectedChannel = channel;
                _selectedKey = key;
                _hasSelection = true;
                _dragging = true;
                if (auto model = _model.lock())
                {
                    model->beginEdit();
                }
            }
            else
            {
                // Clicking the background scrubs, which is what the space is
                // for when there is no key under the pointer.
                _hasSelection = false;
                if (auto model = _model.lock())
                {
                    model->setCurrentFrame(static_cast<int>(
                        std::round(_toFrame(event.pos.x))));
                }
            }
            setDrawUpdate();
        }

        void CurveGraph::mouseReleaseEvent(MouseClickEvent& event)
        {
            IMouseWidget::mouseReleaseEvent(event);
            if (_dragging)
            {
                if (auto model = _model.lock())
                {
                    model->endEdit("Move Key");
                }
            }
            _dragging = false;
            // The keys have moved, so the plot can find its scale again.
            _valueRangeUpdate();
            setDrawUpdate();
        }

        void CurveGraph::mouseMoveEvent(MouseMoveEvent& event)
        {
            IMouseWidget::mouseMoveEvent(event);
            if (!_isMousePressed())
                return;
            if (_dragging && _hasSelection)
            {
                _moveKey(
                    _selectedChannel,
                    _selectedKey,
                    _toFrame(event.pos.x),
                    _toValue(event.pos.y));
                setDrawUpdate();
            }
            else if (auto model = _model.lock())
            {
                model->setCurrentFrame(static_cast<int>(
                    std::round(_toFrame(event.pos.x))));
            }
        }

        void CurveGraph::keyPressEvent(KeyEvent& event)
        {
            if (_hasSelection && 0 == event.modifiers &&
                (Key::Delete == event.key || Key::Backspace == event.key))
            {
                event.accept = true;
                auto model = _model.lock();
                auto* parameter = _channels[_selectedChannel].parameter;
                if (model && core::Parameter::Type::Curve == parameter->getType())
                {
                    const sim::System before = model->getSystem();
                    core::Curve curve = parameter->getCurve();
                    curve.removeKey(_selectedKey);
                    // The last key gone leaves the parameter constant again,
                    // holding the value it was holding before it was animated.
                    if (curve.getKeys().empty())
                    {
                        parameter->setConstant(parameter->getConstant());
                    }
                    else
                    {
                        parameter->setCurve(curve);
                    }
                    model->systemChanged("Delete Key", before);
                }
                _hasSelection = false;
                setDrawUpdate();
                return;
            }
            IMouseWidget::keyPressEvent(event);
        }
    }
}
