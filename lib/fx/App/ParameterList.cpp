// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/ParameterList.h>

namespace fx
{
    namespace app
    {
        std::string ParameterInfo::getPath() const
        {
            return group + "/" + name;
        }

        std::vector<ParameterInfo> getParameters(sim::System& system)
        {
            auto& emitter = system.getEmitter();
            auto& forces = system.getForces();
            const sim::Emitter defaultEmitter;
            const sim::Forces defaultForces;
            const sim::Transform defaultTransform;
            // The transform rows shuttle rather than slide. Where a
            // translation sits between two made-up ends is not information;
            // how fast it is moving under the hand is. Their ranges stay only
            // as clamps, and are wide enough not to be met by accident.
            return
            {
                { "Transform", "Translate X", &emitter.transform.translate.x,
                    -10000.F, 10000.F,
                    defaultTransform.translate.x.getConstant(),
                    ParameterControl::Shuttle, .1F },
                { "Transform", "Translate Y", &emitter.transform.translate.y,
                    -10000.F, 10000.F,
                    defaultTransform.translate.y.getConstant(),
                    ParameterControl::Shuttle, .1F },
                { "Transform", "Translate Z", &emitter.transform.translate.z,
                    -10000.F, 10000.F,
                    defaultTransform.translate.z.getConstant(),
                    ParameterControl::Shuttle, .1F },
                { "Transform", "Rotate X", &emitter.transform.rotate.x,
                    -36000.F, 36000.F,
                    defaultTransform.rotate.x.getConstant(),
                    ParameterControl::Shuttle, 1.F },
                { "Transform", "Rotate Y", &emitter.transform.rotate.y,
                    -36000.F, 36000.F,
                    defaultTransform.rotate.y.getConstant(),
                    ParameterControl::Shuttle, 1.F },
                { "Transform", "Rotate Z", &emitter.transform.rotate.z,
                    -36000.F, 36000.F,
                    defaultTransform.rotate.z.getConstant(),
                    ParameterControl::Shuttle, 1.F },
                { "Transform", "Scale X", &emitter.transform.scale.x,
                    0.F, 1000.F,
                    defaultTransform.scale.x.getConstant(),
                    ParameterControl::Shuttle, .01F },
                { "Transform", "Scale Y", &emitter.transform.scale.y,
                    0.F, 1000.F,
                    defaultTransform.scale.y.getConstant(),
                    ParameterControl::Shuttle, .01F },
                { "Transform", "Scale Z", &emitter.transform.scale.z,
                    0.F, 1000.F,
                    defaultTransform.scale.z.getConstant(),
                    ParameterControl::Shuttle, .01F },
                { "Emitter", "Rate", &emitter.rate, 0.F, 2000.F,
                    defaultEmitter.rate.getConstant() },
                { "Emitter", "Speed", &emitter.speed, 0.F, 30.F,
                    defaultEmitter.speed.getConstant() },
                { "Emitter", "Speed variance", &emitter.speedVariance, 0.F, 1.F,
                    defaultEmitter.speedVariance.getConstant() },
                { "Emitter", "Spread", &emitter.spread, 0.F, 180.F,
                    defaultEmitter.spread.getConstant() },
                { "Emitter", "Size X", &emitter.size.x, 0.F, 20.F,
                    defaultEmitter.size.x.getConstant() },
                { "Emitter", "Size Y", &emitter.size.y, 0.F, 20.F,
                    defaultEmitter.size.y.getConstant() },
                { "Emitter", "Size Z", &emitter.size.z, 0.F, 20.F,
                    defaultEmitter.size.z.getConstant() },
                { "Emitter", "Lifespan", &emitter.lifespan, .1F, 10.F,
                    defaultEmitter.lifespan.getConstant() },
                { "Emitter", "Lifespan variance", &emitter.lifespanVariance, 0.F, 1.F,
                    defaultEmitter.lifespanVariance.getConstant() },
                { "Forces", "Gravity", &forces.gravity.y, -50.F, 50.F,
                    defaultForces.gravity.y.getConstant() },
                { "Forces", "Drag", &forces.drag, 0.F, 4.F,
                    defaultForces.drag.getConstant() }
            };
        }

        void setValue(core::Parameter& parameter, double frame, float value)
        {
            if (core::Parameter::Type::Curve == parameter.getType())
            {
                core::Curve curve = parameter.getCurve();
                core::Key key;
                key.frame = frame;
                key.value = value;
                // addKey replaces the one already at that frame, so a drag
                // moves the key it started on instead of laying down one per
                // mouse move.
                curve.addKey(key);
                parameter.setCurve(curve);
            }
            else
            {
                parameter.setConstant(value);
            }
        }
    }
}
