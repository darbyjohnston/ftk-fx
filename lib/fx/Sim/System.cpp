// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Sim/System.h>

#include <fx/Core/Rand.h>

#include <ftk/Core/Math.h>

#include <cmath>

using namespace ftk;

namespace fx
{
    namespace sim
    {
        std::vector<std::string> getEmitterShapeLabels()
        {
            return { "Point", "Sphere", "Box" };
        }

        std::string getLabel(EmitterShape value)
        {
            const auto labels = getEmitterShapeLabels();
            const size_t i = static_cast<size_t>(value);
            return i < labels.size() ? labels[i] : std::string();
        }

        bool fromString(const std::string& name, EmitterShape& out)
        {
            const auto labels = getEmitterShapeLabels();
            for (size_t i = 0; i < labels.size(); ++i)
            {
                if (labels[i] == name)
                {
                    out = static_cast<EmitterShape>(i);
                    return true;
                }
            }
            return false;
        }

        bool hasVolume(EmitterShape value)
        {
            return EmitterShape::Point != value;
        }

        bool Emitter::operator == (const Emitter& other) const
        {
            return
                enabled == other.enabled &&
                seed == other.seed &&
                shape == other.shape &&
                surface == other.surface &&
                size == other.size &&
                transform == other.transform &&
                rate == other.rate &&
                spread == other.spread &&
                speed == other.speed &&
                speedVariance == other.speedVariance &&
                lifespan == other.lifespan &&
                lifespanVariance == other.lifespanVariance;
        }

        bool Emitter::operator != (const Emitter& other) const
        {
            return !(*this == other);
        }

        bool Forces::operator == (const Forces& other) const
        {
            return gravity == other.gravity && drag == other.drag;
        }

        bool Forces::operator != (const Forces& other) const
        {
            return !(*this == other);
        }

        bool System::operator == (const System& other) const
        {
            return
                _name == other._name &&
                _enabled == other._enabled &&
                _emitter == other._emitter &&
                _forces == other._forces &&
                _substeps == other._substeps;
        }

        bool System::operator != (const System& other) const
        {
            return !(*this == other);
        }

        namespace
        {
            // Random channels. One per draw, so that adding a draw later does
            // not shift the numbers the existing ones get and quietly change
            // every shot that was already approved.
            const uint32_t channelCone = 0;
            const uint32_t channelPhi = 1;
            const uint32_t channelSpeed = 2;
            const uint32_t channelLifespan = 3;
            const uint32_t channelShapeU = 4;
            const uint32_t channelShapeV = 5;
            const uint32_t channelShapeR = 6;
            const uint32_t channelShapeFace = 7;

            //! Where in a shape a particle is born, in units of the shape's
            //! size. Keyed on the particle's id like every other draw, so a
            //! re-simulation puts it in the same place.
            ftk::V3F shapeOffset(
                EmitterShape shape,
                bool surface,
                const ftk::V3F& size,
                uint64_t seed,
                uint64_t id)
            {
                switch (shape)
                {
                case EmitterShape::Sphere:
                {
                    // A direction spread evenly over the sphere rather than
                    // evenly over the angles, which would bunch at the poles.
                    const float z = core::randF(seed, id, channelShapeU, -1.F, 1.F);
                    const float phi = core::randF(
                        seed, id, channelShapeV, 0.F, ftk::pi2);
                    const float r = std::sqrt(std::max(0.F, 1.F - z * z));
                    const ftk::V3F dir(r * std::cos(phi), z, r * std::sin(phi));
                    // Even over a sphere. Scaled into an ellipsoid afterwards
                    // it is no longer even over that surface -- the stretched
                    // parts get fewer per unit area -- which is a refinement
                    // worth having when it matters and is not worth a
                    // rejection loop here.
                    if (surface)
                        return dir;
                    // The cube root spreads them evenly through the volume;
                    // a plain random radius would crowd the centre.
                    return dir * std::cbrt(core::randF(seed, id, channelShapeR));
                }
                case EmitterShape::Box:
                {
                    ftk::V3F out(
                        core::randF(seed, id, channelShapeU, -1.F, 1.F),
                        core::randF(seed, id, channelShapeV, -1.F, 1.F),
                        core::randF(seed, id, channelShapeR, -1.F, 1.F));
                    if (surface)
                    {
                        // Which pair of faces, weighted by their area, so a
                        // flat box puts most of its particles on the two large
                        // faces rather than a third of them on each pair. The
                        // point inside is then pushed out to that face.
                        const float areaX = size.y * size.z;
                        const float areaY = size.x * size.z;
                        const float areaZ = size.x * size.y;
                        const float total = areaX + areaY + areaZ;
                        if (total > 0.F)
                        {
                            const float pick = core::randF(
                                seed, id, channelShapeFace, 0.F, total);
                            if (pick < areaX)
                            {
                                out.x = out.x < 0.F ? -1.F : 1.F;
                            }
                            else if (pick < areaX + areaY)
                            {
                                out.y = out.y < 0.F ? -1.F : 1.F;
                            }
                            else
                            {
                                out.z = out.z < 0.F ? -1.F : 1.F;
                            }
                        }
                    }
                    return out;
                }
                default:
                    return ftk::V3F(0.F, 0.F, 0.F);
                }
            }

            //! Get any unit vector perpendicular to the given one.
            V3F perpendicular(const V3F& v)
            {
                // Cross with whichever axis the vector is least aligned to, so
                // the cross product is never near zero.
                const V3F axis = std::abs(v.x) < .9F ?
                    V3F(1.F, 0.F, 0.F) :
                    V3F(0.F, 1.F, 0.F);
                return normalize(cross(v, axis));
            }

            //! Get a direction within a cone, distributed evenly over the solid
            //! angle rather than over the angle -- otherwise the emission
            //! bunches up along the axis.
            V3F coneDirection(
                const V3F& axis,
                float halfAngle,
                float uCone,
                float uPhi)
            {
                const float cosMax = std::cos(deg2rad(clamp(halfAngle, 0.F, 180.F)));
                const float cosTheta = 1.F - uCone * (1.F - cosMax);
                const float sinTheta = std::sqrt(std::max(0.F, 1.F - cosTheta * cosTheta));
                const float phi = uPhi * pi2;
                const V3F u = perpendicular(axis);
                const V3F v = cross(axis, u);
                return
                    axis * cosTheta +
                    u * (sinTheta * std::cos(phi)) +
                    v * (sinTheta * std::sin(phi));
            }
        }

        const std::string& System::getName() const
        {
            return _name;
        }

        void System::setName(const std::string& value)
        {
            _name = value;
        }

        bool System::isEnabled() const
        {
            return _enabled;
        }

        void System::setEnabled(bool value)
        {
            _enabled = value;
        }

        const Emitter& System::getEmitter() const
        {
            return _emitter;
        }

        Emitter& System::getEmitter()
        {
            return _emitter;
        }

        const Forces& System::getForces() const
        {
            return _forces;
        }

        Forces& System::getForces()
        {
            return _forces;
        }

        int System::getSubsteps() const
        {
            return _substeps;
        }

        void System::setSubsteps(int value)
        {
            _substeps = std::max(1, value);
        }

        void System::_birth(
            core::Pool& pool,
            size_t index,
            size_t count,
            double subFrame) const
        {
            const M44F matrix = _emitter.transform.getMatrix(subFrame);
            const M44F rotation = _emitter.transform.getRotation(subFrame);
            const V3F size = _emitter.size.getValue(subFrame);
            const EmitterShape shape = _emitter.shape;
            const bool surface = _emitter.surface;
            // Up, turned by the emitter. An emitter with no rotation sprays
            // the way it always did.
            const V3F axis = normalize(rotation * V3F(0.F, 1.F, 0.F));
            const float spread = _emitter.spread.getValue(subFrame);
            const float speed = _emitter.speed.getValue(subFrame);
            const float speedVariance = _emitter.speedVariance.getValue(subFrame);
            const float lifespan = _emitter.lifespan.getValue(subFrame);
            const float lifespanVariance = _emitter.lifespanVariance.getValue(subFrame);
            const uint64_t seed = _emitter.seed;

            for (size_t i = index; i < index + count; ++i)
            {
                // Keyed on the particle's id rather than on its index, because
                // the index moves when the pool is compacted and the id never
                // does. This is what makes a re-simulation reproduce the run.
                const uint64_t id = pool.id[i];
                const V3F direction = coneDirection(
                    axis,
                    spread,
                    core::randF(seed, id, channelCone),
                    core::randF(seed, id, channelPhi));
                const float s = speed * (1.F + speedVariance *
                    core::randF(seed, id, channelSpeed, -1.F, 1.F));
                const float l = lifespan * (1.F + lifespanVariance *
                    core::randF(seed, id, channelLifespan, -1.F, 1.F));

                const V3F offset = shapeOffset(shape, surface, size, seed, id);
                pool.position[i] = matrix * V3F(
                    offset.x * size.x,
                    offset.y * size.y,
                    offset.z * size.z);
                pool.velocity[i] = direction * s;
                pool.age[i] = 0.F;
                pool.lifespan[i] = std::max(0.F, l);
            }
        }

        core::Frame System::step(
            const core::Frame& prev,
            int frame,
            double frameRate) const
        {
            core::Frame out = prev;
            if (!_enabled || frameRate <= 0.0)
                return out;

            const int substeps = std::max(1, _substeps);
            const double dt = 1.0 / frameRate / substeps;

            for (int substep = 0; substep < substeps; ++substep)
            {
                // The time this substep lands on. Parameters are sampled here
                // rather than at the frame, so an animated value moves smoothly
                // through the frame instead of jumping at its start.
                const double subFrame =
                    (frame - 1) + (substep + 1) / static_cast<double>(substeps);

                // Emit. The exact count is kept as a double and the pool's
                // integer count chases it, so a rate that is not a whole number
                // of particles per substep still comes out at that rate.
                if (_emitter.enabled)
                {
                    const float rate = std::max(0.F, _emitter.rate.getValue(subFrame));
                    out.emitted += rate * dt;
                    // A hair of slack before taking the whole number. A rate
                    // meant to be a whole number of particles per frame divides
                    // to something a bit under one, and without this the last
                    // bit of that division drops a particle now and then.
                    const uint64_t target = static_cast<uint64_t>(out.emitted + 1e-6);
                    const uint64_t have = out.pool.getEmittedCount();
                    if (target > have)
                    {
                        const size_t count = static_cast<size_t>(target - have);
                        const size_t index = out.pool.birth(count);
                        _birth(out.pool, index, count, subFrame);
                    }
                }

                // Integrate. Semi-implicit Euler: the position uses the new
                // velocity, which is stable enough here and is one line.
                const V3F gravity = _forces.gravity.getValue(subFrame);
                const float drag = std::max(0.F, _forces.drag.getValue(subFrame));
                const size_t size = out.pool.size();
                for (size_t i = 0; i < size; ++i)
                {
                    if (!out.pool.alive[i])
                        continue;
                    V3F& v = out.pool.velocity[i];
                    v = v + (gravity - v * drag) * static_cast<float>(dt);
                    out.pool.position[i] = out.pool.position[i] + v * static_cast<float>(dt);
                    out.pool.age[i] += static_cast<float>(dt);
                }

                // Kill. Tombstones only; the pool is compacted once the frame
                // is finished, so nothing here has to worry about indices
                // moving underneath it.
                for (size_t i = 0; i < size; ++i)
                {
                    if (out.pool.alive[i] && out.pool.age[i] >= out.pool.lifespan[i])
                    {
                        out.pool.kill(i);
                    }
                }
            }

            out.pool.compact();
            return out;
        }
    }
}
