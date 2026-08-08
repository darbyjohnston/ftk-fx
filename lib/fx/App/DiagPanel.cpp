// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/DiagPanel.h>

#include <ftk/UI/DiagWidget.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        struct DiagPanel::Private
        {
            std::shared_ptr<DiagWidget> diagWidget;
        };

        void DiagPanel::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IPanel::_init(
                context,
                "Diagnostics",
                "Info",
                "fx::app::DiagPanel",
                parent);
            FTK_P();

            p.diagWidget = DiagWidget::create(context);
            p.diagWidget->setMarginRole(SizeRole::MarginSmall);
            _setWidget(p.diagWidget);
        }

        DiagPanel::DiagPanel() :
            _p(new Private)
        {}

        DiagPanel::~DiagPanel()
        {}

        std::shared_ptr<DiagPanel> DiagPanel::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<DiagPanel>(new DiagPanel);
            out->_init(context, parent);
            return out;
        }
    }
}
