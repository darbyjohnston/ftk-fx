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

            //! The editor arrangements, drawn as the arrangements themselves so
            //! the button shows what it does. Twenty across, like the icons
            //! feather-tk ships; the reset diamond is smaller because it sits
            //! in a row of sliders rather than on a tool bar.
            std::string layoutSvg(const std::string& rects)
            {
                return
                    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                    "<svg width=\"20\" height=\"20\" viewBox=\"0 0 20 20\""
                    " xmlns=\"http://www.w3.org/2000/svg\">\n"
                    "  <g style=\"fill:#f0f0f0;fill-opacity:1;stroke:none\">\n" +
                    rects +
                    "  </g>\n</svg>\n";
            }

            const std::string rectFull   = "    <rect x=\"2\" y=\"2\" width=\"16\" height=\"16\"/>\n";
            const std::string rectTop    = "    <rect x=\"2\" y=\"2\" width=\"16\" height=\"7\"/>\n";
            const std::string rectBottom = "    <rect x=\"2\" y=\"11\" width=\"16\" height=\"7\"/>\n";
            const std::string rectTopL   = "    <rect x=\"2\" y=\"2\" width=\"7\" height=\"7\"/>\n";
            const std::string rectTopR   = "    <rect x=\"11\" y=\"2\" width=\"7\" height=\"7\"/>\n";
            const std::string rectBotL   = "    <rect x=\"2\" y=\"11\" width=\"7\" height=\"7\"/>\n";
            const std::string rectBotR   = "    <rect x=\"11\" y=\"11\" width=\"7\" height=\"7\"/>\n";

            //! A plus and a minus, for adding and removing a system.
            //! feather-tk's Increment and Decrement are the spinner arrows a
            //! number edit uses, which say "next" rather than "another".
            std::string barsSvg(const std::string& bars)
            {
                return
                    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                    "<svg width=\"20\" height=\"20\" viewBox=\"0 0 20 20\""
                    " xmlns=\"http://www.w3.org/2000/svg\">\n"
                    "  <g style=\"fill:#f0f0f0;fill-opacity:1;stroke:none\">\n" +
                    bars +
                    "  </g>\n</svg>\n";
            }

            const std::string barH = "    <rect x=\"4\" y=\"9\" width=\"12\" height=\"2\"/>\n";
            const std::string barV = "    <rect x=\"9\" y=\"4\" width=\"2\" height=\"12\"/>\n";

            //! One sheet in front of another, drawn as outlines so it does not
            //! read as a solid block at twenty pixels. Not feather-tk's Copy,
            //! which is a clipboard: this makes another system rather than
            //! putting one somewhere to be pasted, and the day systems can be
            //! copied and pasted that icon should still mean that.
            const std::string duplicate =
                // The sheet behind, showing at its top left corner only.
                "    <rect x=\"3\" y=\"3\" width=\"9\" height=\"2\"/>\n"
                "    <rect x=\"3\" y=\"3\" width=\"2\" height=\"9\"/>\n"
                // The sheet in front.
                "    <rect x=\"7\" y=\"7\" width=\"10\" height=\"2\"/>\n"
                "    <rect x=\"7\" y=\"15\" width=\"10\" height=\"2\"/>\n"
                "    <rect x=\"7\" y=\"7\" width=\"2\" height=\"10\"/>\n"
                "    <rect x=\"15\" y=\"7\" width=\"2\" height=\"10\"/>\n";

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
            add(iconSystem, "LayoutSingle", layoutSvg(rectFull));
            add(iconSystem, "LayoutTwo", layoutSvg(rectTop + rectBottom));
            add(iconSystem, "LayoutThree",
                layoutSvg(rectTopL + rectTopR + rectBottom));
            add(iconSystem, "LayoutFour",
                layoutSvg(rectTopL + rectTopR + rectBotL + rectBotR));
            add(iconSystem, "SystemAdd", barsSvg(barH + barV));
            add(iconSystem, "SystemRemove", barsSvg(barH));
            add(iconSystem, "SystemDuplicate", barsSvg(duplicate));
        }
    }
}
