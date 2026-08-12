// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/EditorOptions.h>
#include <fx/App/ParameterList.h>

#include <fx/Core/Frame.h>

#include <ftk/UI/IWidget.h>

#include <ftk/GL/Mesh.h>
#include <ftk/GL/OffscreenBuffer.h>
#include <ftk/GL/Shader.h>

#include <array>

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
        //! what it looks through belongs to the Editor holding it, along with
        //! the menu that decides whether it is what the editor is showing at
        //! all.
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

            float getParticleSize() const;
            void setParticleSize(float);

            DrawType getDrawType() const;
            void setDrawType(DrawType);

            GizmoMode getGizmoMode() const;
            void setGizmoMode(GizmoMode);

            //! Set the callback for the viewport being clicked in, which is how
            //! its editor becomes the current one.
            void setPressCallback(const std::function<void(void)>&);

            //! An arm as it lands on screen: where it starts, where it ends,
            //! and how many pixels a world unit covers along it. The last is
            //! what turns a drag in pixels back into a distance in the scene,
            //! and it falls out of the projection rather than being derived
            //! again from the camera.
            struct ArmScreen
            {
                bool valid = false;
                ftk::V2F origin;
                ftk::V2F tip;
                ftk::V2F dir;
                float pixelsPerUnit = 0.F;
            };

            //! The three arms on screen, in the same pixel space as mouse
            //! positions. Empty valid flags where an arm points at the camera
            //! and has nowhere to go.
            //!
            //! Public so that a capture can write down where the arms actually
            //! landed. A drag shot aimed at an arm was authored by reading
            //! coordinates off a picture, which is guessing -- and a guess
            //! that misses the arm still produces a screenshot, so the shot
            //! looks taken while testing nothing.
            std::array<ArmScreen, 3> getGizmoArms() const;

            ///@}

            void setGeometry(const ftk::Box2I&) override;
            void styleEvent(const ftk::StyleEvent&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            void keyPressEvent(ftk::KeyEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;
            void mouseReleaseEvent(ftk::MouseClickEvent&) override;
            void mouseLeaveEvent() override;
            void scrollEvent(ftk::ScrollEvent&) override;

        private:
            //! Which arm of the manipulator something is on.
            enum class Arm
            {
                None,
                X,
                Y,
                Z
            };

            void _setOrbit(const ftk::V2F&);
            void _setZoom(float);
            void _pan(const ftk::V2I& delta);

            ftk::M44F _getProjection() const;
            ftk::M44F _getView() const;

            //! The camera's placement in the scene, which is _getView()
            //! undone.
            ftk::M44F _getViewInverse() const;

            //! Get the world distance one pixel covers, which is what makes a
            //! pan follow the cursor rather than merely go the right way.
            float _getWorldPerPixel() const;

            //! Build the point vertex buffer from the frame. Rebuilt whole each
            //! frame: at these counts it costs less than tracking what changed,
            //! and there is nothing to get wrong.
            void _pointsUpdate();

            //! The box the alive particles occupy at the current frame.
            //! False when there are none to put a box around.
            bool _getBounds(ftk::V3F& min, ftk::V3F& max) const;

            void _gridUpdate();
            //! The corner tripod, drawn in pixels rather than in the scene so
            //! that it keeps its size whatever the camera is doing.
            void _axisDraw(
                const ftk::Box2I& geometry,
                const ftk::Box2I& drawRect,
                const ftk::DrawEvent&);

            //! A point in the scene, in widget pixels. False when it is behind
            //! the camera, where a projection gives a point that is on screen
            //! and mirrored rather than off it.
            bool _project(const ftk::V3F& world, ftk::V2F& out) const;

            //! The line the cursor points along, in the scene. Orthographic
            //! views give a ray too -- every pixel has its own, they are just
            //! all parallel.
            bool _ray(const ftk::V2I& pos, ftk::V3F& origin, ftk::V3F& dir) const;

            //! The distance along an axis through a point that puts that
            //! point at the given pixel. False at the vanishing point, where
            //! no distance does.
            bool _axisDistance(
                const ftk::V3F& point,
                const ftk::V3F& axis,
                const ftk::V2F& target,
                bool useX,
                float& denom,
                float& out) const;

            //! The world axis an arm runs along.
            static ftk::V3F _gizmoAxis(Arm);

            //! The three directions the arms stand for, which are not the
            //! same in every mode. A translation is added outside the
            //! rotation and so runs along the world's axes; a scale is
            //! applied inside it and runs along the emitter's own, so an arm
            //! drawn along the world's would point one way while the emitter
            //! stretched another.
            std::array<ftk::V3F, 3> _gizmoArmAxes() const;

            //! Where the manipulator is: the current system's emitter, at the
            //! current frame. False when there is no model to ask.
            bool _gizmoOrigin(ftk::V3F& out) const;

            //! How far a ring stands from the origin, in the scene, so that
            //! it lands at about the arms' length on screen. One radius for
            //! all three, which is what makes them read as one ball rather
            //! than three unrelated ovals.
            float _gizmoRadius() const;

            //! A ring about an axis, as the screen segments that can be
            //! drawn. Sampled in the scene and projected point by point: a
            //! circle seen at an angle is an ellipse, and a circle crossing
            //! behind the camera is neither -- projecting the samples gets
            //! both without a special case for either, and drops the pieces
            //! that are behind the camera by leaving their segments out.
            std::vector<std::pair<ftk::V2F, ftk::V2F> > _gizmoRing(Arm) const;

            //! The arm or ring under the pointer, or None. Which of the two
            //! it looks at is the mode's only say in picking.
            Arm _gizmoPick(const ftk::V2I&) const;

            //! What the drag will be called on the undo stack.
            std::string _gizmoCommand() const;

            //! Take the current system to where the drag has taken it.
            void _gizmoMove(const ftk::V2I& pos);

            void _gizmoTranslate(const ftk::V2I& pos);
            void _gizmoRotate(const ftk::V2I& pos);
            void _gizmoScale(const ftk::V2I& pos);

            //! The manipulator, drawn over the buffer like the tripod.
            void _gizmoDraw(
                const ftk::Box2I& geometry,
                const ftk::Box2I& drawRect,
                const ftk::DrawEvent&);

            std::weak_ptr<SceneModel> _model;
            std::shared_ptr<const core::Frame> _frame;
            int _currentFrame = 1;

            //! The arm the pointer is over, and the one being dragged. Kept
            //! apart because a drag holds its arm however far the pointer
            //! wanders off it.
            Arm _gizmoHover = Arm::None;
            Arm _gizmoDrag = Arm::None;

            //! Where the drag started: the emitter's origin, and how far
            //! along the arm the cursor grabbed it. Everything after is
            //! measured against these rather than against the last move, so a
            //! drag cannot drift and cannot chase itself.
            ftk::V3F _gizmoStart;
            //! The arm's line on screen at the press, and where along it the
            //! pointer grabbed.
            ftk::V2F _gizmoScreen;
            ftk::V2F _gizmoDir;
            float _gizmoU = 0.F;

            //! Which screen axis the distance is solved against, and which
            //! side of the vanishing point the drag began on.
            bool _gizmoUseX = true;
            float _gizmoDenom = 0.F;

            GizmoMode _gizmoMode = GizmoMode::Translate;

            //! The transform the drag started from. Rotate and scale are
            //! measured against these the way translate is measured against
            //! the origin: from the press, never from the last move.
            ftk::V3F _gizmoStartRotate;
            ftk::V3F _gizmoStartScale = ftk::V3F(1.F, 1.F, 1.F);

            //! Where the pointer was around the ring at the last move, and
            //! how far it has been around in total. Kept as a running total
            //! rather than as the difference from the press so that passing
            //! the far side of the ring carries on turning instead of
            //! snapping back the other way.
            float _gizmoAnglePrev = 0.F;
            float _gizmoAngleTotal = 0.F;

            //! Which way a turn on screen turns the axis in the scene.
            //! Measured at the press by moving a point around the ring and
            //! watching which way it goes, rather than worked out from the
            //! camera: the answer depends on the projection, the handedness
            //! and which way the screen's y axis points, and any of the three
            //! is easy to get backwards.
            float _gizmoAngleSign = 1.F;

            float _particleSize = 3.F;
            DrawType _drawType = DrawType::Point;
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
            //! Resolved from the style, the way every other widget does it,
            //! so the tripod follows the display scale and anything the theme
            //! changes about these roles.
            struct SizeData
            {
                bool init = true;
                int line = 0;
                int dot = 0;
                int length = 0;
                int margin = 0;

                //! How long the manipulator's arms are and how near the
                //! pointer has to be to grab one. Longer than the tripod's:
                //! the tripod is read, and this is aimed at.
                int gizmo = 0;
                int grab = 0;
            };
            SizeData _size;
            std::shared_ptr<ftk::gl::OffscreenBuffer> _buffer;

            std::shared_ptr<ftk::Observer<std::shared_ptr<const core::Frame> > >
                _frameObserver;
            std::shared_ptr<ftk::Observer<int> > _currentFrameObserver;
            std::shared_ptr<ftk::Observer<size_t> > _currentSystemObserver;
            std::shared_ptr<ftk::Observer<int> > _parameterObserver;
        };
    }
}
