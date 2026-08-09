// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/UI/IContainer.h>

namespace ftk
{
    class Icon;
    class Label;
    class ToolButton;
    class VerticalLayout;
}

namespace fx
{
    namespace app
    {
        //! Base class for the panels in the right hand column.
        //!
        //! A panel is a titled strip of controls that stays visible alongside
        //! whatever is in the viewports -- parameters being dialled while the
        //! sim is watched, diagnostics being watched while it is dialled. That
        //! is what makes it a column rather than one of the panes: a pane would
        //! have to be given up to see it.
        //!
        //! The panel provides the header and the frame; what goes inside is the
        //! subclass's business.
        class IPanel : public ftk::IContainer
        {
            FTK_NON_COPYABLE(IPanel);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::string& name,
                const std::string& icon,
                const std::string& objectName,
                const std::shared_ptr<IWidget>& parent);

            IPanel();

        public:
            virtual ~IPanel() = 0;

            const std::string& getPanelName() const;

            //! Set whether the header is shown. Hidden when the column is
            //! showing tabs, since the tab already names the panel and carries
            //! its own close button.
            void setHeaderVisible(bool);

            //! Set what the close button does. The panel does not know how the
            //! column keeps track of what is open, so it asks.
            void setCloseCallback(const std::function<void(void)>&);

        protected:
            //! Set the panel's contents, which go below the header. Named
            //! apart from IContainer::_setWidget, which is the frame this puts
            //! them in.
            void _setContent(const std::shared_ptr<ftk::IWidget>&);

        private:
            FTK_PRIVATE();
        };
    }
}
