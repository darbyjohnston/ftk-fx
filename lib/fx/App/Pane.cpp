// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Pane.h>

#include <fx/App/PanePlaceholder.h>
#include <fx/App/Viewport.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/RowLayout.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void Pane::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            ViewType viewType,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "fx::app::Pane", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);

            _model = model;
            _viewType = viewType;

            _layout = VerticalLayout::create(context, shared_from_this());
            _layout->setSpacingRole(SizeRole::None);

            _headerLayout = HorizontalLayout::create(context, _layout);
            _headerLayout->setMarginRole(SizeRole::MarginInside);
            _headerLayout->setSpacingRole(SizeRole::SpacingTool);

            _paneTypeComboBox = ComboBox::create(
                context,
                getPaneTypeLabels(),
                _headerLayout);
            _paneTypeComboBox->setTooltip("What this pane shows");
            _paneTypeComboBox->setIndexCallback(
                [this](int value)
                {
                    setPaneType(static_cast<PaneType>(value));
                });

            _viewTypeComboBox = ComboBox::create(
                context,
                getViewTypeLabels(),
                _headerLayout);
            _viewTypeComboBox->setCurrentIndex(static_cast<int>(viewType));
            _viewTypeComboBox->setTooltip("The view this pane looks through");
            _viewTypeComboBox->setIndexCallback(
                [this](int value)
                {
                    setViewType(static_cast<ViewType>(value));
                });

            _headerLayout->addSpacer(Stretch::Expanding);

            _contentUpdate();
        }

        Pane::~Pane()
        {}

        std::shared_ptr<Pane> Pane::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            ViewType viewType,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Pane>(new Pane);
            out->_init(context, model, viewType, parent);
            return out;
        }

        PaneType Pane::getPaneType() const
        {
            return _paneType;
        }

        void Pane::setPaneType(PaneType value)
        {
            if (value == _paneType)
                return;
            _paneType = value;
            _paneTypeComboBox->setCurrentIndex(static_cast<int>(value));
            _contentUpdate();
        }

        ViewType Pane::getViewType() const
        {
            return _viewType;
        }

        void Pane::setViewType(ViewType value)
        {
            if (value == _viewType)
                return;
            _viewType = value;
            _viewTypeComboBox->setCurrentIndex(static_cast<int>(value));
            if (_viewport)
            {
                _viewport->setViewType(value);
            }
        }

        std::shared_ptr<Viewport> Pane::getViewport() const
        {
            // Null unless a viewport is what is on screen. A viewport that has
            // been made but switched away from is not what the view actions
            // mean by "the current one".
            return PaneType::View == _paneType ? _viewport : nullptr;
        }

        std::shared_ptr<IWidget> Pane::_getContent(PaneType paneType)
        {
            auto i = _content.find(paneType);
            if (i != _content.end())
                return i->second;

            auto context = getContext();
            std::shared_ptr<IWidget> content;
            if (PaneType::View == paneType)
            {
                auto model = _model.lock();
                _viewport = Viewport::create(context, model, _viewType);
                // The content accepts the click, so the pane never sees it.
                // The viewport passes it back up rather than the pane trying to
                // intercept what its own content is handling.
                _viewport->setPressCallback(
                    [this]
                    {
                        if (_pressCallback)
                        {
                            _pressCallback();
                        }
                    });
                _viewport->setPointSize(_pointSize);
                content = _viewport;
            }
            else
            {
                content = PanePlaceholder::create(context, paneType);
            }
            return _content[paneType] = content;
        }

        void Pane::_contentUpdate()
        {
            auto content = _getContent(_paneType);
            for (const auto& i : _content)
            {
                // Detached rather than hidden, so a pane holds one content in
                // its layout at a time and the rest cost nothing to lay out.
                i.second->setParent(i.second == content ? _layout : nullptr);
            }
            _viewTypeComboBox->setVisible(PaneType::View == _paneType);
        }

        void Pane::setPointSize(float value)
        {
            _pointSize = value;
            if (_viewport)
            {
                _viewport->setPointSize(value);
            }
        }

        void Pane::setCurrent(bool value)
        {
            if (value == _current)
                return;
            _current = value;
            setDrawUpdate();
        }

        void Pane::setPressCallback(const std::function<void(void)>& value)
        {
            _pressCallback = value;
        }

        Size2I Pane::getSizeHint() const
        {
            return _layout->getSizeHint();
        }

        void Pane::sizeHintEvent(const SizeHintEvent& event)
        {
            IWidget::sizeHintEvent(event);
            _border = event.style->getSizeRole(
                SizeRole::Border,
                event.displayScale);
        }

        void Pane::setGeometry(const Box2I& value)
        {
            IWidget::setGeometry(value);
            _layout->setGeometry(value);
        }

        void Pane::drawEvent(const Box2I& drawRect, const DrawEvent& event)
        {
            IWidget::drawEvent(drawRect, event);
            // Mark the pane the menu actions and the keyboard apply to. With
            // four of them on screen this is the difference between an
            // arrangement and a guess.
            if (_current)
            {
                event.render->drawMesh(
                    border(getGeometry(), _border),
                    event.style->getColorRole(ColorRole::KeyFocus));
            }
        }

        void Pane::mousePressEvent(MouseClickEvent& event)
        {
            // Not accepted: the content below wants the click too, and this
            // only needs to know that the pane was the one clicked in.
            if (_pressCallback)
            {
                _pressCallback();
            }
        }
    }
}
