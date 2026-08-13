// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Icons.h>

#include <ftk/UI/IconSystem.h>

#include <ftk/Core/Context.h>

#include <cstdint>
#include <vector>

// Compiled from etc/Icons by ftk-resource; see lib/fx/Resource. Declared
// rather than included because the generator writes the definitions and no
// header, which is how feather-tk and tlRender carry their own.
namespace fx_resource
{
    extern std::vector<uint8_t> Key;
    extern std::vector<uint8_t> LayoutFour;
    extern std::vector<uint8_t> LayoutSingle;
    extern std::vector<uint8_t> LayoutThree;
    extern std::vector<uint8_t> LayoutTwo;
    extern std::vector<uint8_t> Rotate;
    extern std::vector<uint8_t> Scale;
    extern std::vector<uint8_t> SystemAdd;
    extern std::vector<uint8_t> SystemDuplicate;
    extern std::vector<uint8_t> SystemRemove;
    extern std::vector<uint8_t> Translate;
}

namespace fx
{
    namespace app
    {
        void registerIcons(const std::shared_ptr<ftk::Context>& context)
        {
            auto iconSystem = context->getSystem<ftk::IconSystem>();
            iconSystem->add("Key", fx_resource::Key);
            iconSystem->add("LayoutSingle", fx_resource::LayoutSingle);
            iconSystem->add("LayoutTwo", fx_resource::LayoutTwo);
            iconSystem->add("LayoutThree", fx_resource::LayoutThree);
            iconSystem->add("LayoutFour", fx_resource::LayoutFour);
            iconSystem->add("SystemAdd", fx_resource::SystemAdd);
            iconSystem->add("SystemDuplicate", fx_resource::SystemDuplicate);
            iconSystem->add("SystemRemove", fx_resource::SystemRemove);
            iconSystem->add("Translate", fx_resource::Translate);
            iconSystem->add("Rotate", fx_resource::Rotate);
            iconSystem->add("Scale", fx_resource::Scale);
        }
    }
}
