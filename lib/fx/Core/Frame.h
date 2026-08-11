// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Pool.h>

#include <vector>

namespace fx
{
    namespace core
    {
        //! Everything one system needs from the frame before it.
        //!
        //! A system's frame is a pure function of this: give the solver the
        //! same SystemFrame and the same parameters and it produces the same
        //! next one, on any machine, whether it arrived here by playing forward
        //! or by being restored from the cache. Nothing else may be carried
        //! between frames -- no generator position, no counters living on the
        //! solver -- or re-simulating from a cached frame stops matching a run
        //! from the start, and then the cache is lying.
        struct SystemFrame
        {
            Pool pool;

            //! The exact number of particles emission has asked for so far,
            //! including the fraction of a particle left over at the end of the
            //! frame. The pool's integer emitted count chases this, so a rate
            //! that is not a whole number of particles per frame still emits at
            //! that rate rather than rounding down every frame.
            double emitted = 0.0;

            size_t getByteCount() const;
        };

        //! One frame of the whole scene: what every system holds at that frame.
        //!
        //! One pool per system rather than one pool with a system column, which
        //! is §16's resolved answer: the systems are solved independently and a
        //! shared pool would have every one of them paying for the others'
        //! births and deaths.
        //!
        //! Cached whole, rather than one cache per system. Editing any system
        //! drops the frame, so a scene with several systems re-simulates all of
        //! them for an edit that touched one. That is the coarse answer, and it
        //! is deliberate: §16 still has "layer caching granularity" open, and a
        //! cache per system would answer it by accident.
        struct Frame
        {
            std::vector<SystemFrame> systems;

            size_t getByteCount() const;

            //! The particles in every system.
            size_t getParticleCount() const;
        };
    }
}
