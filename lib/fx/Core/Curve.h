// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <cstddef>
#include <vector>

namespace fx
{
    namespace core
    {
        //! Key interpolation.
        enum class Interp
        {
            Step,
            Linear,

            //! Tangents taken from the neighbouring keys.
            Smooth,

            //! Tangents given by the key's own slopes. This is the mode the
            //! curve editor's handles will drag.
            //!
            //! Evaluated as a Hermite segment, which is a cubic Bezier whose
            //! handles sit a third of the way along the segment in time. Free
            //! handle timing needs a solve for t given the frame; there is
            //! nothing yet that can author it, so it is not written yet.
            Bezier,

            Count,
            First = Step
        };

        //! Curve behaviour outside the keyed range.
        enum class Infinity
        {
            //! Hold the end key's value.
            Constant,

            //! Continue at the end key's slope.
            Linear,

            //! Repeat the curve.
            Cycle,

            //! Repeat the curve, each repeat starting where the last ended.
            CycleOffset,

            //! Repeat the curve, alternating direction.
            Oscillate,

            Count,
            First = Constant
        };

        //! An animation curve key.
        struct Key
        {
            double frame = 0.0;
            float  value = 0.F;
            Interp interp = Interp::Smooth;

            //! Tangent slopes in value units per frame. Held separately so a
            //! key can be broken; the curve editor unifies them by writing
            //! both.
            float inSlope = 0.F;
            float outSlope = 0.F;

            bool operator == (const Key&) const;
            bool operator != (const Key&) const;
        };

        //! A time-domain animation curve.
        //!
        //! Evaluated once per frame, so evaluation cost does not matter and the
        //! interpolation can be whatever reads best.
        class Curve
        {
        public:
            //! Get the keys, in frame order.
            const std::vector<Key>& getKeys() const;

            //! Set the keys. They are sorted by frame, and where two keys share
            //! a frame the last one wins.
            void setKeys(const std::vector<Key>&);

            //! Add a key, replacing any key already at that frame. Returns the
            //! index the key ended up at.
            size_t addKey(const Key&);

            //! Remove a key.
            void removeKey(size_t index);

            //! Remove every key.
            void clear();

            //! \name Infinity
            ///@{

            Infinity getPreInfinity() const;
            Infinity getPostInfinity() const;
            void setPreInfinity(Infinity);
            void setPostInfinity(Infinity);

            ///@}

            //! Evaluate the curve. An empty curve is zero everywhere.
            float getValue(double frame) const;

            bool operator == (const Curve&) const;
            bool operator != (const Curve&) const;

        private:
            //! Evaluate within the keyed range, which the caller has already
            //! mapped the frame into.
            float _getValueInRange(double frame) const;

            //! Get a key's effective tangent slopes, which for a smooth key are
            //! not stored but taken from its neighbours.
            void _getSlopes(size_t index, float& in, float& out) const;

            std::vector<Key> _keys;
            Infinity _preInfinity = Infinity::Constant;
            Infinity _postInfinity = Infinity::Constant;
        };
    }
}
