// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <memory>

namespace ftk
{
    class Context;
}

namespace fx
{
    namespace app
    {
        //! Register the icons this application adds to feather-tk's.
        //!
        //! Kept here rather than pushed into the toolkit: a keyframe diamond
        //! means something to an animation tool and nothing to a file browser,
        //! and the icon system takes application icons for exactly this.
        void registerIcons(const std::shared_ptr<ftk::Context>&);
    }
}
