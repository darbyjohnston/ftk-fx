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
            return
            {
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
