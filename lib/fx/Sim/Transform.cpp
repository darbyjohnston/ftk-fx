// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Sim/Transform.h>

#include <ftk/Core/Math.h>

#include <cmath>

using namespace ftk;

namespace fx
{
    namespace sim
    {
        namespace
        {
            //! The same angle wound to within half a turn of another.
            float wind(float degrees, float near)
            {
                return degrees + std::round((near - degrees) / 360.F) * 360.F;
            }

            //! How far apart two sets of angles are, for choosing between
            //! them. Squared, because only the comparison is wanted.
            float angleDistance(const V3F& a, const V3F& b)
            {
                const V3F d(a.x - b.x, a.y - b.y, a.z - b.z);
                return d.x * d.x + d.y * d.y + d.z * d.z;
            }
        }

        M44F Transform::getRotation(double frame) const
        {
            const V3F r = rotate.getValue(frame);
            return rotateZ(r.z) * rotateY(r.y) * rotateX(r.x);
        }

        M44F Transform::getMatrix(double frame) const
        {
            return
                ftk::translate(translate.getValue(frame)) *
                getRotation(frame) *
                ftk::scale(scale.getValue(frame));
        }

        V3F eulerZYX(const M44F& m, const V3F& near)
        {
            const float toDegrees = 180.F / pi;
            V3F a, b;
            // Straight up or straight down, the first and last turns are
            // about the same axis and the rotation only decides their sum --
            // the gimbal lock three angles have always had. Given to the
            // last of them, so that what moves is the angle whose ring
            // reaches this pose.
            if (std::abs(m.get(2, 0)) > .999999F)
            {
                const bool up = m.get(2, 0) < 0.F;
                const float sum = up ?
                    -std::atan2(m.get(0, 1), m.get(1, 1)) :
                    std::atan2(-m.get(0, 1), m.get(1, 1));
                a = V3F(0.F, up ? 90.F : -90.F, sum * toDegrees);
                b = a;
            }
            else
            {
                const float y = std::asin(clamp(-m.get(2, 0), -1.F, 1.F));
                a = V3F(
                    std::atan2(m.get(2, 1), m.get(2, 2)) * toDegrees,
                    y * toDegrees,
                    std::atan2(m.get(1, 0), m.get(0, 0)) * toDegrees);
                // The other way to the same place: the middle angle taken
                // over the top instead of under it, with the two either side
                // turned half round to match.
                b = V3F(
                    std::atan2(-m.get(2, 1), -m.get(2, 2)) * toDegrees,
                    180.F - y * toDegrees,
                    std::atan2(-m.get(1, 0), -m.get(0, 0)) * toDegrees);
            }
            a = V3F(wind(a.x, near.x), wind(a.y, near.y), wind(a.z, near.z));
            b = V3F(wind(b.x, near.x), wind(b.y, near.y), wind(b.z, near.z));
            return angleDistance(a, near) <= angleDistance(b, near) ? a : b;
        }

        bool Transform::operator == (const Transform& other) const
        {
            return
                translate == other.translate &&
                rotate == other.rotate &&
                scale == other.scale;
        }

        bool Transform::operator != (const Transform& other) const
        {
            return !(*this == other);
        }
    }
}
