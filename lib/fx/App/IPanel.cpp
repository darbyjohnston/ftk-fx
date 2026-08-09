// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/IPanel.h>

#include <ftk/UI/Divider.h>
#include <ftk/UI/Icon.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ToolButton.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        struct IPanel::Private
        {
            std::string name;
            std::function<void(void)> closeCallback;

            std::shared_ptr<Icon> icon;
            std::shared_ptr<Label> label;
            std::shared_ptr<ToolButton> closeButton;
            std::shared_ptr<IWidget> header;
            std::shared_ptr<IWidget> divider;
            std::shared_ptr<VerticalLayout> panelLayout;
            std::shared_ptr<VerticalLayout> layout;
        };

        void IPanel::_init(
            const std::shared_ptr<Context>& context,
            const std::string& name,
            const std::string& icon,
            const std::string& objectName,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, objectName, parent);
            FTK_P();
            p.name = name;

            if (!icon.empty())
            {
                p.icon = Icon::create(context, icon);
                p.icon->setMarginRole(SizeRole::MarginSmall);
            }

            p.label = Label::create(context, name);
            p.label->setMarginRole(SizeRole::MarginSmall);
            p.label->setHStretch(Stretch::Expanding);

            p.closeButton = ToolButton::create(context);
            p.closeButton->setIcon("Close");
            p.closeButton->setTooltip("Close this panel");
            p.closeButton->setClickedCallback(
                [this]
                {
                    if (_p->closeCallback)
                    {
                        _p->closeCallback();
                    }
                });

            p.layout = VerticalLayout::create(context);
            p.layout->setSpacingRole(SizeRole::None);
            auto hLayout = HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(SizeRole::None);
            p.header = hLayout;
            // Coloured so each panel reads as its own thing in a stack of them,
            // rather than the column being one long run of controls.
            hLayout->setBackgroundRole(ColorRole::Header);
            if (p.icon)
            {
                p.icon->setParent(hLayout);
            }
            p.label->setParent(hLayout);
            p.closeButton->setParent(hLayout);
            p.divider = Divider::create(context, Orientation::Vertical, p.layout);
            p.panelLayout = VerticalLayout::create(context, p.layout);
            p.panelLayout->setSpacingRole(SizeRole::None);
            p.panelLayout->setHStretch(Stretch::Expanding);
            _setWidget(p.layout);
        }

        IPanel::IPanel() :
            _p(new Private)
        {}

        IPanel::~IPanel()
        {}

        const std::string& IPanel::getPanelName() const
        {
            return _p->name;
        }

        void IPanel::setHeaderVisible(bool value)
        {
            FTK_P();
            p.header->setVisible(value);
            p.divider->setVisible(value);
        }

        void IPanel::setCloseCallback(const std::function<void(void)>& value)
        {
            _p->closeCallback = value;
        }

        void IPanel::_setContent(const std::shared_ptr<IWidget>& value)
        {
            value->setParent(_p->panelLayout);
        }
    }
}
