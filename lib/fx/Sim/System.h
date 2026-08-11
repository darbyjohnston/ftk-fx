// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Frame.h>
#include <fx/Core/Parameter.h>

#include <string>
#include <vector>

namespace fx
{
    namespace sim
    {
        //! The shape particles are born in.
        //!
        //! One emitter with a shape rather than an emitter class per shape:
        //! §6 lists the volumetric primitives as one kind, and they differ
        //! only in where a point inside them is. The emitters that will need
        //! their own implementations -- curve, geometry, texture driven,
        //! secondary -- differ in more than that, and can have one then.
        enum class EmitterShape
        {
            //! One place. A sphere of no size, and treated as one.
            Point,

            //! Radii rather than a radius, so it is an ellipsoid when they
            //! differ. Costs nothing and is worth having.
            Sphere,

            //! Half extents.
            Box,

            Count,
            First = Point
        };

        std::vector<std::string> getEmitterShapeLabels();
        std::string getLabel(EmitterShape);
        bool fromString(const std::string&, EmitterShape&);

        //! Whether a shape has an inside to emit from.
        bool hasVolume(EmitterShape);

        //! An emitter.
        //!
        //! Particles are born somewhere in a shape, moving in a cone about a
        //! direction. Every value here is a Parameter, so all of it is already
        //! animatable.
        struct Emitter
        {
            bool enabled = true;

            //! The seed for every random choice this emitter makes. Exposed
            //! because dialling a seed is how an artist gets a different
            //! version of the same effect.
            uint64_t seed = 1;

            EmitterShape shape = EmitterShape::Point;

            //! Born on the shape's surface rather than anywhere inside it.
            //! Meaningless for a point, which is all surface.
            bool surface = false;

            core::V3Parameter position;

            //! Radii for a sphere, half extents for a box, ignored for a
            //! point.
            core::V3Parameter size = core::V3Parameter(ftk::V3F(1.F, 1.F, 1.F));

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

            bool operator == (const Emitter&) const;
            bool operator != (const Emitter&) const;
        };

        //! The force fields acting on a system.
        struct Forces
        {
            core::V3Parameter gravity = core::V3Parameter(ftk::V3F(0.F, -9.8F, 0.F));

            //! Linear drag, as a fraction of speed removed per second.
            core::Parameter drag = core::Parameter(.1F);

            bool operator == (const Forces&) const;
            bool operator != (const Forces&) const;
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

            const Emitter& getEmitter() const;
            Emitter& getEmitter();

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

            //! Compares the recipe. Two systems that compare equal produce the
            //! same particles, which is what makes this the test for whether a
            //! scene has unsaved changes.
            bool operator == (const System&) const;
            bool operator != (const System&) const;

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
            Emitter _emitter;
            Forces _forces;
            int _substeps = 1;
        };
    }
}
