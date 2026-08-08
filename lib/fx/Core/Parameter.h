// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Curve.h>

#include <ftk/Core/Vector.h>

namespace fx
{
    namespace core
    {
        //! A value that can be animated.
        //!
        //! Every value the artist can reach is one of these rather than a plain
        //! float, so that "can I animate this?" is never a question about a
        //! particular control. Retrofitting that later would mean touching
        //! every subsystem, which is why it is here in the first slice with
        //! only two of its eventual forms written.
        class Parameter
        {
        public:
            Parameter() = default;
            explicit Parameter(float constant);

            //! What the parameter holds. Profile curves, expressions, and
            //! connections join this list as they are written.
            enum class Type
            {
                Constant,
                Curve,

                Count,
                First = Constant
            };

            Type getType() const;

            //! Get the constant value. This is the last constant the parameter
            //! held even while it is animated, so that turning animation off
            //! returns it to where it was.
            float getConstant() const;

            //! Set the constant value, making the parameter constant.
            void setConstant(float);

            const Curve& getCurve() const;

            //! Set the curve, making the parameter animated.
            void setCurve(const Curve&);

            //! Evaluate the parameter.
            float getValue(double frame) const;

            bool operator == (const Parameter&) const;
            bool operator != (const Parameter&) const;

        private:
            Type  _type = Type::Constant;
            float _constant = 0.F;
            Curve _curve;
        };

        //! A three-component value that can be animated.
        //!
        //! Three parameters rather than a parameter of vectors: the components
        //! of a direction or a colour are keyed apart far more often than
        //! together, and this way the curve editor only ever deals in scalars.
        struct V3Parameter
        {
            V3Parameter() = default;
            explicit V3Parameter(const ftk::V3F&);

            Parameter x;
            Parameter y;
            Parameter z;

            ftk::V3F getValue(double frame) const;

            bool operator == (const V3Parameter&) const;
            bool operator != (const V3Parameter&) const;
        };
    }
}
