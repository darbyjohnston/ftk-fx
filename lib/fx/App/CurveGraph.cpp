// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/CurveEditorPrivate.h>

#include <fx/App/SceneModel.h>

#include <ftk/UI/DrawUtil.h>

#include <ftk/Core/Math.h>

#include <algorithm>
#include <sstream>
#include <iomanip>
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

        namespace
        {
            //! A little air above and below, and a range that is never zero
            //! high: a curve with one key is a horizontal line, and it should
            //! be a horizontal line through the middle rather than a division
            //! by zero.
            RangeF padded(float min, float max)
            {
                const float pad = std::max(
                    (max - min) * .1F,
                    std::max(std::abs(max), 1.F) * .1F);
                return RangeF(min - pad, max + pad);
            }
        }

        namespace
        {
            //! A round step that puts about the wanted number of ticks across
            //! a span: one of 1, 2 or 5 times a power of ten, so the labels
            //! read as numbers a person would have chosen.
            double niceStep(double span, int wanted)
            {
                if (span <= 0.0 || wanted < 1)
                    return 1.0;
                const double rough = span / wanted;
                const double power = std::pow(10.0, std::floor(std::log10(rough)));
                const double n = rough / power;
                double step = 10.0;
                if (n < 1.5) step = 1.0;
                else if (n < 3.0) step = 2.0;
                else if (n < 7.0) step = 5.0;
                return step * power;
            }

            //! A tick's value, with only as many decimals as its step needs.
            //! A step of 10 wants no point at all, and one of 0.05 wants two.
            std::string tickLabel(double value, double step)
            {
                int decimals = 0;
                if (step < 1.0)
                {
                    decimals = static_cast<int>(
                        std::ceil(-std::log10(step)));
                    decimals = std::min(decimals, 4);
                }
                // Negative zero reads as a mistake rather than as zero.
                if (std::abs(value) < step * .0001)
                {
                    value = 0.0;
                }
                std::stringstream ss;
                ss << std::fixed << std::setprecision(decimals) << value;
                return ss.str();
            }
        }

        Box2I CurveGraph::_plotBox() const
        {
            // The margin, and then room for the labels: a gutter down the
            // left for values and one along the bottom for frames. Worked
            // out here rather than at each of the four places that used to
            // do it, because a gutter reserved in three of them is a plot
            // that draws its curves over its own axis.
            return margin(
                getGeometry(),
                -(_size.margin + _size.valueGutter),
                -_size.margin,
                -_size.margin,
                -(_size.margin + _size.frameGutter));
        }

        void CurveGraph::_valueRangeUpdate()
        {
            _channelRanges.assign(_channels.size(), RangeF(0.F, 1.F));
            float allMin = 0.F;
            float allMax = 0.F;
            bool any = false;
            for (size_t c = 0; c < _channels.size(); ++c)
            {
                const auto* parameter = _channels[c].parameter;
                if (core::Parameter::Type::Curve != parameter->getType())
                    continue;
                float min = 0.F;
                float max = 0.F;
                bool first = true;
                for (const auto& key : parameter->getCurve().getKeys())
                {
                    min = first ? key.value : std::min(min, key.value);
                    max = first ? key.value : std::max(max, key.value);
                    first = false;
                }
                if (first)
                    continue;
                _channelRanges[c] = padded(min, max);
                allMin = any ? std::min(allMin, min) : min;
                allMax = any ? std::max(allMax, max) : max;
                any = true;
            }
            _valueRange = any ? padded(allMin, allMax) : RangeF(0.F, 1.F);
        }

        const RangeF& CurveGraph::_getValueRange(size_t channel) const
        {
            if (CurveValueMode::Normalized == _valueMode &&
                channel < _channelRanges.size())
            {
                return _channelRanges[channel];
            }
            return _valueRange;
        }

        void CurveGraph::setValueMode(CurveValueMode value)
        {
            if (value == _valueMode)
                return;
            _valueMode = value;
            setDrawUpdate();
        }

        V2I CurveGraph::getPos(size_t channel, double frame, float value) const
        {
            const V2F p = _toPos(channel, frame, value);
            return V2I(
                static_cast<int>(std::round(p.x)),
                static_cast<int>(std::round(p.y)));
        }

        V2F CurveGraph::_toPos(size_t channel, double frame, float value) const
        {
            const Box2I g = _plotBox();
            const RangeI& r = _range;
            const double frames = std::max(1, r.max() - r.min());
            const RangeF& v = _getValueRange(channel);
            const float span = std::max(v.max() - v.min(), .0001F);
            return V2F(
                g.min.x + (frame - r.min()) / frames * g.w(),
                // Value up, pixels down.
                g.max.y - (value - v.min()) / span * g.h());
        }

        double CurveGraph::_toFrame(int x) const
        {
            const Box2I g = _plotBox();
            if (g.w() <= 0)
                return _range.min();
            const double frames = _range.max() - _range.min();
            return _range.min() + (x - g.min.x) / static_cast<double>(g.w()) * frames;
        }

        float CurveGraph::_toValue(size_t channel, int y) const
        {
            const Box2I g = _plotBox();
            if (g.h() <= 0)
                return 0.F;
            const RangeF& v = _getValueRange(channel);
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
                    const V2F p = _toPos(c, keys[k].frame, keys[k].value);
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

        void CurveGraph::_axesDraw(const DrawEvent& event)
        {
            const Box2I plot = _plotBox();
            if (plot.w() <= 0 || plot.h() <= 0)
                return;
            const Color4F grid =
                event.style->getColorRole(ColorRole::Border);
            const Color4F text =
                event.style->getColorRole(ColorRole::TextDisabled);

            // Frames along the bottom. Always meaningful: every channel is
            // plotted against the same frames whatever the value mode is.
            const double frameSpan = _range.max() - _range.min();
            const double frameStep = niceStep(frameSpan, 6);
            const double frameFirst =
                std::ceil(_range.min() / frameStep) * frameStep;
            for (double f = frameFirst; f <= _range.max(); f += frameStep)
            {
                const int x = static_cast<int>(_toPos(0, f, 0.F).x);
                event.render->drawRect(
                    Box2I(x, plot.min.y, _size.border, plot.h()), grid);
                const std::string label = tickLabel(f, frameStep);
                const Size2I size =
                    event.fontSystem->getSize(label, _size.fontInfo);
                // Held inside the widget. Centred on its tick, the label
                // for the last frame runs half its width past the end of the
                // plot and comes back as "12" where it should say "120".
                const Box2I& all = getGeometry();
                event.render->drawText(
                    event.fontSystem->getGlyphs(label, _size.fontInfo),
                    _size.fontMetrics,
                    V2I(
                        clamp(
                            x - size.w / 2,
                            all.min.x,
                            all.max.x - size.w),
                        plot.max.y + _size.margin),
                    text);
            }

            // Values up the left, and only when they mean something. Each
            // channel has its own range in normalized mode, so one column of
            // numbers beside three curves would be true of at most one of
            // them -- the mode is for comparing shapes, and it says so.
            if (CurveValueMode::Absolute != _valueMode)
                return;
            const double valueSpan = _valueRange.max() - _valueRange.min();
            const double valueStep = niceStep(valueSpan, 5);
            const double valueFirst =
                std::ceil(_valueRange.min() / valueStep) * valueStep;
            for (double v = valueFirst; v <= _valueRange.max(); v += valueStep)
            {
                const int y = static_cast<int>(
                    _toPos(0, _range.min(), static_cast<float>(v)).y);
                event.render->drawRect(
                    Box2I(plot.min.x, y, plot.w(), _size.border), grid);
                const std::string label = tickLabel(v, valueStep);
                const Size2I size =
                    event.fontSystem->getSize(label, _size.fontInfo);
                event.render->drawText(
                    event.fontSystem->getGlyphs(label, _size.fontInfo),
                    _size.fontMetrics,
                    V2I(
                        plot.min.x - _size.margin - size.w,
                        y - size.h / 2),
                    text);
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
                // Room for the widest label either axis is likely to want,
                // measured rather than guessed: a gutter sized to "0" is a
                // gutter that clips every number that is not zero.
                _size.valueGutter = event.fontSystem->getSize(
                    "-0000.00", _size.fontInfo).w + _size.margin;
                _size.frameGutter =
                    event.fontSystem->getSize("0", _size.fontInfo).h +
                    _size.margin;
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

            // Where zero is, when zero is on screen and means the same thing
            // for every channel. Normalized, each curve has its own zero and a
            // single line would be a lie.
            if (CurveValueMode::Absolute == _valueMode &&
                v.min() < 0.F && v.max() > 0.F)
            {
                const V2F zero = _toPos(0, _range.min(), 0.F);
                event.render->drawRect(
                    Box2I(g.min.x, static_cast<int>(zero.y), g.w(), _size.border),
                    event.style->getColorRole(ColorRole::Border));
            }

            // The playhead, in the colour tlRender's timeline uses for the
            // current time, which the cache bar below now uses too. Drawn in
            // Border it was the grid's own colour, and once the grid had
            // lines of its own there was nothing to tell them apart.
            const V2F playhead = _toPos(0, _currentFrame, v.min());
            event.render->drawRect(
                Box2I(
                    static_cast<int>(playhead.x),
                    g.min.y,
                    _size.border * 2,
                    g.h()),
                event.style->getColorRole(ColorRole::Red));

            _axesDraw(event);

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
                const Box2I inside = _plotBox();
                std::vector<std::pair<V2F, V2F> > lines;
                V2F prev;
                for (int x = 0; x <= inside.w(); ++x)
                {
                    const double frame = _toFrame(inside.min.x + x);
                    const V2F p = _toPos(c, frame, curve.getValue(frame));
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
                    const V2F p = _toPos(c, keys[k].frame, keys[k].value);
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
                    _toValue(_selectedChannel, event.pos.y));
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
