// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <string>

#define FX_VERSION_MAJOR 0
#define FX_VERSION_MINOR 1
#define FX_VERSION_PATCH 0
#define FX_VERSION_FULL "0.1.0"

namespace fx
{
    namespace core
    {
        //! Get the application version.
        std::string getVersion();
    }
}
