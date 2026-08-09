// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/PaneOptions.h>

#include <ftk/UI/IWidget.h>

#include <map>

namespace ftk
{
    class Action;
    class MenuBar;
    class VerticalLayout;
}

namespace fx
{
    namespace app
    {
        class SceneModel;
        class Viewport;

        //! One slot in the main region, and whatever it is showing.
        //!
        //! The pane owns a header with a menu bar of its own: a Pane menu that
        //! changes what it shows, and a View menu for the camera when what it
        //! shows is a viewport. Per-content controls belong here rather than in
        //! the application's menu bar, because a menu bar whose contents depend
        //! on which pane was last clicked is a menu bar nobody can learn.
        //!
        //! Content is made the first time the pane is asked for it and then
        //! kept, so switching away and back finds a camera where it was left
        //! without every pane paying for every kind of content up front.
        class Pane : public ftk::IWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                ViewType,
                const std::shared_ptr<IWidget>& parent);

            Pane() = default;

        public:
            virtual ~Pane();

            static std::shared_ptr<Pane> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                ViewType = ViewType::Perspective,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            //! \name Content
            ///@{

            PaneType getPaneType() const;
            void setPaneType(PaneType);

            ViewType getViewType() const;

            //! Set what a viewport pane looks through. Remembered even while
            //! the pane is showing something else, so switching back to a view
            //! comes back to the one that was there.
            void setViewType(ViewType);

            //! Get the viewport, or null when the pane is not showing one. The
            //! view actions have to cope with that: a spreadsheet has no camera
            //! to frame.
            std::shared_ptr<Viewport> getViewport() const;

            ///@}

            //! Set the point size of this pane's viewport, whether or not it
            //! is the content on screen. A display setting should still be in
            //! force when a pane is switched back to.
            void setPointSize(float);

            //! Set whether this is the pane actions apply to. The current pane
            //! draws a border so that it is obvious which one that is.
            void setCurrent(bool);

            //! Set the callback for the pane being clicked in, which is how it
            //! becomes the current one.
            void setPressCallback(const std::function<void(void)>&);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;
            void drawOverlayEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;

        private:
            //! Get the content for a type, making it the first time.
            std::shared_ptr<IWidget> _getContent(PaneType);

            void _contentUpdate();

            //! Rebuild the header's menus for the current content. Rebuilt
            //! rather than hidden: ftk::MenuBar adds and clears menus but does
            //! not take one away, and a View menu left on a spreadsheet would
            //! be a menu that does nothing.
            void _menuUpdate();

            std::weak_ptr<SceneModel> _model;
            PaneType _paneType = PaneType::View;
            ViewType _viewType = ViewType::Perspective;
            std::map<PaneType, std::shared_ptr<IWidget> > _content;
            std::shared_ptr<Viewport> _viewport;

            bool _current = false;
            std::function<void(void)> _pressCallback;
            int _border = 0;
            float _pointSize = 3.F;

            std::shared_ptr<ftk::MenuBar> _menuBar;
            std::map<PaneType, std::shared_ptr<ftk::Action> > _paneTypeActions;
            std::map<ViewType, std::shared_ptr<ftk::Action> > _viewTypeActions;
            std::shared_ptr<ftk::VerticalLayout> _layout;
        };
    }
}
