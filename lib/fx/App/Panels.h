// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/UI/IWidget.h>

#include <ftk/Core/ObservableList.h>

#include <map>

namespace ftk
{
    class ScrollWidget;
    class VerticalLayout;
}

namespace fx
{
    namespace app
    {
        class IPanel;
        class SceneModel;
        class Panes;

        //! The right hand column of panels.
        //!
        //! A stack of panels rather than a tab per panel, because the reason
        //! these are a column and not one of the panes is that they are wanted
        //! at the same time as each other: watching frame time while dragging a
        //! slider is the point, and tabs would make it one or the other.
        //!
        //! One scroll area for the whole stack rather than one inside each
        //! panel, so a panel takes the height its contents need instead of an
        //! equal share of the column, and there is a single scroll bar rather
        //! than one nested inside another.
        //!
        //! Every panel is made up front and shown or hidden. There are few of
        //! them and they are cheap; making them on demand is machinery to be
        //! written when one of them is not.
        class Panels : public ftk::IWidget
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

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _add(const std::shared_ptr<IPanel>&);
            void _openUpdate();

            std::vector<std::string> _names;
            std::map<std::string, std::shared_ptr<IPanel> > _panels;
            std::shared_ptr<ftk::ObservableList<std::string> > _open;
            std::shared_ptr<ftk::VerticalLayout> _layout;
            std::shared_ptr<ftk::ScrollWidget> _scrollWidget;
        };
    }
}
