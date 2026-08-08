// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Core/Parameter.h>

namespace fx
{
    namespace core
    {
        Parameter::Parameter(float constant) :
            _constant(constant)
        {}

        Parameter::Type Parameter::getType() const
        {
            return _type;
        }

        float Parameter::getConstant() const
        {
            return _constant;
        }

        void Parameter::setConstant(float value)
        {
            _type = Type::Constant;
            _constant = value;
        }

        const Curve& Parameter::getCurve() const
        {
            return _curve;
        }

        void Parameter::setCurve(const Curve& value)
        {
            _type = Type::Curve;
            _curve = value;
        }

        float Parameter::getValue(double frame) const
        {
            return Type::Curve == _type ? _curve.getValue(frame) : _constant;
        }

        bool Parameter::operator == (const Parameter& other) const
        {
            return
                _type == other._type &&
                _constant == other._constant &&
                _curve == other._curve;
        }

        bool Parameter::operator != (const Parameter& other) const
        {
            return !(*this == other);
        }

        V3Parameter::V3Parameter(const ftk::V3F& value) :
            x(value.x),
            y(value.y),
            z(value.z)
        {}

        ftk::V3F V3Parameter::getValue(double frame) const
        {
            return ftk::V3F(
                x.getValue(frame),
                y.getValue(frame),
                z.getValue(frame));
        }

        bool V3Parameter::operator == (const V3Parameter& other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }

        bool V3Parameter::operator != (const V3Parameter& other) const
        {
            return !(*this == other);
        }
    }
}
