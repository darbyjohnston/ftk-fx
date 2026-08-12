// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Parameter.h>

#include <ftk/Core/Matrix.h>

namespace fx
{
    namespace sim
    {
        //! Where something is, how it is turned, and how big it is.
        //!
        //! Built from Parameters like everything else, so a transform is
        //! animatable by construction rather than by a later retrofit -- which
        //! is the same argument §4a makes for the Parameter type itself.
        //!
        //! There is one of these on one emitter today. It exists now rather
        //! than when there are several because every one of those will need
        //! it, and because a thing with no transform has nowhere to put a
        //! manipulator.
        struct Transform
        {
            core::V3Parameter translate;

            //! Degrees, applied X then Y then Z.
            core::V3Parameter rotate;

            core::V3Parameter scale = core::V3Parameter(ftk::V3F(1.F, 1.F, 1.F));

            //! Scale, then rotate, then translate. For positions.
            ftk::M44F getMatrix(double frame) const;

            //! The rotation alone. For directions, which have no position to
            //! be moved and no size to be scaled.
            ftk::M44F getRotation(double frame) const;

            bool operator == (const Transform&) const;
            bool operator != (const Transform&) const;
        };
    }
}
