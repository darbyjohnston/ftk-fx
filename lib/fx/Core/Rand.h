// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <cstdint>

namespace fx
{
    namespace core
    {
        //! \name Random Numbers
        //!
        //! Keyed rather than sequential: every random value is a pure function
        //! of a seed, a particle id, and a channel. Nothing is carried from
        //! frame to frame, so a particle draws the same numbers no matter which
        //! cached frame the re-simulation started from. A sequential generator
        //! would answer differently depending on where it was resumed, which is
        //! the one thing the cache cannot tolerate.
        //!
        //! The channel separates the draws made for one particle -- speed,
        //! direction, lifespan -- so that adding a draw does not shift the
        //! values the others get.
        ///@{

        //! Mix a 64-bit key down to 32 bits (the SplitMix64 finalizer).
        inline uint32_t randHash(uint64_t key)
        {
            key ^= key >> 30;
            key *= 0xbf58476d1ce4e5b9ULL;
            key ^= key >> 27;
            key *= 0x94d049bb133111ebULL;
            key ^= key >> 31;
            return static_cast<uint32_t>(key >> 32);
        }

        //! Get a random number in the range [0, 1).
        inline float randF(uint64_t seed, uint64_t id, uint32_t channel)
        {
            const uint32_t h = randHash(
                randHash(seed * 0x9e3779b97f4a7c15ULL + id) + channel);
            // 24 bits keeps every result exactly representable as a float.
            return (h >> 8) * (1.F / 16777216.F);
        }

        //! Get a random number in the given range.
        inline float randF(
            uint64_t seed,
            uint64_t id,
            uint32_t channel,
            float min,
            float max)
        {
            return min + randF(seed, id, channel) * (max - min);
        }

        ///@}
    }
}
