// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/EditorOptions.h>

#include <ftk/UI/IContainer.h>

#include <ftk/Core/Observable.h>

#include <array>

namespace fx
{
    namespace app
    {
        class Editor;
        class SceneModel;

        //! The main region: the editors and the arrangement they are in.
        //!
        //! Every editor slot is made once and kept, whether or not the current
        //! arrangement shows it, and changing the arrangement re-parents them
        //! into a new tree of splitters. That is why switching to four-up and
        //! back finds each editor showing what it was showing, from the camera
        //! it was left at, and why the OpenGL resources behind a viewport are
        //! not thrown away and rebuilt every time the artist changes their
        //! mind.
        class Editors : public ftk::IContainer
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            Editors() = default;

        public:
            virtual ~Editors();

            static std::shared_ptr<Editors> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            //! \name Layout
            ///@{

            EditorLayout getLayout() const;
            std::shared_ptr<ftk::IObservable<EditorLayout> > observeLayout() const;
            void setLayout(EditorLayout);

            ///@}

            //! \name Current Editor
            ///@{

            //! Get the editor the menu actions and the keyboard apply to. Never
            //! null, and always one the current arrangement shows.
            const std::shared_ptr<Editor>& getCurrent() const;

            //! Get an editor by slot, whether or not it is on screen.
            const std::shared_ptr<Editor>& getEditor(int) const;

            int getCurrentIndex() const;
            void setCurrentIndex(int);

            ///@}

            //! Set the point size in every viewport. Display settings are the
            //! same everywhere; it is the camera that differs between them.
            void setParticleSize(float);

            //! Set how the particles draw in every viewport.
            void setDrawType(DrawType);

        private:
            //! Tear down the tree of splitters and build the one the current
            //! arrangement calls for.
            void _layoutUpdate();

            std::array<std::shared_ptr<Editor>, editorCountMax> _editors;
            std::shared_ptr<ftk::Observable<EditorLayout> > _layout;
            int _currentIndex = 0;

            //! The root of the splitter tree, which is an editor itself when
            //! the arrangement is a single one.
            std::shared_ptr<ftk::IWidget> _root;
            std::shared_ptr<ftk::Observer<float> > _particleSizeObserver;
            std::shared_ptr<ftk::Observer<DrawType> > _drawTypeObserver;
        };
    }
}
