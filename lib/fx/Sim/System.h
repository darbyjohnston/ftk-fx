// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Frame.h>
#include <fx/Core/Parameter.h>

#include <string>

namespace fx
{
    namespace sim
    {
        //! A point emitter.
        //!
        //! Particles are born at one place, in a cone about a direction. Every
        //! value here is a Parameter, so all of it is already animatable.
        struct PointEmitter
        {
            bool enabled = true;

            //! The seed for every random choice this emitter makes. Exposed
            //! because dialling a seed is how an artist gets a different
            //! version of the same effect.
            uint64_t seed = 1;

            core::V3Parameter position;

            //! Particles per second.
            core::Parameter rate = core::Parameter(200.F);

            core::V3Parameter direction = core::V3Parameter(ftk::V3F(0.F, 1.F, 0.F));

            //! Half-angle of the emission cone, in degrees.
            core::Parameter spread = core::Parameter(25.F);

            //! Initial speed, in units per second.
            core::Parameter speed = core::Parameter(6.F);

            //! Spread of the initial speed, as a fraction of it.
            core::Parameter speedVariance = core::Parameter(.25F);

            //! Lifespan, in seconds.
            core::Parameter lifespan = core::Parameter(3.F);

            //! Spread of the lifespan, as a fraction of it.
            core::Parameter lifespanVariance = core::Parameter(.25F);
        };

        //! The force fields acting on a system.
        struct Forces
        {
            core::V3Parameter gravity = core::V3Parameter(ftk::V3F(0.F, -9.8F, 0.F));

            //! Linear drag, as a fraction of speed removed per second.
            core::Parameter drag = core::Parameter(.1F);
        };

        //! A particle system: one pool's worth of emitters, fields, and rules.
        //!
        //! The system owns the recipe, not the particles. The particles live in
        //! the cache, one pool per frame, and the system is the pure function
        //! that turns one frame into the next. Keeping the state out of the
        //! system is what makes scrubbing, re-simulation, and locking work the
        //! same way -- they are all just "run step() from a frame I already
        //! have".
        class System
        {
        public:
            const std::string& getName() const;
            void setName(const std::string&);

            bool isEnabled() const;
            void setEnabled(bool);

            const PointEmitter& getEmitter() const;
            PointEmitter& getEmitter();

            const Forces& getForces() const;
            Forces& getForces();

            //! Get the number of solver steps per frame.
            int getSubsteps() const;
            void setSubsteps(int);

            //! Advance one frame. The result depends on nothing but the
            //! arguments -- see core::Frame.
            core::Frame step(
                const core::Frame& prev,
                int frame,
                double frameRate) const;

        private:
            //! Fill in a newly born particle. `subFrame` is the time the
            //! particle is born at, which is not a whole frame when there is
            //! more than one substep.
            void _birth(
                core::Pool&,
                size_t index,
                size_t count,
                double subFrame) const;

            std::string _name = "particles";
            bool _enabled = true;
            PointEmitter _emitter;
            Forces _forces;
            int _substeps = 1;
        };
    }
}
