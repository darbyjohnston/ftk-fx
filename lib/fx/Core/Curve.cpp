// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Core/Curve.h>

#include <algorithm>
#include <cmath>

namespace fx
{
    namespace core
    {
        namespace
        {
            //! A tangent slope, guarding the zero-length span that two keys on
            //! the same frame would give.
            float slope(double dFrame, float dValue)
            {
                return dFrame != 0.0 ?
                    static_cast<float>(dValue / dFrame) :
                    0.F;
            }
        }

        bool Key::operator == (const Key& other) const
        {
            return
                frame == other.frame &&
                value == other.value &&
                interp == other.interp &&
                inSlope == other.inSlope &&
                outSlope == other.outSlope;
        }

        bool Key::operator != (const Key& other) const
        {
            return !(*this == other);
        }

        const std::vector<Key>& Curve::getKeys() const
        {
            return _keys;
        }

        void Curve::setKeys(const std::vector<Key>& value)
        {
            _keys = value;
            std::stable_sort(
                _keys.begin(),
                _keys.end(),
                [](const Key& a, const Key& b) { return a.frame < b.frame; });
            // Keep the last of any keys sharing a frame, so that setting a list
            // that repeats a frame behaves the same way as adding those keys
            // one at a time.
            auto i = std::unique(
                _keys.rbegin(),
                _keys.rend(),
                [](const Key& a, const Key& b) { return a.frame == b.frame; });
            _keys.erase(_keys.begin(), i.base());
        }

        size_t Curve::addKey(const Key& key)
        {
            auto i = std::lower_bound(
                _keys.begin(),
                _keys.end(),
                key.frame,
                [](const Key& a, double frame) { return a.frame < frame; });
            if (i != _keys.end() && i->frame == key.frame)
            {
                *i = key;
            }
            else
            {
                i = _keys.insert(i, key);
            }
            return i - _keys.begin();
        }

        void Curve::removeKey(size_t index)
        {
            if (index < _keys.size())
            {
                _keys.erase(_keys.begin() + index);
            }
        }

        void Curve::clear()
        {
            _keys.clear();
        }

        Infinity Curve::getPreInfinity() const
        {
            return _preInfinity;
        }

        Infinity Curve::getPostInfinity() const
        {
            return _postInfinity;
        }

        void Curve::setPreInfinity(Infinity value)
        {
            _preInfinity = value;
        }

        void Curve::setPostInfinity(Infinity value)
        {
            _postInfinity = value;
        }

        void Curve::_getSlopes(size_t index, float& in, float& out) const
        {
            const Key& key = _keys[index];
            switch (key.interp)
            {
            case Interp::Bezier:
                in = key.inSlope;
                out = key.outSlope;
                break;
            default:
            {
                // A smooth key takes its slope from the line through its
                // neighbours, and at an end from the one segment it has.
                const bool hasPrev = index > 0;
                const bool hasNext = index + 1 < _keys.size();
                float s = 0.F;
                if (hasPrev && hasNext)
                {
                    const Key& prev = _keys[index - 1];
                    const Key& next = _keys[index + 1];
                    s = slope(next.frame - prev.frame, next.value - prev.value);
                }
                else if (hasNext)
                {
                    const Key& next = _keys[index + 1];
                    s = slope(next.frame - key.frame, next.value - key.value);
                }
                else if (hasPrev)
                {
                    const Key& prev = _keys[index - 1];
                    s = slope(key.frame - prev.frame, key.value - prev.value);
                }
                in = s;
                out = s;
                break;
            }
            }
        }

        float Curve::_getValueInRange(double frame) const
        {
            // The segment containing the frame: the last key at or before it.
            auto i = std::upper_bound(
                _keys.begin(),
                _keys.end(),
                frame,
                [](double frame, const Key& b) { return frame < b.frame; });
            if (i == _keys.begin())
                return _keys.front().value;
            const size_t index = (i - _keys.begin()) - 1;
            if (index + 1 >= _keys.size())
                return _keys.back().value;

            const Key& a = _keys[index];
            const Key& b = _keys[index + 1];
            if (Interp::Step == a.interp)
                return a.value;

            const double dFrame = b.frame - a.frame;
            if (dFrame <= 0.0)
                return b.value;
            const float dValue = b.value - a.value;
            const float linear = slope(dFrame, dValue);

            // Each end of the segment contributes its own tangent. A linear or
            // stepped key contributes the segment's own slope, so a run of
            // linear keys comes out straight rather than nearly straight.
            float sa = linear;
            float sb = linear;
            if (Interp::Linear != a.interp && Interp::Step != a.interp)
            {
                float unused = 0.F;
                _getSlopes(index, unused, sa);
            }
            if (Interp::Linear != b.interp && Interp::Step != b.interp)
            {
                float unused = 0.F;
                _getSlopes(index + 1, sb, unused);
            }

            const double t = (frame - a.frame) / dFrame;
            const double t2 = t * t;
            const double t3 = t2 * t;
            const double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
            const double h10 = t3 - 2.0 * t2 + t;
            const double h01 = -2.0 * t3 + 3.0 * t2;
            const double h11 = t3 - t2;
            return static_cast<float>(
                h00 * a.value +
                h10 * dFrame * sa +
                h01 * b.value +
                h11 * dFrame * sb);
        }

        float Curve::getValue(double frame) const
        {
            if (_keys.empty())
                return 0.F;
            if (1 == _keys.size())
                return _keys.front().value;

            const Key& first = _keys.front();
            const Key& last = _keys.back();
            const double span = last.frame - first.frame;
            if (span <= 0.0)
                return last.value;

            const bool before = frame < first.frame;
            const bool after = frame > last.frame;
            if (!before && !after)
                return _getValueInRange(frame);

            switch (before ? _preInfinity : _postInfinity)
            {
            case Infinity::Linear:
            {
                // Take the end slope from the curve itself rather than from the
                // key, so that every interpolation mode extrapolates the way it
                // looks like it should -- including Step, which comes out flat.
                const double eps = 0.01;
                if (before)
                {
                    const float s = (_getValueInRange(first.frame + eps) - first.value) /
                        static_cast<float>(eps);
                    return first.value + static_cast<float>(frame - first.frame) * s;
                }
                const float s = (last.value - _getValueInRange(last.frame - eps)) /
                    static_cast<float>(eps);
                return last.value + static_cast<float>(frame - last.frame) * s;
            }
            case Infinity::Cycle:
            case Infinity::CycleOffset:
            case Infinity::Oscillate:
            {
                const double cycles = std::floor((frame - first.frame) / span);
                double mapped = frame - cycles * span;
                const Infinity mode = before ? _preInfinity : _postInfinity;
                if (Infinity::Oscillate == mode &&
                    std::fmod(std::abs(cycles), 2.0) >= 1.0)
                {
                    mapped = first.frame + last.frame - mapped;
                }
                float out = _getValueInRange(mapped);
                if (Infinity::CycleOffset == mode)
                {
                    out += static_cast<float>(cycles) * (last.value - first.value);
                }
                return out;
            }
            default:
                return before ? first.value : last.value;
            }
        }

        bool Curve::operator == (const Curve& other) const
        {
            return
                _keys == other._keys &&
                _preInfinity == other._preInfinity &&
                _postInfinity == other._postInfinity;
        }

        bool Curve::operator != (const Curve& other) const
        {
            return !(*this == other);
        }
    }
}
