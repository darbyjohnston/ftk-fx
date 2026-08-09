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
            IContainer::_init(context, "fx::app::PanePlaceholder", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);
            setBackgroundRole(ColorRole::Well);

            auto layout = VerticalLayout::create(context);
            layout->setSpacingRole(SizeRole::SpacingSmall);
            layout->addSpacer(Stretch::Expanding);

            auto label = Label::create(context, getLabel(paneType), layout);
            label->setFont(FontType::Bold);
            label->setTextRole(ColorRole::TextDisabled);
            label->setHAlign(HAlign::Center);

            auto description = Label::create(
                context,
                getPaneTypeDescription(paneType),
                layout);
            description->setTextRole(ColorRole::TextDisabled);
            description->setHAlign(HAlign::Center);

            layout->addSpacer(Stretch::Expanding);
            _setWidget(layout);
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

    }
}
