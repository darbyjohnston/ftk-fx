// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/PaneOptions.h>

#include <fx/Core/Frame.h>

#include <ftk/UI/IWidget.h>

#include <ftk/GL/Mesh.h>
#include <ftk/GL/OffscreenBuffer.h>
#include <ftk/GL/Shader.h>

namespace fx
{
    namespace app
    {
        class SceneModel;

        //! A viewport.
        //!
        //! Particles are drawn as points, coloured by how far through their
        //! life they are, over a ground grid that gives the fall somewhere to
        //! fall to.
        //!
        //! The viewport owns its camera and nothing else: the menu that chooses
        //! what it looks through belongs to the Pane holding it, along with the
        //! menu that decides whether it is what the pane is showing at all.
        class Viewport : public ftk::IWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                ViewType,
                const std::shared_ptr<ftk::IWidget>& parent);

            Viewport() = default;

        public:
            virtual ~Viewport();

            static std::shared_ptr<Viewport> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                ViewType = ViewType::Perspective,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            //! \name View
            ///@{

            ViewType getViewType() const;
            void setViewType(ViewType);

            //! Put the camera back where a new view of this type starts.
            void frameView();

            void zoomIn();
            void zoomOut();

            ///@}

            //! \name Display
            ///@{

            float getPointSize() const;
            void setPointSize(float);

            //! Set the callback for the viewport being clicked in, which is how
            //! its pane becomes the current one.
            void setPressCallback(const std::function<void(void)>&);

            ///@}

            void setGeometry(const ftk::Box2I&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;
            void scrollEvent(ftk::ScrollEvent&) override;

        private:
            void _setOrbit(const ftk::V2F&);
            void _setZoom(float);
            void _pan(const ftk::V2I& delta);

            ftk::M44F _getProjection() const;
            ftk::M44F _getView() const;

            //! Get the world distance one pixel covers, which is what makes a
            //! pan follow the cursor rather than merely go the right way.
            float _getWorldPerPixel() const;

            //! Build the point vertex buffer from the frame. Rebuilt whole each
            //! frame: at these counts it costs less than tracking what changed,
            //! and there is nothing to get wrong.
            void _pointsUpdate();

            void _gridUpdate();
            //! The corner tripod, drawn in pixels rather than in the scene so
            //! that it keeps its size whatever the camera is doing.
            void _axisDraw(const ftk::Box2I&, const ftk::DrawEvent&);

            std::shared_ptr<const core::Frame> _frame;
            float _pointSize = 3.F;
            std::function<void(void)> _pressCallback;

            ViewType _viewType = ViewType::Perspective;
            ftk::V2F _orbit = ftk::V2F(35.F, 20.F);
            ftk::V3F _center = ftk::V3F(0.F, 5.F, 0.F);

            //! Distance from the camera to the centre, in perspective.
            float _distance = 30.F;

            //! How much of the world the viewport covers vertically, in an
            //! orthographic view. This is the ortho equivalent of _distance,
            //! and each is kept while the other is in use so that switching
            //! back and forth does not lose the framing.
            float _orthoHeight = 30.F;

            float _fov = 45.F;


            bool _doRender = true;
            bool _pointsDirty = true;
            std::shared_ptr<ftk::gl::Shader> _shader;
            std::shared_ptr<ftk::gl::VBO> _pointsVbo;
            std::shared_ptr<ftk::gl::VAO> _pointsVao;
            size_t _pointCount = 0;
            std::shared_ptr<ftk::gl::VBO> _gridVbo;
            std::shared_ptr<ftk::gl::VAO> _gridVao;
            size_t _gridCount = 0;
            float _displayScale = 1.F;
            std::shared_ptr<ftk::gl::OffscreenBuffer> _buffer;

            std::shared_ptr<ftk::Observer<std::shared_ptr<const core::Frame> > >
                _frameObserver;
        };
    }
}
