// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/ViewOptions.h>

#include <ftk/UI/IWidget.h>

#include <ftk/Core/Observable.h>

#include <array>

namespace fx
{
    namespace app
    {
        class SceneModel;
        class Viewport;

        //! The viewports and the arrangement they are in.
        //!
        //! Every viewport is made once and kept, whether or not the current
        //! arrangement shows it, and changing the arrangement re-parents them
        //! into a new tree of splitters. That is why switching to four-up and
        //! back finds each camera where it was left, and why the OpenGL
        //! resources behind a viewport are not thrown away and rebuilt every
        //! time the artist changes their mind.
        class Views : public ftk::IWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            Views() = default;

        public:
            virtual ~Views();

            static std::shared_ptr<Views> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            //! \name Layout
            ///@{

            ViewLayout getLayout() const;
            std::shared_ptr<ftk::IObservable<ViewLayout> > observeLayout() const;
            void setLayout(ViewLayout);

            ///@}

            //! \name Current Viewport
            ///@{

            //! Get the viewport the menu actions and the keyboard apply to.
            //! Never null, and always one the current arrangement shows.
            const std::shared_ptr<Viewport>& getCurrent() const;

            //! Get a viewport by slot, whether or not it is on screen.
            const std::shared_ptr<Viewport>& getViewport(int) const;

            int getCurrentIndex() const;
            void setCurrentIndex(int);

            ///@}

            //! Set the point size in every viewport. Display settings are the
            //! same everywhere; it is the camera that differs between them.
            void setPointSize(float);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            //! Tear down the tree of splitters and build the one the current
            //! arrangement calls for.
            void _layoutUpdate();

            std::array<std::shared_ptr<Viewport>, viewCountMax> _viewports;
            std::shared_ptr<ftk::Observable<ViewLayout> > _layout;
            int _currentIndex = 0;

            //! The root of the splitter tree, which is a viewport itself when
            //! the arrangement is a single one.
            std::shared_ptr<ftk::IWidget> _root;
        };
    }
}
