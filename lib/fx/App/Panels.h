// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/UI/IContainer.h>

#include <ftk/Core/Observable.h>
#include <ftk/Core/ObservableList.h>

#include <map>

namespace ftk
{
    class ScrollWidget;
    class TabWidget;
    class VerticalLayout;
}

namespace fx
{
    namespace app
    {
        class IPanel;
        class Panes;
        class SceneModel;

        //! How the panel column presents its panels.
        enum class PanelStyle
        {
            //! Stacked, all of them visible at once.
            Column,

            //! One at a time, with a tab bar.
            Tabs,

            Count,
            First = Column
        };

        std::vector<std::string> getPanelStyleLabels();
        std::string getLabel(PanelStyle);

        //! The right hand column of panels.
        //!
        //! Stacked by default, because the reason these are a column and not
        //! one of the panes is that they are wanted at the same time as each
        //! other: watching frame time while dragging a slider is the point.
        //! Tabs are the other choice for when the column is narrow and one
        //! panel at full height beats two squeezed, which is a judgement only
        //! the person looking at it can make.
        //!
        //! Stacked, there is one scroll area for the whole column, so a panel
        //! takes the height its contents need rather than an equal share. In
        //! tabs each panel scrolls on its own, since only one is on screen.
        //!
        //! Every panel is made up front and shown or hidden. There are few of
        //! them and they are cheap; making them on demand is machinery to be
        //! written when one of them is not.
        class Panels : public ftk::IContainer
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<Panes>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            Panels() = default;

        public:
            virtual ~Panels();

            static std::shared_ptr<Panels> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<Panes>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            //! Get every panel's name, in the order they are stacked.
            const std::vector<std::string>& getPanelNames() const;

            //! \name Open Panels
            ///@{

            bool isOpen(const std::string&) const;
            std::shared_ptr<ftk::IObservableList<std::string> > observeOpen() const;
            void setOpen(const std::string&, bool);

            ///@}

            //! \name Style
            ///@{

            PanelStyle getStyle() const;
            std::shared_ptr<ftk::IObservable<PanelStyle> > observeStyle() const;
            void setStyle(PanelStyle);

            ///@}

            void tickEvent(bool, bool, const ftk::TickEvent&) override;

        private:
            void _add(const std::shared_ptr<IPanel>&);

            //! Put the open panels into whichever container the style calls
            //! for, and take the column away when there are none.
            //!
            //! Never called from a callback belonging to something it takes
            //! apart -- see _panelsDirty.
            void _panelsUpdate();

            //! Get a panel's scroll area for tabs, making it the first time.
            const std::shared_ptr<ftk::ScrollWidget>& _getScroll(
                const std::string&);

            //! Set when the containers need rebuilding, acted on at the next
            //! tick.
            //!
            //! Rebuilding destroys the tab bar's buttons, and closing a tab is
            //! one of the things that asks for it. ftk::IButton reads its own
            //! members after calling the clicked callback, so freeing the
            //! button from inside that callback leaves it reading freed memory.
            //! Waiting a tick puts the rebuild after the click is done.
            bool _panelsDirty = false;

            std::vector<std::string> _names;
            std::map<std::string, std::shared_ptr<IPanel> > _panels;
            std::map<std::string, std::shared_ptr<ftk::ScrollWidget> > _scrolls;
            std::shared_ptr<ftk::ObservableList<std::string> > _open;
            std::shared_ptr<ftk::Observable<PanelStyle> > _style;

            std::shared_ptr<ftk::VerticalLayout> _columnLayout;
            std::shared_ptr<ftk::ScrollWidget> _columnScroll;
            std::shared_ptr<ftk::TabWidget> _tabWidget;
        };
    }
}
