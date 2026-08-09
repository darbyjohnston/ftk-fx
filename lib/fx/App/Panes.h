// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/PaneOptions.h>

#include <ftk/UI/IContainer.h>

#include <ftk/Core/Observable.h>

#include <array>

namespace fx
{
    namespace app
    {
        class Pane;
        class SceneModel;

        //! The main region: the panes and the arrangement they are in.
        //!
        //! Every pane slot is made once and kept, whether or not the current
        //! arrangement shows it, and changing the arrangement re-parents them
        //! into a new tree of splitters. That is why switching to four-up and
        //! back finds each pane showing what it was showing, from the camera it
        //! was left at, and why the OpenGL resources behind a viewport are not
        //! thrown away and rebuilt every time the artist changes their mind.
        class Panes : public ftk::IContainer
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            Panes() = default;

        public:
            virtual ~Panes();

            static std::shared_ptr<Panes> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            //! \name Layout
            ///@{

            PaneLayout getLayout() const;
            std::shared_ptr<ftk::IObservable<PaneLayout> > observeLayout() const;
            void setLayout(PaneLayout);

            ///@}

            //! \name Current Pane
            ///@{

            //! Get the pane the menu actions and the keyboard apply to. Never
            //! null, and always one the current arrangement shows.
            const std::shared_ptr<Pane>& getCurrent() const;

            //! Get a pane by slot, whether or not it is on screen.
            const std::shared_ptr<Pane>& getPane(int) const;

            int getCurrentIndex() const;
            void setCurrentIndex(int);

            ///@}

            //! Set the point size in every viewport. Display settings are the
            //! same everywhere; it is the camera that differs between them.
            void setPointSize(float);

        private:
            //! Tear down the tree of splitters and build the one the current
            //! arrangement calls for.
            void _layoutUpdate();

            std::array<std::shared_ptr<Pane>, paneCountMax> _panes;
            std::shared_ptr<ftk::Observable<PaneLayout> > _layout;
            int _currentIndex = 0;

            //! The root of the splitter tree, which is a pane itself when the
            //! arrangement is a single one.
            std::shared_ptr<ftk::IWidget> _root;
        };
    }
}
