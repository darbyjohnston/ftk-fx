// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/TimelineBar.h>

#include <fx/App/CacheBar.h>
#include <fx/App/SceneModel.h>

#include <ftk/UI/CheckBox.h>
#include <ftk/UI/Divider.h>
#include <tlRender/UI/TimeEdit.h>
#include <tlRender/UI/TimeLabel.h>
#include <tlRender/UI/TimeUnitsWidget.h>

#include <tlRender/Timeline/TimeUnits.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Spacer.h>
#include <ftk/UI/ToolButton.h>

#include <ftk/Core/Format.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        namespace
        {
            std::shared_ptr<ToolButton> button(
                const std::shared_ptr<Context>& context,
                const std::shared_ptr<IWidget>& parent,
                const std::string& icon,
                const std::string& tooltip,
                const std::function<void(void)>& callback)
            {
                auto out = ToolButton::create(context, parent);
                out->setIcon(icon);
                out->setTooltip(tooltip);
                out->setClickedCallback(callback);
                return out;
            }
        }

        void TimelineBar::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, "fx::app::TimelineBar", parent);
            _model = model;

            _layout = VerticalLayout::create(context);
            _layout->setSpacingRole(SizeRole::SpacingSmall);
            _layout->setMarginRole(SizeRole::MarginSmall);
            _setWidget(_layout);

            auto hLayout = HorizontalLayout::create(context, _layout);
            hLayout->setSpacingRole(SizeRole::SpacingTool);

            std::weak_ptr<SceneModel> weak(model);
            button(context, hLayout, "FrameStart", "Go to the start",
                [weak] { if (auto m = weak.lock()) m->frameStart(); });
            button(context, hLayout, "FramePrev", "Go to the previous frame",
                [weak] { if (auto m = weak.lock()) m->framePrev(); });

            _playButton = ToolButton::create(context, hLayout);
            _playButton->setCheckable(true);
            _playButton->setIcon("PlaybackForward");
            _playButton->setCheckedIcon("PlaybackStop");
            _playButton->setTooltip("Play");
            _playButton->setCheckedCallback(
                [weak](bool value) { if (auto m = weak.lock()) m->setPlaying(value); });

            button(context, hLayout, "FrameNext", "Go to the next frame",
                [weak] { if (auto m = weak.lock()) m->frameNext(); });
            button(context, hLayout, "FrameEnd", "Go to the end",
                [weak] { if (auto m = weak.lock()) m->frameEnd(); });

            // tlRender's, so that a frame count, a timecode and seconds are
            // all the same widget reading one model -- and so that the units
            // an artist picks here are the units they will get everywhere
            // else once this application reads media.
            _frameRate = model->getFrameRate();
            _timeUnitsModel = tl::TimeUnitsModel::create(context);
            // Frames to start with. Timecode is the right default for a
            // player, where the media has a time of its own; here a frame is
            // the unit the simulation is actually stepped in, and the cache
            // and the curve editor both count in them.
            _timeUnitsModel->setTimeUnits(tl::TimeUnits::Frames);
            _timeEdit = tl::ui::TimeEdit::create(
                context, _timeUnitsModel, hLayout);
            _timeEdit->setTooltip("The current frame");
            _timeEdit->setCallback(
                [weak](const OTIO_NS::RationalTime& value)
                {
                    if (auto m = weak.lock())
                    {
                        m->setCurrentFrame(static_cast<int>(value.value()));
                    }
                });

            _durationLabel = tl::ui::TimeLabel::create(
                context, _timeUnitsModel, hLayout);
            _durationLabel->setTooltip("The length of the simulation");

            _lockCheckBox = CheckBox::create(context, "Lock", hLayout);
            _lockCheckBox->setTooltip(
                "Freeze this frame so that changing a parameter cannot "
                "re-simulate it");
            _lockCheckBox->setCheckedCallback(
                [weak](bool value) { if (auto m = weak.lock()) m->setCurrentLocked(value); });

            hLayout->addSpacer(Stretch::Expanding);

            // The units button at the far end, beside the read-outs it
            // changes rather than beside the transport it does not.
            _timeUnitsWidget = tl::ui::TimeUnitsWidget::create(
                context, _timeUnitsModel, hLayout);
            _timeUnitsWidget->setTooltip("Show the time as frames, seconds or timecode");

            _statusLabel = Label::create(context, hLayout);
            _statusLabel->setFont(FontType::Mono);

            _cacheBar = CacheBar::create(context, model, _layout);
            setScreenshotTag(_cacheBar, "MainWindow.CacheBar");

            _rangeObserver = Observer<RangeI>::create(
                model->observeRange(),
                [this](const RangeI& value)
                {
                    _range = value;
                    // The whole range as a duration, so the read-out beside
                    // the current time says how much there is of it.
                    _durationLabel->setValue(OTIO_NS::RationalTime(
                        value.max() - value.min() + 1, _frameRate));
                });

            _currentFrameObserver = Observer<int>::create(
                model->observeCurrentFrame(),
                [this](int value)
                {
                    _currentFrame = value;
                    _timeEdit->setValue(OTIO_NS::RationalTime(value, _frameRate));
                    _lockUpdate();
                });

            _playingObserver = Observer<bool>::create(
                model->observePlaying(),
                [this](bool value)
                {
                    _playButton->setChecked(value);
                    _playButton->setTooltip(value ? "Stop" : "Play");
                });

            _frameObserver = Observer<std::shared_ptr<const core::Frame> >::create(
                model->observeFrame(),
                [this](const std::shared_ptr<const core::Frame>& value)
                {
                    _particleCount = value ? value->getParticleCount() : 0;
                    _statusUpdate();
                });

            _cacheByteCountObserver = Observer<size_t>::create(
                model->observeCacheByteCount(),
                [this](size_t value)
                {
                    _cacheByteCount = value;
                    _statusUpdate();
                });

            _statesObserver = ListObserver<core::FrameState>::create(
                model->observeCacheStates(),
                [this](const std::vector<core::FrameState>& value)
                {
                    _states = value;
                    _lockUpdate();
                });
        }

        TimelineBar::~TimelineBar()
        {}

        std::shared_ptr<TimelineBar> TimelineBar::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<TimelineBar>(new TimelineBar);
            out->_init(context, model, parent);
            return out;
        }



        void TimelineBar::_statusUpdate()
        {
            _statusLabel->setText(Format("{0} particles   {1} MB cached").
                arg(_particleCount).
                arg(_cacheByteCount / (1024.0 * 1024.0), 1));
        }

        void TimelineBar::_lockUpdate()
        {
            const size_t i = static_cast<size_t>(_currentFrame - _range.min());
            const bool locked = i < _states.size() &&
                core::FrameState::Locked == _states[i];
            _lockCheckBox->setChecked(locked);
        }
    }
}
