// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Sim/System.h>

#include <string>
#include <vector>

namespace fx
{
    namespace app
    {
        //! One row of the parameter panel, and one channel of the curve
        //! editor.
        //!
        //! The range is interface knowledge rather than something the solver
        //! has an opinion about -- a rate of a million is meaningful, it is
        //! just not what a slider should span.
        struct ParameterInfo
        {
            std::string group;
            std::string name;

            //! Points into the model's system, which outlives every widget
            //! and is never replaced, only assigned to.
            core::Parameter* parameter = nullptr;

            float min = 0.F;
            float max = 1.F;

            //! The value a fresh scene has, for the reset button.
            float defaultValue = 0.F;

            //! "Emitter/Rate", which is what a curve is stored and looked up
            //! under.
            std::string getPath() const;
        };

        //! List every animatable parameter, in the order the panel shows them.
        //!
        //! One list rather than one per widget: the panel and the curve editor
        //! have to agree about what exists and what it is called, and the way
        //! to make two things agree is to not have two things.
        std::vector<ParameterInfo> getParameters(sim::System&);
    }
}
