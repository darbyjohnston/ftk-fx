// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Pool.h>

namespace fx
{
    namespace core
    {
        //! Everything one frame of simulation needs from the frame before it.
        //!
        //! A frame is a pure function of this: give the solver the same Frame
        //! and the same parameters and it produces the same next Frame, on any
        //! machine, whether it arrived here by playing forward or by being
        //! restored from the cache. Nothing else may be carried between frames
        //! -- no generator position, no counters living on the solver -- or
        //! re-simulating from a cached frame stops matching a run from the
        //! start, and then the cache is lying.
        struct Frame
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
    }
}
