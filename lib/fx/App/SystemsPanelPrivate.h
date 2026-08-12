// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/SystemsPanel.h>

#include <ftk/UI/IMouseWidget.h>

namespace ftk
{
    class CheckBox;
    class HorizontalLayout;
    class Label;
}

namespace fx
{
    namespace app
    {
        //! One row of the systems list.
        //!
        //! Drawn the way feather-tk draws a list item rather than the way it
        //! draws a button: the whole row takes the colour, corner to corner,
        //! with nothing inset and nothing rounded. A row of buttons reads as
        //! several things that happen to be stacked; a list reads as one thing
        //! with a row picked out of it, and which of those the artist sees is
        //! decided entirely by whether the highlight reaches the edges.
        //!
        //! Not ftk::ListWidget itself, whose items are a string and a tooltip.
        //! A system row carries controls -- one today and more later -- so it
        //! has to be a widget rather than a value. What is borrowed is the
        //! drawing.
        class SystemRow : public ftk::IMouseWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::string& text,
                const std::shared_ptr<ftk::IWidget>& parent);

            SystemRow() = default;

        public:
            virtual ~SystemRow();

            static std::shared_ptr<SystemRow> create(
                const std::shared_ptr<ftk::Context>&,
                const std::string& text,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            //! Set whether this is the row the panels are following.
            void setCurrent(bool);

            //! Set whether the system is solved. Not setEnabled(),
            //! which IWidget already has and which greys a widget out.
            void setSolved(bool);

            //! Set the callback for the row being picked.
            void setCurrentCallback(const std::function<void(void)>&);

            //! Set the callback for the row's check box.
            void setSolvedCallback(const std::function<void(bool)>&);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;

        private:
            std::shared_ptr<ftk::Label> _label;
            std::shared_ptr<ftk::CheckBox> _checkBox;
            std::shared_ptr<ftk::HorizontalLayout> _layout;
            bool _current = false;
            std::function<void(void)> _currentCallback;
        };
    }
}
