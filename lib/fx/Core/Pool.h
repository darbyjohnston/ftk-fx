// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/Core/Vector.h>

#include <cstdint>
#include <vector>

namespace fx
{
    namespace core
    {
        //! Particle pool.
        //!
        //! Structure of arrays: each attribute is one contiguous array, and a
        //! particle is an index into all of them rather than an object.
        //!
        //! The attributes are named members rather than a declared schema. The
        //! schema is coming, but its shape is a guess until there is a second
        //! set of attributes to hold, and a wrong guess would have to be
        //! re-understood every time anything touches the pool.
        //!
        //! A pool is a value: the cache keeps one copy per frame, and copying
        //! is how a frame is produced from the frame before it.
        class Pool
        {
        public:
            std::vector<uint64_t> id;
            std::vector<ftk::V3F> position;
            std::vector<ftk::V3F> velocity;
            std::vector<float>    age;
            std::vector<float>    lifespan;

            //! Tombstones. A dead particle keeps its slot until compact().
            std::vector<uint8_t>  alive;

            //! Get the number of slots, alive and tombstoned.
            size_t size() const;

            //! Get the number of live particles.
            size_t getAliveCount() const;

            //! Add particles at the end of the pool, returning the index of the
            //! first one added. New particles are alive, at the origin, with
            //! zero velocity and age, and no lifespan.
            size_t birth(size_t count);

            //! Tombstone a particle. Nothing moves, so indices taken before the
            //! call stay valid -- which is what lets a rule kill particles
            //! while it is walking the arrays.
            void kill(size_t index);

            //! Drop the tombstoned particles, keeping the rest in order. This
            //! is the point where indices change, and it is called once per
            //! frame rather than whenever something dies.
            void compact();

            //! Remove every particle and reset the id counter.
            void clear();

            //! Get the number of particles emitted since the last clear().
            //!
            //! This is also the next id, which is what makes ids stable: the
            //! count only ever goes up, so an id is never reused and always
            //! identifies the same particle across a re-simulation.
            uint64_t getEmittedCount() const;

            //! Get the memory used by the pool, for the cache's budget.
            size_t getByteCount() const;

        private:
            uint64_t _nextId = 1;
            size_t _aliveCount = 0;
        };
    }
}
