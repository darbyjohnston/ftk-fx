// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Core/Frame.h>

namespace fx
{
    namespace core
    {
        size_t Frame::getByteCount() const
        {
            return pool.getByteCount() + sizeof(double);
        }
    }
}
