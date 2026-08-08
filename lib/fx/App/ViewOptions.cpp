// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/ViewOptions.h>

namespace fx
{
    namespace app
    {
        std::vector<std::string> getViewTypeLabels()
        {
            return { "Persp", "Front", "Side", "Top" };
        }

        std::string getLabel(ViewType value)
        {
            const auto labels = getViewTypeLabels();
            const size_t i = static_cast<size_t>(value);
            return i < labels.size() ? labels[i] : std::string();
        }

        bool fromString(const std::string& name, ViewType& out)
        {
            const auto labels = getViewTypeLabels();
            for (size_t i = 0; i < labels.size(); ++i)
            {
                if (labels[i] == name)
                {
                    out = static_cast<ViewType>(i);
                    return true;
                }
            }
            return false;
        }

        bool isOrtho(ViewType value)
        {
            return value != ViewType::Perspective;
        }

        ftk::V2F getViewOrbit(ViewType value)
        {
            // Yaw about the vertical axis, then pitch. Front looks along -Z,
            // side looks along -X, top looks straight down.
            switch (value)
            {
            case ViewType::Front: return ftk::V2F(0.F, 0.F);
            case ViewType::Side:  return ftk::V2F(90.F, 0.F);
            case ViewType::Top:   return ftk::V2F(0.F, 90.F);
            default:              return ftk::V2F(35.F, 20.F);
            }
        }

        std::vector<std::string> getViewLayoutLabels()
        {
            return { "Single", "Two", "Three", "Four" };
        }

        std::string getLabel(ViewLayout value)
        {
            const auto labels = getViewLayoutLabels();
            const size_t i = static_cast<size_t>(value);
            return i < labels.size() ? labels[i] : std::string();
        }

        int getViewCount(ViewLayout value)
        {
            switch (value)
            {
            case ViewLayout::Two:   return 2;
            case ViewLayout::Three: return 3;
            case ViewLayout::Four:  return 4;
            default:                return 1;
            }
        }

        ViewType getDefaultViewType(int index)
        {
            switch (index)
            {
            case 1:  return ViewType::Top;
            case 2:  return ViewType::Front;
            case 3:  return ViewType::Side;
            default: return ViewType::Perspective;
            }
        }
    }
}
