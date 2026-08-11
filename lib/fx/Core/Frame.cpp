// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Core/Frame.h>

namespace fx
{
    namespace core
    {
        size_t SystemFrame::getByteCount() const
        {
            return pool.getByteCount() + sizeof(double);
        }

        size_t Frame::getByteCount() const
        {
            size_t out = 0;
            for (const auto& system : systems)
            {
                out += system.getByteCount();
            }
            return out;
        }

        size_t Frame::getParticleCount() const
        {
            size_t out = 0;
            for (const auto& system : systems)
            {
                out += system.pool.size();
            }
            return out;
        }
    }
}
