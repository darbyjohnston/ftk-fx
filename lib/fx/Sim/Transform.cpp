// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Sim/Transform.h>

using namespace ftk;

namespace fx
{
    namespace sim
    {
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
