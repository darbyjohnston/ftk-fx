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
        //! What the panel puts beside the number.
        enum class ParameterControl
        {
            //! A slider, for a value with a range worth showing: a rate runs
            //! from none to a lot, and where in that it sits means something.
            Slider,

            //! A shuttle, for a value with no range at all. A translation is
            //! as far as the artist drags it, and a slider can only offer a
            //! made-up span of scene units and then lie when the value leaves
            //! it. A shuttle asks how fast, not how far along.
            Shuttle
        };

        //! One row of the parameter panel, and one channel of the curve
        //! editor.
        //!
        //! The range is interface knowledge rather than something the solver
        //! has an opinion about -- a rate of a million is meaningful, it is
        //! just not what a slider should span. For a shuttle it is only a
        //! clamp, since there is no track for it to be a span of.
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

            ParameterControl control = ParameterControl::Slider;

            //! How much one notch of a shuttle is worth. Unused by a slider,
            //! which has its range to divide up instead.
            float step = .1F;

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

        //! Set what a parameter is worth at a frame.
        //!
        //! An animated parameter gets a key at that frame; a constant one is
        //! just set. Silently dropping a curve because something moved a value
        //! is not what anyone dragging a slider or a manipulator is asking
        //! for, and every widget that writes a value has to agree about that --
        //! so they share this rather than each remembering it.
        void setValue(core::Parameter&, double frame, float value);
    }
}
