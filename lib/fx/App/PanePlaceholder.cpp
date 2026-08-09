// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/PanePlaceholder.h>

#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        void PanePlaceholder::_init(
            const std::shared_ptr<Context>& context,
            PaneType paneType,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "fx::app::PanePlaceholder", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);
            setBackgroundRole(ColorRole::Well);

            _layout = VerticalLayout::create(context, shared_from_this());
            _layout->setSpacingRole(SizeRole::SpacingSmall);
            _layout->addSpacer(Stretch::Expanding);

            auto label = Label::create(context, getLabel(paneType), _layout);
            label->setFont(FontType::Bold);
            label->setTextRole(ColorRole::TextDisabled);
            label->setHAlign(HAlign::Center);

            auto description = Label::create(
                context,
                getPaneTypeDescription(paneType),
                _layout);
            description->setTextRole(ColorRole::TextDisabled);
            description->setHAlign(HAlign::Center);

            _layout->addSpacer(Stretch::Expanding);
        }

        PanePlaceholder::~PanePlaceholder()
        {}

        std::shared_ptr<PanePlaceholder> PanePlaceholder::create(
            const std::shared_ptr<Context>& context,
            PaneType paneType,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<PanePlaceholder>(new PanePlaceholder);
            out->_init(context, paneType, parent);
            return out;
        }

        Size2I PanePlaceholder::getSizeHint() const
        {
            return _layout->getSizeHint();
        }

        void PanePlaceholder::setGeometry(const Box2I& value)
        {
            IWidget::setGeometry(value);
            _layout->setGeometry(value);
        }
    }
}
