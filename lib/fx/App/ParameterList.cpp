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
            return
            {
                { "Transform", "Translate X", &emitter.transform.translate.x, -50.F, 50.F,
                    defaultTransform.translate.x.getConstant() },
                { "Transform", "Translate Y", &emitter.transform.translate.y, -50.F, 50.F,
                    defaultTransform.translate.y.getConstant() },
                { "Transform", "Translate Z", &emitter.transform.translate.z, -50.F, 50.F,
                    defaultTransform.translate.z.getConstant() },
                { "Transform", "Rotate X", &emitter.transform.rotate.x, -180.F, 180.F,
                    defaultTransform.rotate.x.getConstant() },
                { "Transform", "Rotate Y", &emitter.transform.rotate.y, -180.F, 180.F,
                    defaultTransform.rotate.y.getConstant() },
                { "Transform", "Rotate Z", &emitter.transform.rotate.z, -180.F, 180.F,
                    defaultTransform.rotate.z.getConstant() },
                { "Transform", "Scale X", &emitter.transform.scale.x, 0.F, 10.F,
                    defaultTransform.scale.x.getConstant() },
                { "Transform", "Scale Y", &emitter.transform.scale.y, 0.F, 10.F,
                    defaultTransform.scale.y.getConstant() },
                { "Transform", "Scale Z", &emitter.transform.scale.z, 0.F, 10.F,
                    defaultTransform.scale.z.getConstant() },
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
    }
}
