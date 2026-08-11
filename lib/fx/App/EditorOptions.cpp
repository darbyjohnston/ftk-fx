// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/EditorOptions.h>

namespace fx
{
    namespace app
    {
        namespace
        {
            template<typename T>
            bool labelToEnum(
                const std::vector<std::string>& labels,
                const std::string& name,
                T& out)
            {
                for (size_t i = 0; i < labels.size(); ++i)
                {
                    if (labels[i] == name)
                    {
                        out = static_cast<T>(i);
                        return true;
                    }
                }
                return false;
            }

            template<typename T>
            std::string enumToLabel(
                const std::vector<std::string>& labels,
                T value)
            {
                const size_t i = static_cast<size_t>(value);
                return i < labels.size() ? labels[i] : std::string();
            }
        }

        std::vector<std::string> getEditorTypeLabels()
        {
            return { "View", "Curves", "Spreadsheet", "Expression", "Compositor" };
        }

        std::string getLabel(EditorType value)
        {
            return enumToLabel(getEditorTypeLabels(), value);
        }

        bool fromString(const std::string& name, EditorType& out)
        {
            return labelToEnum(getEditorTypeLabels(), name, out);
        }

        std::string getEditorTypeDescription(EditorType value)
        {
            // The section each stand-in is standing in for, so that whoever
            // opens one knows where its design already is.
            switch (value)
            {
            case EditorType::Curves:      return "Curve editor \xc2\xa7" "4a";
            case EditorType::Spreadsheet: return "Particle spreadsheet \xc2\xa7" "12";
            case EditorType::Expression:  return "Expression editor \xc2\xa7" "9";
            case EditorType::Compositor:  return "Compositor \xc2\xa7" "10a";
            default:                    return std::string();
            }
        }

        std::vector<std::string> getViewTypeLabels()
        {
            return { "Persp", "Front", "Side", "Top" };
        }

        std::string getLabel(ViewType value)
        {
            return enumToLabel(getViewTypeLabels(), value);
        }

        bool fromString(const std::string& name, ViewType& out)
        {
            return labelToEnum(getViewTypeLabels(), name, out);
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

        std::vector<std::string> getEditorLayoutLabels()
        {
            return { "Single", "Two", "Three", "Four" };
        }

        std::vector<std::string> getDrawTypeLabels()
        {
            return { "Point", "Sphere" };
        }

        std::string getLabel(DrawType value)
        {
            return enumToLabel(getDrawTypeLabels(), value);
        }

        bool fromString(const std::string& name, DrawType& out)
        {
            return labelToEnum(getDrawTypeLabels(), name, out);
        }

        std::string getLabel(EditorLayout value)
        {
            return enumToLabel(getEditorLayoutLabels(), value);
        }

        int getEditorCount(EditorLayout value)
        {
            switch (value)
            {
            case EditorLayout::Two:   return 2;
            case EditorLayout::Three: return 3;
            case EditorLayout::Four:  return 4;
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
