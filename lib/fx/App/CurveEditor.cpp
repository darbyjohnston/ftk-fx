// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/CurveEditorPrivate.h>

#include <fx/App/SceneModel.h>

#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/Splitter.h>

#include <algorithm>

using namespace ftk;

namespace fx
{
    namespace app
    {
        std::vector<std::string> getCurveValueModeLabels()
        {
            return { "Absolute", "Normalized" };
        }

        void CurveEditor::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, "fx::app::CurveEditor", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);
            _model = model;

            _graph = CurveGraph::create(context, model);

            // Above the channels rather than over the plot: it says how to
            // read the vertical axis, and the channels are what it is read
            // against.
            _valueModeComboBox = ComboBox::create(
                context, getCurveValueModeLabels());
            _valueModeComboBox->setCurrentIndex(0);
            _valueModeComboBox->setTooltip(
                "Absolute compares the channels' values; normalized compares "
                "their shapes");
            _valueModeComboBox->setIndexCallback(
                [this](int value)
                {
                    _graph->setValueMode(static_cast<CurveValueMode>(value));
                });

            _channelLayout = VerticalLayout::create(context);
            _channelLayout->setMarginRole(SizeRole::MarginSmall);
            _channelLayout->setSpacingRole(SizeRole::SpacingSmall);
            auto scrollWidget = ScrollWidget::create(context);
            scrollWidget->setWidget(_channelLayout);
            scrollWidget->setBorder(false);

            auto channelColumn = VerticalLayout::create(context);
            channelColumn->setSpacingRole(SizeRole::None);
            _valueModeComboBox->setParent(channelColumn);
            scrollWidget->setParent(channelColumn);

            auto splitter = Splitter::create(context, Orientation::Horizontal);
            splitter->setSplit(.3F);
            splitter->setWidgets({ channelColumn, _graph });
            _setWidget(splitter);

            _channelsUpdate();

            // Which parameters are animated changes when a key is set from the
            // panel, which arrives as a parameter edit rather than as a new
            // scene. That fires on every step of a drag, so the list is only
            // rebuilt when the set of animated parameters actually differs.
            _parameterObserver = Observer<int>::create(
                model->observeParameterChanged(),
                [this](int) { _channelsUpdate(); });
            _sceneObserver = Observer<int>::create(
                model->observeSceneChanged(),
                [this](int)
                {
                    // The rows have to be rebuilt whatever the new scene
                    // animates, so the record of what they last showed is
                    // thrown away rather than compared against.
                    //
                    // What is ticked is not: the parameters have the same
                    // names in every scene, and a channel the artist chose to
                    // watch should still be the one they are watching. Note
                    // that an observer runs its callback when it is created,
                    // so anything cleared here is cleared before the artist
                    // has done anything at all.
                    _animated.clear();
                    _channelsUpdate();
                });
        }

        CurveEditor::~CurveEditor()
        {}

        std::shared_ptr<CurveEditor> CurveEditor::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<CurveEditor>(new CurveEditor);
            out->_init(context, model, parent);
            return out;
        }

        void CurveEditor::setValueMode(CurveValueMode value)
        {
            _valueModeComboBox->setCurrentIndex(static_cast<int>(value));
            _graph->setValueMode(value);
        }

        void CurveEditor::_graphUpdate()
        {
            std::vector<ParameterInfo> shown;
            std::vector<std::string> paths;
            for (const auto& info : _channels)
            {
                if (core::Parameter::Type::Curve == info.parameter->getType() &&
                    std::find(_shown.begin(), _shown.end(), info.getPath()) !=
                    _shown.end())
                {
                    shown.push_back(info);
                    paths.push_back(info.getPath());
                }
            }
            // Only when the set actually differs. Handing the plot the same
            // channels again drops its selection, and this runs on every
            // parameter edit -- including the ones a drag is making, so the
            // drag would cancel itself after its first move.
            if (paths == _graphPaths)
                return;
            _graphPaths = paths;
            _graph->setChannels(shown);
        }

        void CurveEditor::_channelsUpdate()
        {
            auto model = _model.lock();
            auto context = getContext();
            if (!model || !context)
                return;

            _channels = getParameters(model->getSystem());

            std::vector<std::string> animated;
            for (const auto& info : _channels)
            {
                if (core::Parameter::Type::Curve == info.parameter->getType())
                {
                    animated.push_back(info.getPath());
                }
            }

            // Nothing became animated or stopped being animated, so the rows
            // still say what they should. This runs on every step of a slider
            // drag, and rebuilding eight widgets each time would be felt.
            if (animated == _animated)
            {
                _graphUpdate();
                return;
            }
            _animated = animated;

            // Anything newly animated shows itself: keying a value and then
            // having to find it in a list is a step nobody wants.
            for (const auto& path : animated)
            {
                if (std::find(_shown.begin(), _shown.end(), path) == _shown.end())
                {
                    _shown.push_back(path);
                }
            }

            // Copied, because setParent(nullptr) erases from the very list
            // getChildren() returns a reference to.
            const auto children = _channelLayout->getChildren();
            for (const auto& child : children)
            {
                child->setParent(nullptr);
            }

            std::string group;
            for (const auto& info : _channels)
            {
                if (info.group != group)
                {
                    group = info.group;
                    auto label = Label::create(context, group, _channelLayout);
                    label->setTextRole(ColorRole::TextDisabled);
                }
                const std::string path = info.getPath();
                const bool isAnimated =
                    core::Parameter::Type::Curve == info.parameter->getType();
                auto checkBox = CheckBox::create(context, info.name, _channelLayout);
                checkBox->setChecked(
                    isAnimated &&
                    std::find(_shown.begin(), _shown.end(), path) != _shown.end());
                // A parameter with no curve has nothing to plot. The row is
                // there to say the channel exists, not to be switched on.
                checkBox->setEnabled(isAnimated);
                checkBox->setCheckedCallback(
                    [this, path](bool value)
                    {
                        auto i = std::find(_shown.begin(), _shown.end(), path);
                        if (value && i == _shown.end())
                        {
                            _shown.push_back(path);
                        }
                        else if (!value && i != _shown.end())
                        {
                            _shown.erase(i);
                        }
                        // Only what is plotted changes, so the rows this
                        // callback is running from are left alone.
                        _graphUpdate();
                    });
            }

            _graphUpdate();
        }
    }
}
