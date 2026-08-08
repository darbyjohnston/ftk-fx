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
        namespace
        {
            // Random channels. One per draw, so that adding a draw later does
            // not shift the numbers the existing ones get and quietly change
            // every shot that was already approved.
            const uint32_t channelCone = 0;
            const uint32_t channelPhi = 1;
            const uint32_t channelSpeed = 2;
            const uint32_t channelLifespan = 3;

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

        const PointEmitter& System::getEmitter() const
        {
            return _emitter;
        }

        PointEmitter& System::getEmitter()
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
            const V3F origin = _emitter.position.getValue(subFrame);
            const V3F axis = normalize(_emitter.direction.getValue(subFrame));
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

                pool.position[i] = origin;
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
