// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Viewport.h>

#include <fx/App/SceneModel.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/DrawUtil.h>

#include <ftk/GL/GL.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Math.h>
#include <ftk/Core/Matrix.h>
#include <ftk/Core/RenderUtil.h>

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
                    "void main()\n"
                    "{\n"
                    "    outColor = fColor;\n"
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

            _viewTypeComboBox = ComboBox::create(
                context,
                getViewTypeLabels(),
                shared_from_this());
            _viewTypeComboBox->setCurrentIndex(static_cast<int>(viewType));
            _viewTypeComboBox->setTooltip("The view this viewport looks through");
            _viewTypeComboBox->setIndexCallback(
                [this](int value)
                {
                    setViewType(static_cast<ViewType>(value));
                });

            _frameObserver = Observer<std::shared_ptr<const core::Frame> >::create(
                model->observeFrame(),
                [this](const std::shared_ptr<const core::Frame>& value)
                {
                    _frame = value;
                    _pointsDirty = true;
                    _doRender = true;
                    setDrawUpdate();
                });
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
            _viewTypeComboBox->setCurrentIndex(static_cast<int>(value));

            // Snap to the new view's orientation, but keep where the camera is
            // pointed and how far out it is. Switching from perspective to top
            // should look at the same part of the scene from above, not jump
            // back to the origin.
            _orbit = getViewOrbit(value);
            _doRender = true;
            setDrawUpdate();
        }

        void Viewport::frameView()
        {
            _orbit = getViewOrbit(_viewType);
            _center = V3F(0.F, 5.F, 0.F);
            _distance = 30.F;
            _orthoHeight = 30.F;
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

        float Viewport::getPointSize() const
        {
            return _pointSize;
        }

        void Viewport::setPointSize(float value)
        {
            if (value == _pointSize)
                return;
            _pointSize = value;
            _doRender = true;
            setDrawUpdate();
        }

        void Viewport::setCurrent(bool value)
        {
            if (value == _current)
                return;
            _current = value;
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
            // Orthographic views sit at the centre and let the clip planes do
            // the work, so there is no distance to pull back by.
            const float distance = isOrtho(_viewType) ? 0.F : _distance;
            return
                translate(V3F(0.F, 0.F, -distance)) *
                rotateX(_orbit.y) *
                rotateY(_orbit.x) *
                translate(-_center);
        }

        void Viewport::sizeHintEvent(const SizeHintEvent& event)
        {
            IWidget::sizeHintEvent(event);
            _margin = event.style->getSizeRole(
                SizeRole::MarginSmall,
                event.displayScale);
            _border = event.style->getSizeRole(
                SizeRole::Border,
                event.displayScale);
        }

        void Viewport::setGeometry(const Box2I& value)
        {
            const bool changed = value != getGeometry();
            IWidget::setGeometry(value);
            _doRender |= changed;

            const Size2I hint = _viewTypeComboBox->getSizeHint();
            _viewTypeComboBox->setGeometry(Box2I(
                value.min.x + _margin,
                value.min.y + _margin,
                hint.w,
                hint.h));
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
            _pointCount = _frame ? _frame->pool.size() : 0;
            if (0 == _pointCount)
                return;

            const core::Pool& pool = _frame->pool;
            std::vector<uint8_t> data(_pointCount * vertexByteCount);
            uint8_t* p = data.data();
            for (size_t i = 0; i < _pointCount; ++i)
            {
                const float lifespan = pool.lifespan[i];
                const float t = lifespan > 0.F ?
                    clamp(pool.age[i] / lifespan, 0.F, 1.F) :
                    0.F;
                writeVertex(p, pool.position[i], ageColor(t));
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
                        glEnable(GL_DEPTH_TEST);
                        _gridVao->bind();
                        _gridVao->draw(GL_LINES, 0, _gridCount);
                    }

                    if (_pointsVao && _pointCount > 0)
                    {
                        // Additive, and without writing depth: sparks and
                        // embers read as light rather than as surfaces, and it
                        // means the points do not have to be sorted.
                        _shader->setUniform("pointSize", _pointSize);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        glDepthMask(GL_FALSE);
#if defined(FTK_API_GL_4_1)
                        glEnable(GL_PROGRAM_POINT_SIZE);
#endif // FTK_API_GL_4_1
                        _pointsVao->bind();
                        _pointsVao->draw(GL_POINTS, 0, _pointCount);
#if defined(FTK_API_GL_4_1)
                        glDisable(GL_PROGRAM_POINT_SIZE);
#endif // FTK_API_GL_4_1
                        glDepthMask(GL_TRUE);
                    }
                    glDisable(GL_DEPTH_TEST);

                    // Put back what the renderer set up in begin(). It enables
                    // blending once for the whole frame and its primitives only
                    // ever set the blend function, so leaving blending off here
                    // draws every glyph after this widget as a solid box.
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

            // Mark the viewport the menu actions and the keyboard apply to.
            // With four of them on screen this is the difference between an
            // arrangement and a guess.
            if (_current)
            {
                event.render->drawMesh(
                    border(g, _border),
                    event.style->getColorRole(ColorRole::KeyFocus));
            }
        }

        void Viewport::mouseMoveEvent(MouseMoveEvent& event)
        {
            event.accept = true;
            const V2I d = event.pos - event.prev;

            // Middle drag pans, and so does the left button with the alt key,
            // for the sake of anyone on a trackpad with no middle button.
            const bool alt = _mouseModifiers & static_cast<int>(KeyModifier::Alt);
            if (MouseButton::Middle == _mouseButton ||
                (MouseButton::Left == _mouseButton && alt))
            {
                _pan(d);
            }
            else if (MouseButton::Left == _mouseButton &&
                !isOrtho(_viewType))
            {
                // Orbiting an axis view would turn it into something that is
                // no longer the front, so the axis views do not orbit at all.
                _setOrbit(_orbit + V2F(d.x * .25F, d.y * .25F));
            }
        }

        void Viewport::mousePressEvent(MouseClickEvent& event)
        {
            event.accept = true;
            takeKeyFocus();
            _mouseButton = event.button;
            _mouseModifiers = event.modifiers;
            if (_pressCallback)
            {
                _pressCallback();
            }
        }

        void Viewport::mouseReleaseEvent(MouseClickEvent& event)
        {
            event.accept = true;
            _mouseButton = MouseButton::None;
            _mouseModifiers = 0;
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
