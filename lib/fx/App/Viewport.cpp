// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Viewport.h>

#include <fx/App/SceneModel.h>


#include <ftk/GL/GL.h>
#include <ftk/GL/Util.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Math.h>
#include <ftk/Core/Matrix.h>
#include <ftk/Core/RenderUtil.h>
#include <ftk/UI/DrawUtil.h>

#include <algorithm>
#include <array>
#include <cstring>

using namespace ftk;

namespace fx
{
    namespace app
    {
        namespace
        {
            const size_t vertexByteCount = 16;

            // Half float rather than the full float the offscreen default asks
            // for. The viewport blends additively, so it does want headroom
            // above one -- but not four bytes a channel to hold it, and §10 has
            // already settled on half float for everything that leaves the
            // application. The colour buffers are the largest thing the
            // viewports allocate, and this halves them.
            //
            // GLES has no half float offscreen target, so there it keeps
            // whatever the default is.
#if defined(FTK_API_GL_4_1)
            const gl::TextureType offscreenColorType = gl::TextureType::RGBA_F16;
#else
            const gl::TextureType offscreenColorType = gl::offscreenColorDefault;
#endif // FTK_API_GL_4_1

            const float gridExtent = 12.F;
            const int gridLines = 25;

            //! How far the faint negative stub runs, as a fraction of the
            //! positive axis.
            const float axisStub = .4F;

            std::string vertexSource()
            {
                return
                    "#version 410\n"
                    "\n"
                    "layout(location = 0) in vec3 vPos;\n"
                    "layout(location = 1) in vec4 vColor;\n"
                    "out vec4 fColor;\n"
                    "\n"
                    "uniform mat4 mvp;\n"
                    "uniform float pointSize;\n"
                    "\n"
                    "void main()\n"
                    "{\n"
                    "    gl_Position = mvp * vec4(vPos, 1.0);\n"
                    "    gl_PointSize = pointSize;\n"
                    "    fColor = vColor;\n"
                    "}\n";
            }

            std::string fragmentSource()
            {
                return
                    "#version 410\n"
                    "\n"
                    "in vec4 fColor;\n"
                    "out vec4 outColor;\n"
                    "\n"
                    "uniform float pointSize;\n"
                    // 0 draws whatever this is flat -- the grid, where
                    // gl_PointCoord means nothing. 1 cuts a point back to the
                    // disc inside it. 2 shades that disc as a sphere.
                    "uniform int drawType;\n"
                    "\n"
                    "void main()\n"
                    "{\n"
                    "    float a = 1.0;\n"
                    "    vec3 rgb = fColor.rgb;\n"
                    "    if (drawType > 0)\n"
                    "    {\n"
                    // A point is a square, which is plain to see by the time it
                    // is a few pixels across. Cut it back to the disc inside,
                    // and fade the last pixel of the edge rather than stepping
                    // it: unsoftened, the disc looks worse than the square did.
                    "        vec2 p = (gl_PointCoord - vec2(0.5)) * 2.0;\n"
                    "        float r2 = dot(p, p);\n"
                    "        float aa = 2.0 / max(pointSize, 1.0);\n"
                    "        a = 1.0 - smoothstep(1.0 - aa, 1.0, sqrt(r2));\n"
                    "        if (a <= 0.0) discard;\n"
                    "        if (drawType > 1)\n"
                    "        {\n"
                    // The impostor: the disc is the silhouette of a unit
                    // sphere, so the surface normal falls out of where in the
                    // disc this pixel is. Lit from the camera with a little
                    // wrap, which reads as volume without needing a light in
                    // the scene or a depth pass to sort them.
                    "            vec3 n = vec3(p, sqrt(max(0.0, 1.0 - r2)));\n"
                    "            float d = clamp(n.z * 0.85 + 0.15, 0.0, 1.0);\n"
                    "            rgb *= d;\n"
                    "        }\n"
                    "    }\n"
                    "    outColor = vec4(rgb, fColor.a * a);\n"
                    "}\n";
            }

            //! Write one vertex into a vertex buffer.
            void writeVertex(uint8_t*& p, const V3F& pos, const Color4F& color)
            {
                float* pf = reinterpret_cast<float*>(p);
                pf[0] = pos.x;
                pf[1] = pos.y;
                pf[2] = pos.z;
                p += 3 * sizeof(float);
                *p++ = static_cast<uint8_t>(clamp(color.r, 0.F, 1.F) * 255.F);
                *p++ = static_cast<uint8_t>(clamp(color.g, 0.F, 1.F) * 255.F);
                *p++ = static_cast<uint8_t>(clamp(color.b, 0.F, 1.F) * 255.F);
                *p++ = static_cast<uint8_t>(clamp(color.a, 0.F, 1.F) * 255.F);
            }

            //! Colour by how far through its life a particle is. This is a
            //! display choice rather than a simulated attribute -- the pool has
            //! no colour yet, and being able to see age is worth more at this
            //! stage than being able to author it.
            Color4F ageColor(float t)
            {
                const Color4F young(1.F, .92F, .62F);
                const Color4F middle(1.F, .48F, .13F);
                const Color4F old(.42F, .07F, .04F);
                const Color4F a = t < .5F ? young : middle;
                const Color4F b = t < .5F ? middle : old;
                const float u = t < .5F ? t * 2.F : (t - .5F) * 2.F;
                return Color4F(
                    lerp(u, a.r, b.r),
                    lerp(u, a.g, b.g),
                    lerp(u, a.b, b.b),
                    1.F - t * t);
            }
        }

        void Viewport::_init(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            ViewType viewType,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "fx::app::Viewport", parent);
            setHStretch(Stretch::Expanding);
            setVStretch(Stretch::Expanding);

            _viewType = viewType;
            _orbit = getViewOrbit(viewType);
            _model = model;

            _frameObserver = Observer<std::shared_ptr<const core::Frame> >::create(
                model->observeFrame(),
                [this](const std::shared_ptr<const core::Frame>& value)
                {
                    _frame = value;
                    _pointsDirty = true;
                    _doRender = true;
                    setDrawUpdate();
                });

            // The manipulator sits on a transform that can be animated, belong
            // to a different system from one moment to the next, and be moved
            // by the panel while the viewport is only watching. All three move
            // it, and none of them change the particles on their own, so they
            // ask for a draw rather than a render.
            _currentFrameObserver = Observer<int>::create(
                model->observeCurrentFrame(),
                [this](int value)
                {
                    _currentFrame = value;
                    setDrawUpdate();
                });
            _currentSystemObserver = Observer<size_t>::create(
                model->observeCurrentSystem(),
                [this](size_t) { setDrawUpdate(); });
            _parameterObserver = Observer<int>::create(
                model->observeParameterChanged(),
                [this](int) { setDrawUpdate(); });
        }

        Viewport::~Viewport()
        {}

        std::shared_ptr<Viewport> Viewport::create(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<SceneModel>& model,
            ViewType viewType,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Viewport>(new Viewport);
            out->_init(context, model, viewType, parent);
            return out;
        }

        ViewType Viewport::getViewType() const
        {
            return _viewType;
        }

        void Viewport::setViewType(ViewType value)
        {
            if (value == _viewType)
                return;
            _viewType = value;

            // Snap to the new view's orientation, but keep where the camera is
            // pointed and how far out it is. Switching from perspective to top
            // should look at the same part of the scene from above, not jump
            // back to the origin.
            _orbit = getViewOrbit(value);
            _doRender = true;
            setDrawUpdate();
        }

        bool Viewport::_getBounds(V3F& min, V3F& max) const
        {
            if (!_frame)
                return false;
            bool any = false;
            for (const auto& system : _frame->systems)
            {
                const core::Pool& pool = system.pool;
                for (size_t i = 0; i < pool.size(); ++i)
                {
                    if (!pool.alive[i])
                        continue;
                    const V3F& p = pool.position[i];
                    if (!any)
                    {
                        min = p;
                        max = p;
                        any = true;
                    }
                    else
                    {
                        min.x = std::min(min.x, p.x);
                        min.y = std::min(min.y, p.y);
                        min.z = std::min(min.z, p.z);
                        max.x = std::max(max.x, p.x);
                        max.y = std::max(max.y, p.y);
                        max.z = std::max(max.z, p.z);
                    }
                }
            }
            return any;
        }

        void Viewport::frameView()
        {
            V3F min, max;
            if (!_getBounds(min, max))
            {
                // Nothing alive to frame -- before the first particle is born,
                // or after the last one dies. The grid is what is on screen, so
                // it is what gets framed.
                min = V3F(-gridExtent, 0.F, -gridExtent);
                max = V3F(gridExtent, 0.F, gridExtent);
            }
            _center = (min + max) / 2.F;

            const float aspect = aspectRatio(getGeometry().size());
            const float margin = 1.1F;
            if (isOrtho(_viewType))
            {
                // What the box measures on screen, which for an orthographic
                // view is exact: the corners go through the same rotation the
                // camera uses and the extent is read off the result. A plume
                // seen from above is wide and shallow, and framing it as a
                // sphere would zoom out to fit a height that is not on screen.
                const M44F rotation = rotateX(_orbit.y) * rotateY(_orbit.x);
                V2F extent(0.F, 0.F);
                for (int i = 0; i < 8; ++i)
                {
                    const V3F corner(
                        (i & 1) ? max.x : min.x,
                        (i & 2) ? max.y : min.y,
                        (i & 4) ? max.z : min.z);
                    const V3F p = rotation * (corner - _center);
                    extent.x = std::max(extent.x, std::abs(p.x));
                    extent.y = std::max(extent.y, std::abs(p.y));
                }
                // The projection takes the height and derives the width from
                // the aspect, so an editor taller than it is wide has to grow
                // the height to fit the same width across.
                _orthoHeight = 2.F * margin *
                    std::max(std::max(extent.y, extent.x / aspect), .5F);
            }
            else
            {
                // A sphere for the perspective view, because that one orbits:
                // a box that just fits seen face on does not when seen corner
                // on, and a fit that changes as the camera moves is worse than
                // one that is slightly loose.
                const float radius = std::max(length(max - min) / 2.F, 1.F);
                // The narrower of the two half angles decides how far back the
                // camera has to be.
                const float vHalf = deg2rad(_fov) / 2.F;
                const float hHalf = std::atan(std::tan(vHalf) * aspect);
                _distance = radius * margin / std::sin(std::min(vHalf, hHalf));
            }

            _doRender = true;
            setDrawUpdate();
        }

        void Viewport::zoomIn()
        {
            _setZoom(.9F);
        }

        void Viewport::zoomOut()
        {
            _setZoom(1.1F);
        }

        DrawType Viewport::getDrawType() const
        {
            return _drawType;
        }

        void Viewport::setDrawType(DrawType value)
        {
            if (value == _drawType)
                return;
            _drawType = value;
            _doRender = true;
            setDrawUpdate();
        }

        float Viewport::getParticleSize() const
        {
            return _particleSize;
        }

        void Viewport::setParticleSize(float value)
        {
            if (value == _particleSize)
                return;
            _particleSize = value;
            _doRender = true;
            setDrawUpdate();
        }

        void Viewport::setPressCallback(const std::function<void(void)>& value)
        {
            _pressCallback = value;
        }

        void Viewport::_setOrbit(const V2F& value)
        {
            V2F tmp;
            tmp.x = std::fmod(value.x, 360.F);
            tmp.y = clamp(value.y, -89.F, 89.F);
            if (tmp == _orbit)
                return;
            _orbit = tmp;
            _doRender = true;
            setDrawUpdate();
        }

        void Viewport::_setZoom(float scale)
        {
            if (isOrtho(_viewType))
            {
                _orthoHeight = clamp(_orthoHeight * scale, .01F, 10000.F);
            }
            else
            {
                _distance = clamp(_distance * scale, .1F, 10000.F);
            }
            _doRender = true;
            setDrawUpdate();
        }

        float Viewport::_getWorldPerPixel() const
        {
            const int h = getGeometry().h();
            if (h <= 0)
                return 0.F;
            const float height = isOrtho(_viewType) ?
                _orthoHeight :
                2.F * _distance * std::tan(deg2rad(_fov) / 2.F);
            return height / static_cast<float>(h);
        }

        void Viewport::_pan(const V2I& delta)
        {
            // The camera's right and up axes in world space. The view rotates
            // the world by pitch then yaw, so undoing it in the other order
            // turns a screen direction back into a world one.
            const M44F inverse = rotateY(-_orbit.x) * rotateX(-_orbit.y);
            const V3F right = inverse * V3F(1.F, 0.F, 0.F);
            const V3F up = inverse * V3F(0.F, 1.F, 0.F);

            // The scene follows the cursor, so the camera goes the other way.
            // Screen y counts downwards, which is why up is added rather than
            // subtracted.
            const float scale = _getWorldPerPixel();
            _center = _center -
                right * (delta.x * scale) +
                up * (delta.y * scale);
            _doRender = true;
            setDrawUpdate();
        }

        M44F Viewport::_getProjection() const
        {
            const Size2I size = getGeometry().size();
            const float aspect = aspectRatio(size);
            if (isOrtho(_viewType))
            {
                // The clip planes are wide open rather than fitted to the
                // scene: an orthographic view has no perspective to lose to a
                // sloppy depth range, and nothing here needs the precision.
                const float h = _orthoHeight / 2.F;
                const float w = h * aspect;
                return ortho(-w, w, -h, h, -10000.F, 10000.F);
            }
            return perspective(_fov, aspect, .1F, 10000.F);
        }

        M44F Viewport::_getView() const
        {
            // Orthographic editors sit at the centre and let the clip planes do
            // the work, so there is no distance to pull back by.
            const float distance = isOrtho(_viewType) ? 0.F : _distance;
            return
                translate(V3F(0.F, 0.F, -distance)) *
                rotateX(_orbit.y) *
                rotateY(_orbit.x) *
                translate(-_center);
        }

        void Viewport::setGeometry(const Box2I& value)
        {
            const bool changed = value != getGeometry();
            IWidget::setGeometry(value);
            _doRender |= changed;
        }

        void Viewport::_gridUpdate()
        {
            std::vector<uint8_t> data;
            const size_t count = gridLines * 4;
            data.resize(count * vertexByteCount);
            uint8_t* p = data.data();
            const Color4F line(.28F, .28F, .30F);
            const Color4F axisX(.45F, .22F, .22F);
            const Color4F axisZ(.22F, .22F, .45F);
            for (int i = 0; i < gridLines; ++i)
            {
                const float t = i / static_cast<float>(gridLines - 1);
                const float v = lerp(t, -gridExtent, gridExtent);
                const bool axis = std::abs(v) < .001F;
                writeVertex(p, V3F(v, 0.F, -gridExtent), axis ? axisZ : line);
                writeVertex(p, V3F(v, 0.F,  gridExtent), axis ? axisZ : line);
                writeVertex(p, V3F(-gridExtent, 0.F, v), axis ? axisX : line);
                writeVertex(p, V3F( gridExtent, 0.F, v), axis ? axisX : line);
            }
            _gridVbo = gl::VBO::create(count, gl::VBOType::Pos3_F32_Color_U8);
            _gridVbo->copy(data);
            _gridVao = gl::VAO::create(_gridVbo->getType(), _gridVbo->getID());
            _gridCount = count;
        }

        void Viewport::_pointsUpdate()
        {
            _pointsDirty = false;
            _pointCount = _frame ? _frame->getParticleCount() : 0;
            if (0 == _pointCount)
                return;

            // Every system's particles into one buffer and one draw. They are
            // solved apart and drawn together: the points are additive and
            // unsorted, so which system a particle came from makes no
            // difference to what ends up on screen.
            std::vector<uint8_t> data(_pointCount * vertexByteCount);
            uint8_t* p = data.data();
            for (const auto& system : _frame->systems)
            {
                const core::Pool& pool = system.pool;
                for (size_t i = 0; i < pool.size(); ++i)
                {
                    const float lifespan = pool.lifespan[i];
                    const float t = lifespan > 0.F ?
                        clamp(pool.age[i] / lifespan, 0.F, 1.F) :
                        0.F;
                    writeVertex(p, pool.position[i], ageColor(t));
                }
            }

            // Grow the buffer in steps rather than to the exact count, so that
            // a particle count wandering up and down does not reallocate every
            // frame.
            const size_t capacity = _pointsVbo ? _pointsVbo->getSize() : 0;
            if (capacity < _pointCount)
            {
                size_t size = std::max<size_t>(1024, capacity);
                while (size < _pointCount)
                {
                    size *= 2;
                }
                _pointsVbo = gl::VBO::create(size, gl::VBOType::Pos3_F32_Color_U8);
                _pointsVao = gl::VAO::create(
                    _pointsVbo->getType(),
                    _pointsVbo->getID());
            }
            _pointsVbo->copy(data, 0, data.size());
        }

        bool Viewport::_project(const V3F& world, V2F& out) const
        {
            const Box2I& g = getGeometry();
            if (!g.isValid())
                return false;
            const V4F clip =
                _getProjection() * _getView() * V4F(world.x, world.y, world.z, 1.F);
            // Behind the camera the divide flips the point through the origin
            // and puts it on screen looking perfectly reasonable. Orthographic
            // views never do this; perspective ones do it the moment the
            // emitter goes behind the viewer.
            if (clip.w <= .0001F)
                return false;
            const V2F ndc(clip.x / clip.w, clip.y / clip.w);
            out = V2F(
                g.min.x + (ndc.x * .5F + .5F) * g.w(),
                // Screen y runs the other way to the camera's.
                g.min.y + (.5F - ndc.y * .5F) * g.h());
            return true;
        }

        M44F Viewport::_getViewInverse() const
        {
            // Built from the same parts as _getView(), reversed, rather than
            // inverted numerically: it is a rigid transform, so the inverse is
            // known exactly and costs nothing.
            const float distance = isOrtho(_viewType) ? 0.F : _distance;
            return
                translate(_center) *
                rotateY(-_orbit.x) *
                rotateX(-_orbit.y) *
                translate(V3F(0.F, 0.F, distance));
        }

        bool Viewport::_ray(const V2I& pos, V3F& origin, V3F& dir) const
        {
            const Box2I& g = getGeometry();
            if (!g.isValid())
                return false;
            const float x =
                (pos.x - g.min.x) / static_cast<float>(g.w()) * 2.F - 1.F;
            // Screen y runs the other way to the camera's.
            const float y =
                1.F - (pos.y - g.min.y) / static_cast<float>(g.h()) * 2.F;
            const float aspect = aspectRatio(g.size());
            const M44F inv = _getViewInverse();

            // Built from the camera rather than by unprojecting a point on the
            // far plane. Unprojecting is exact inside the frustum and nonsense
            // outside it: past the edge the homogeneous w changes sign and the
            // ray comes back pointing the other way. A drag is entitled to
            // leave the window, so the ray has to be defined out there too.
            //
            // As a tangent it is: the angle off the view axis approaches a
            // right angle as the cursor runs away and never passes it, which
            // is the property that was missing.
            if (isOrtho(_viewType))
            {
                const float h = _orthoHeight / 2.F;
                origin = inv * V3F(x * h * aspect, y * h, 0.F);
                const V3F ahead = inv * V3F(x * h * aspect, y * h, -1.F);
                dir = normalize(ahead - origin);
            }
            else
            {
                const float t = std::tan(deg2rad(_fov) / 2.F);
                origin = inv * V3F(0.F, 0.F, 0.F);
                const V3F ahead = inv * V3F(x * t * aspect, y * t, -1.F);
                dir = normalize(ahead - origin);
            }
            return true;
        }

        bool Viewport::_gizmoPlane(const V3F& axis, V3F& out) const
        {
            const Box2I& g = getGeometry();
            if (!g.isValid())
                return false;
            V3F origin, view;
            if (!_ray(
                V2I(g.min.x + g.w() / 2, g.min.y + g.h() / 2), origin, view))
                return false;
            // The plane holding the axis that most faces the camera, which is
            // the axis and whatever is left of the view direction once the
            // axis has been taken out of it. Nothing is left when the axis
            // points at the camera -- the case where there is no plane to drag
            // against, and no arm drawn to grab either.
            const V3F n = view - axis * dot(axis, view);
            if (length(n) < .001F)
                return false;
            out = normalize(n);
            return true;
        }

        bool Viewport::_gizmoPlaneFacing(
            const V2I& pos,
            const V3F& axis,
            V3F& out) const
        {
            if (!_gizmoPlane(axis, out))
                return false;
            V3F rayOrigin, rayDir;
            if (!_ray(pos, rayOrigin, rayDir))
                return false;
            // Turned to face the ray that is grabbing, so that "the cursor has
            // swung round to the plane's edge" is one comparison afterwards
            // rather than a sign to keep track of.
            if (dot(rayDir, out) < 0.F)
            {
                out = out * -1.F;
            }
            return std::abs(dot(rayDir, out)) >= .08F;
        }

        bool Viewport::_axisParam(
            const V2I& pos,
            const V3F& point,
            const V3F& axis,
            const V3F& normal,
            float& out) const
        {
            V3F rayOrigin, rayDir;
            if (!_ray(pos, rayOrigin, rayDir))
                return false;

            // Where the cursor meets the plane, then how far along the axis
            // that is. Not the nearest point between the axis and the cursor's
            // line: that reads beautifully and is unusable, because its
            // denominator is 1 - (axis . ray)^2, which collapses wherever the
            // cursor happens to aim along the axis. That is not a far-fetched
            // corner -- it is a region of the viewport, and dragging into it
            // sends the answer to infinity and out the other side with its
            // sign flipped.
            //
            // The normal is turned to face the ray that grabbed the arm, so
            // this is positive at the press and falls as the cursor swings
            // round towards the plane's edge. Refused before it reaches it:
            // at the plane the ray meets it nowhere, and past the plane it
            // meets it behind the camera, which is what sends the emitter
            // backwards -- the arm appears to reverse when the cursor keeps
            // going the same way.
            //
            // Held rather than clamped. A manipulator that stops when the
            // cursor asks for something it cannot answer is one the artist can
            // recover from by coming back; one that guesses is not.
            const float denom = dot(rayDir, normal);
            if (denom < .08F)
                return false;
            const float s = dot(point - rayOrigin, normal) / denom;
            const V3F hit = rayOrigin + rayDir * s;
            out = dot(hit - point, axis);
            return true;
        }

        bool Viewport::_gizmoOrigin(V3F& out) const
        {
            auto model = _model.lock();
            if (!model)
                return false;
            const auto& transform = model->getSystem().getEmitter().transform;
            // The transform's own origin, which is its translation: rotation
            // and scale leave the point they turn about where it is.
            const double frame = _currentFrame;
            out = V3F(
                transform.translate.x.getValue(frame),
                transform.translate.y.getValue(frame),
                transform.translate.z.getValue(frame));
            return true;
        }

        V3F Viewport::_gizmoAxis(Arm arm)
        {
            switch (arm)
            {
            case Arm::X: return V3F(1.F, 0.F, 0.F);
            case Arm::Y: return V3F(0.F, 1.F, 0.F);
            case Arm::Z: return V3F(0.F, 0.F, 1.F);
            default: break;
            }
            return V3F(0.F, 0.F, 0.F);
        }

        std::array<Viewport::ArmScreen, 3> Viewport::_gizmoArms() const
        {
            std::array<ArmScreen, 3> out;
            V3F origin;
            if (!_gizmoOrigin(origin))
                return out;
            V2F o;
            if (!_project(origin, o))
                return out;

            const std::array<V3F, 3> axes =
            {
                V3F(1.F, 0.F, 0.F),
                V3F(0.F, 1.F, 0.F),
                V3F(0.F, 0.F, 1.F)
            };
            for (size_t i = 0; i < axes.size(); ++i)
            {
                // A unit along the axis, projected. Its length on screen is
                // the scale a drag needs, and its direction is the arm: both
                // come out of the same projection, so they cannot disagree
                // about which way the axis points.
                V2F p;
                if (!_project(origin + axes[i], p))
                    continue;
                const V2F d = p - o;
                const float length = std::sqrt(d.x * d.x + d.y * d.y);
                // An arm pointing at or away from the camera lands on a few
                // pixels, where a direction is noise and a drag along it would
                // fly. Left invalid: not drawn, not grabbable.
                if (length < 1.F)
                    continue;
                out[i].valid = true;
                out[i].origin = o;
                out[i].dir = V2F(d.x / length, d.y / length);
                out[i].pixelsPerUnit = length;
                out[i].tip = V2F(
                    o.x + out[i].dir.x * _size.gizmo,
                    o.y + out[i].dir.y * _size.gizmo);
            }
            return out;
        }

        namespace
        {
            //! How far a point is from a segment, in pixels.
            float distanceToSegment(const V2F& p, const V2F& a, const V2F& b)
            {
                const V2F ab(b.x - a.x, b.y - a.y);
                const float lengthSquared = ab.x * ab.x + ab.y * ab.y;
                float t = 0.F;
                if (lengthSquared > 0.F)
                {
                    t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSquared;
                    t = clamp(t, 0.F, 1.F);
                }
                const V2F near(a.x + ab.x * t, a.y + ab.y * t);
                const V2F d(p.x - near.x, p.y - near.y);
                return std::sqrt(d.x * d.x + d.y * d.y);
            }
        }

        Viewport::Arm Viewport::_gizmoPick(const V2I& pos) const
        {
            const auto arms = _gizmoArms();
            const V2F p(pos.x, pos.y);
            Arm out = Arm::None;
            float best = static_cast<float>(_size.grab);
            for (size_t i = 0; i < arms.size(); ++i)
            {
                if (!arms[i].valid)
                    continue;
                const float d = distanceToSegment(p, arms[i].origin, arms[i].tip);
                // Nearest wins rather than first, so the arm on top where two
                // cross is the one that grabs.
                if (d <= best)
                {
                    best = d;
                    out = static_cast<Arm>(i + 1);
                }
            }
            return out;
        }

        void Viewport::_gizmoDraw(
            const Box2I& g,
            const Box2I& drawRect,
            const DrawEvent& event)
        {
            const auto arms = _gizmoArms();
            bool any = false;
            for (const auto& arm : arms)
            {
                any |= arm.valid;
            }
            if (!any)
                return;

            const ClipRectEnabledState clipRectEnabledState(event.render);
            const ClipRectState clipRectState(event.render);
            event.render->setClipRectEnabled(true);
            event.render->setClipRect(intersect(g, drawRect));

            const std::array<Color4F, 3> colors =
            {
                Color4F(.92F, .30F, .30F),
                Color4F(.40F, .84F, .36F),
                Color4F(.36F, .54F, .95F)
            };

            LineOptions lineOptions;
            lineOptions.width = _size.line;

            // While an arm is held, its line carries on across the viewport.
            // The emitter can only travel along it, so anything the pointer
            // does at a right angle to it is dropped -- and without the line
            // there is nothing on screen saying so once the pointer is past
            // the end of a ninety pixel arm. It reads as the manipulator
            // falling behind rather than as the pointer having left the rail.
            if (Arm::None != _gizmoDrag)
            {
                const size_t i = static_cast<size_t>(_gizmoDrag) - 1;
                if (arms[i].valid)
                {
                    const float reach =
                        static_cast<float>(g.w() + g.h());
                    Color4F rail = colors[i];
                    rail.a = .25F;
                    event.render->drawLine(
                        V2F(
                            arms[i].origin.x - arms[i].dir.x * reach,
                            arms[i].origin.y - arms[i].dir.y * reach),
                        V2F(
                            arms[i].origin.x + arms[i].dir.x * reach,
                            arms[i].origin.y + arms[i].dir.y * reach),
                        rail,
                        lineOptions);
                }
            }

            for (size_t i = 0; i < arms.size(); ++i)
            {
                if (!arms[i].valid)
                    continue;
                const Arm arm = static_cast<Arm>(i + 1);
                // White while it is the one being dragged or aimed at, so the
                // arm says what will move before the mouse goes down.
                const bool lit = arm == _gizmoDrag ||
                    (Arm::None == _gizmoDrag && arm == _gizmoHover);
                const Color4F color = lit ?
                    Color4F(1.F, 1.F, 1.F) :
                    colors[i];
                event.render->drawLine(
                    arms[i].origin, arms[i].tip, color, lineOptions);
                event.render->drawMesh(
                    circle(
                        V2I(
                            static_cast<int>(arms[i].tip.x),
                            static_cast<int>(arms[i].tip.y)),
                        _size.dot),
                    color);
            }
        }

        void Viewport::_axisDraw(
            const Box2I& g,
            const Box2I& drawRect,
            const DrawEvent& event)
        {
            // Drawn over the top of the buffer rather than inside it, so
            // nothing else is keeping it within the widget.
            const ClipRectEnabledState clipRectEnabledState(event.render);
            const ClipRectState clipRectState(event.render);
            event.render->setClipRectEnabled(true);
            event.render->setClipRect(intersect(g, drawRect));

            // The camera's rotation and nothing else: the tripod says which
            // way the scene is facing, not where it is or how far away.
            const M44F rotation = rotateX(_orbit.y) * rotateY(_orbit.x);
            const float length = _size.length;
            // Inset by a whole axis and a dot, not by the stub. Which way an
            // axis points depends on the camera, so any of the six arms can be
            // the one heading for the corner: in a top view it is the positive
            // Z, and at a stub's inset it ran out of the editor and over the
            // splitter below.
            const float reach = length + _size.dot;
            const V2F origin(
                g.min.x + _size.margin + reach,
                g.max.y - _size.margin - reach);

            struct Axis
            {
                V3F dir;
                Color4F color;
            };
            std::array<Axis, 3> axes =
            {
                Axis{ V3F(1.F, 0.F, 0.F), Color4F(.92F, .30F, .30F) },
                Axis{ V3F(0.F, 1.F, 0.F), Color4F(.40F, .84F, .36F) },
                Axis{ V3F(0.F, 0.F, 1.F), Color4F(.36F, .54F, .95F) }
            };
            std::array<V3F, 3> dir;
            for (size_t i = 0; i < axes.size(); ++i)
            {
                dir[i] = rotation * axes[i].dir;
            }

            // Furthest first, so the axis nearest the viewer is the one drawn
            // on top where they cross. The depth test used to do this.
            std::array<size_t, 3> order = { 0, 1, 2 };
            std::sort(
                order.begin(),
                order.end(),
                [&dir](size_t a, size_t b) { return dir[a].z < dir[b].z; });

            LineOptions lineOptions;
            lineOptions.width = _size.line;
            for (size_t i : order)
            {
                // Screen y runs the other way to the camera's.
                const V2F tip(
                    origin.x + dir[i].x * length,
                    origin.y - dir[i].y * length);
                const V2F stub(
                    origin.x - dir[i].x * length * axisStub,
                    origin.y + dir[i].y * length * axisStub);
                Color4F faint = axes[i].color;
                faint.a = .35F;
                event.render->drawLine(origin, stub, faint, lineOptions);
                event.render->drawLine(origin, tip, axes[i].color, lineOptions);
                event.render->drawMesh(
                    circle(
                        V2I(
                            static_cast<int>(tip.x),
                            static_cast<int>(tip.y)),
                        _size.dot),
                    axes[i].color);
            }
        }

        void Viewport::styleEvent(const StyleEvent& event)
        {
            IWidget::styleEvent(event);
            if (event.hasChanges())
            {
                _size.init = true;
            }
        }

        void Viewport::sizeHintEvent(const SizeHintEvent& event)
        {
            IWidget::sizeHintEvent(event);
            if (_size.init)
            {
                _size.init = false;
                // Resolved here because a draw event carries neither the style
                // nor the display scale.
                const int border = event.style->getSizeRole(
                    SizeRole::Border, event.displayScale);
                // Twice the border: a hairline reads as part of the grid
                // rather than as something laid over it. The dots stay where
                // they were -- they were already the right size, and following
                // the line width would only make them balls on sticks.
                _size.line = border * 2;
                _size.dot = border * 2;
                _size.length = event.style->getSizeRole(
                    SizeRole::Swatch, event.displayScale);
                _size.margin = event.style->getSizeRole(
                    SizeRole::Margin, event.displayScale);
                _size.gizmo = event.style->getSizeRole(
                    SizeRole::Handle, event.displayScale) * 6;
                _size.grab = event.style->getSizeRole(
                    SizeRole::Handle, event.displayScale);
            }
        }

        void Viewport::drawEvent(const Box2I& drawRect, const DrawEvent& event)
        {
            const Box2I& g = getGeometry();
            try
            {
                if (!_shader)
                {
                    _shader = gl::Shader::create(vertexSource(), fragmentSource());
                }
                if (!_gridVao)
                {
                    _gridUpdate();
                }
                if (_pointsDirty)
                {
                    _pointsUpdate();
                }

                const Size2I size = g.size();
                gl::OffscreenBufferOptions options;
#if defined(FTK_API_GL_4_1)
                options.depth = gl::OffscreenDepth::_24;
                options.stencil = gl::OffscreenStencil::_8;
#elif defined(FTK_API_GLES_2)
                options.stencil = gl::OffscreenStencil::_8;
#endif // FTK_API_GL_4_1
                if (gl::doCreate(_buffer, size, offscreenColorType, options))
                {
                    _buffer = gl::OffscreenBuffer::create(
                        size,
                        offscreenColorType,
                        options);
                    _doRender = true;
                }

                if (_doRender && _buffer)
                {
                    _doRender = false;
                    gl::OffscreenBufferBinding binding(_buffer);

                    // Everything this draw changes goes back on the way out.
                    // The renderer enables blending once for the whole frame
                    // and its primitives only set the blend function, so a
                    // widget that leaves blending off draws every glyph after
                    // it as a solid box.
                    const gl::StateSave stateSave;

                    const ViewportState viewportState(event.render);
                    const ClipRectEnabledState clipRectEnabledState(event.render);
                    const ClipRectState clipRectState(event.render);
                    const TransformState transformState(event.render);
                    const RenderSizeState renderSizeState(event.render);
                    event.render->setRenderSize(size);
                    event.render->setViewport(Box2I(0, 0, size.w, size.h));
                    event.render->setClipRectEnabled(false);

                    glClearColor(.11F, .11F, .12F, 1.F);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    const M44F mvp = _getProjection() * _getView();
                    _shader->bind();
                    _shader->setUniform("mvp", mvp);

                    if (_gridVao && _gridCount > 0)
                    {
                        _shader->setUniform("pointSize", 1.F);
                        _shader->setUniform("drawType", 0);
                        glEnable(GL_DEPTH_TEST);
                        _gridVao->bind();
                        _gridVao->draw(GL_LINES, 0, _gridCount);
                    }

                    if (_pointsVao && _pointCount > 0)
                    {
                        // Additive, and without writing depth: sparks and
                        // embers read as light rather than as surfaces, and it
                        // means the points do not have to be sorted.
                        _shader->setUniform("pointSize", _particleSize);
                        _shader->setUniform("drawType", DrawType::Sphere == _drawType ? 2 : 1);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        glDepthMask(GL_FALSE);
#if defined(FTK_API_GL_4_1)
                        glEnable(GL_PROGRAM_POINT_SIZE);
#endif // FTK_API_GL_4_1
                        _pointsVao->bind();
                        _pointsVao->draw(GL_POINTS, 0, _pointCount);
                    }

                }
            }
            catch (const std::exception& e)
            {
                if (auto context = getContext())
                {
                    context->log("fx::app::Viewport", e.what(), LogType::Error);
                }
            }

            if (_buffer)
            {
                event.render->drawTexture(_buffer->getColorID(), g, true);
            }

            // Over the top of the buffer rather than inside it, and through
            // the renderer rather than in OpenGL: the tripod is a screen space
            // overlay at a fixed size, which is the one part of this widget
            // that a two dimensional drawing API expresses well. The
            // manipulator is the same, and for the same reason -- it is
            // grabbed in pixels, so it may as well be drawn in them.
            _gizmoDraw(g, drawRect, event);
            _axisDraw(g, drawRect, event);
        }

        void Viewport::mouseMoveEvent(MouseMoveEvent& event)
        {
            event.accept = true;
            const V2I d = event.pos - event.prev;

            if (Arm::None != _gizmoDrag)
            {
                _gizmoMove(event.pos);
                return;
            }

            // Nothing held: light the arm the pointer is on, so that it is
            // clear what a press would grab before it grabs it.
            if (MouseButton::None == event.button)
            {
                const Arm hover = _gizmoPick(event.pos);
                if (hover != _gizmoHover)
                {
                    _gizmoHover = hover;
                    setDrawUpdate();
                }
                return;
            }

            // Middle drag pans, and so does the left button with the alt key,
            // for the sake of anyone on a trackpad with no middle button.
            const bool alt = event.modifiers & static_cast<int>(KeyModifier::Alt);
            if (MouseButton::Middle == event.button ||
                (MouseButton::Left == event.button && alt))
            {
                _pan(d);
            }
            else if (MouseButton::Left == event.button &&
                !isOrtho(_viewType))
            {
                // Orbiting an axis view would turn it into something that is
                // no longer the front, so the axis editors do not orbit at all.
                _setOrbit(_orbit + V2F(d.x * .25F, d.y * .25F));
            }
        }

        void Viewport::mousePressEvent(MouseClickEvent& event)
        {
            event.accept = true;
            takeKeyFocus();
            if (_pressCallback)
            {
                _pressCallback();
            }

            // The camera keeps the alt key and the middle button, so a
            // manipulator can only take a plain left press -- which means
            // grabbing an arm never costs a way to move the view.
            const bool alt = event.modifiers & static_cast<int>(KeyModifier::Alt);
            if (MouseButton::Left != event.button || alt)
                return;
            const Arm arm = _gizmoPick(event.pos);
            if (Arm::None == arm)
                return;
            V3F origin;
            if (!_gizmoOrigin(origin))
                return;
            // Where along the axis the cursor was when it grabbed. Everything
            // after this is measured against it, so the point that was grabbed
            // is the point that stays under the pointer -- rather than the
            // emitter's own origin jumping to the cursor on the first move.
            V3F normal;
            if (!_gizmoPlaneFacing(event.pos, _gizmoAxis(arm), normal))
                return;
            float t = 0.F;
            if (!_axisParam(event.pos, origin, _gizmoAxis(arm), normal, t))
                return;
            _gizmoDrag = arm;
            _gizmoHover = arm;
            _gizmoStart = origin;
            _gizmoNormal = normal;
            _gizmoT = t;
            if (auto model = _model.lock())
            {
                model->beginEdit();
            }
            setDrawUpdate();
        }

        void Viewport::mouseReleaseEvent(MouseClickEvent& event)
        {
            event.accept = true;
            if (Arm::None == _gizmoDrag)
                return;
            _gizmoDrag = Arm::None;
            if (auto model = _model.lock())
            {
                // Closed even when nothing moved: endEdit() records nothing
                // when the state matches, and leaving an edit open is how undo
                // goes quiet for the rest of the session.
                model->endEdit("Move System");
            }
            setDrawUpdate();
        }

        void Viewport::mouseLeaveEvent()
        {
            if (Arm::None != _gizmoHover)
            {
                _gizmoHover = Arm::None;
                setDrawUpdate();
            }
        }

        void Viewport::_gizmoMove(const V2I& pos)
        {
            auto model = _model.lock();
            if (!model || Arm::None == _gizmoDrag)
                return;

            // Against the axis the drag started on, which stays where it was
            // for the whole gesture. Solving against the arm where it is now
            // would be a loop: the answer moves the emitter, the emitter moves
            // the arm, and the next answer is measured against the arm the
            // last one moved.
            const V3F axis = _gizmoAxis(_gizmoDrag);
            float t = 0.F;
            if (!_axisParam(pos, _gizmoStart, axis, _gizmoNormal, t))
                return;
            const float distance = t - _gizmoT;

            V3F value = _gizmoStart;
            switch (_gizmoDrag)
            {
            case Arm::X: value.x = _gizmoStart.x + distance; break;
            case Arm::Y: value.y = _gizmoStart.y + distance; break;
            case Arm::Z: value.z = _gizmoStart.z + distance; break;
            default: break;
            }

            const sim::System before = model->getSystem();
            auto& translate = model->getSystem().getEmitter().transform.translate;
            const double frame = _currentFrame;
            switch (_gizmoDrag)
            {
            case Arm::X: setValue(translate.x, frame, value.x); break;
            case Arm::Y: setValue(translate.y, frame, value.y); break;
            case Arm::Z: setValue(translate.z, frame, value.z); break;
            default: break;
            }
            model->systemChanged("Move System", before);
        }

        void Viewport::scrollEvent(ScrollEvent& event)
        {
            event.accept = true;
            // Scaled rather than stepped, so that zooming in and back out
            // returns to where it started instead of drifting.
            _setZoom(std::pow(.9F, event.value.y));
        }
    }
}
