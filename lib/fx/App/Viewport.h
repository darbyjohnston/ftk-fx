// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

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

        //! The viewport.
        //!
        //! Particles are drawn as points, coloured by how far through their
        //! life they are, over a ground grid that gives the fall somewhere to
        //! fall to.
        class Viewport : public ftk::IWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            Viewport() = default;

        public:
            virtual ~Viewport();

            static std::shared_ptr<Viewport> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            //! \name View
            ///@{

            void frameView();
            void zoomIn();
            void zoomOut();

            float getPointSize() const;
            void setPointSize(float);

            ///@}

            void setGeometry(const ftk::Box2I&) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;
            void mouseReleaseEvent(ftk::MouseClickEvent&) override;
            void scrollEvent(ftk::ScrollEvent&) override;

        private:
            void _setOrbit(const ftk::V2F&);
            void _setDistance(float);
            ftk::M44F _getMVP() const;

            //! Build the point vertex buffer from the frame. Rebuilt whole each
            //! frame: at these counts it costs less than tracking what changed,
            //! and there is nothing to get wrong.
            void _pointsUpdate();

            void _gridUpdate();

            std::shared_ptr<const core::Frame> _frame;
            float _pointSize = 3.F;

            ftk::V2F _orbit = ftk::V2F(35.F, 20.F);
            float _distance = 30.F;
            ftk::V3F _center = ftk::V3F(0.F, 5.F, 0.F);
            float _fov = 45.F;
            ftk::MouseButton _mouseButton = ftk::MouseButton::None;

            bool _doRender = true;
            bool _pointsDirty = true;
            std::shared_ptr<ftk::gl::Shader> _shader;
            std::shared_ptr<ftk::gl::VBO> _pointsVbo;
            std::shared_ptr<ftk::gl::VAO> _pointsVao;
            size_t _pointCount = 0;
            std::shared_ptr<ftk::gl::VBO> _gridVbo;
            std::shared_ptr<ftk::gl::VAO> _gridVao;
            size_t _gridCount = 0;
            std::shared_ptr<ftk::gl::OffscreenBuffer> _buffer;

            std::shared_ptr<ftk::Observer<std::shared_ptr<const core::Frame> > >
                _frameObserver;
        };
    }
}
