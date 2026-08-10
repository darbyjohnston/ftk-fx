// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Icons.h>

#include <ftk/UI/IconSystem.h>

#include <ftk/Core/Context.h>

#include <string>

namespace fx
{
    namespace app
    {
        namespace
        {
            // Drawn at the size the reset button uses, since they sit in the
            // same row as each other.
            const std::string keySvg =
                "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                "<svg width=\"11\" height=\"11\" viewBox=\"0 0 11 11\""
                " xmlns=\"http://www.w3.org/2000/svg\">\n"
                "  <path style=\"fill:#f0f0f0;fill-opacity:1;stroke:none\""
                " d=\"M 5.5,0.7 10.3,5.5 5.5,10.3 0.7,5.5 Z\" />\n"
                "</svg>\n";

            void add(
                const std::shared_ptr<ftk::IconSystem>& iconSystem,
                const std::string& name,
                const std::string& svg)
            {
                iconSystem->add(
                    name,
                    std::vector<uint8_t>(svg.begin(), svg.end()));
            }
        }

        void registerIcons(const std::shared_ptr<ftk::Context>& context)
        {
            auto iconSystem = context->getSystem<ftk::IconSystem>();
            add(iconSystem, "Key", keySvg);
        }
    }
}
