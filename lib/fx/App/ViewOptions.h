// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/Core/Vector.h>

#include <string>
#include <vector>

namespace fx
{
    namespace app
    {
        //! What a viewport is looking through.
        //!
        //! The axis views are orthographic, which is what makes them worth
        //! having: judging whether debris is really travelling along the ground
        //! plane is guesswork down a perspective camera.
        //!
        //! Imported cameras join this list once there is scene import to bring
        //! them in.
        enum class ViewType
        {
            Perspective,
            Front,
            Side,
            Top,

            Count,
            First = Perspective
        };

        std::vector<std::string> getViewTypeLabels();
        std::string getLabel(ViewType);

        //! Look a view type up by its label. Returns false when the name does
        //! not match one, so a typo in a manifest is reported rather than
        //! quietly capturing the wrong view.
        bool fromString(const std::string&, ViewType&);

        //! Is the view orthographic?
        bool isOrtho(ViewType);

        //! Get the fixed orientation of an axis view, as the yaw and pitch the
        //! camera would have been orbited to. Perspective returns the angles a
        //! new view starts at, which the user is then free to change.
        ftk::V2F getViewOrbit(ViewType);

        //! How the viewports are arranged.
        //!
        //! Fixed arrangements rather than dockable panels: one arrangement per
        //! viewport count is most of the value, and every arrangement that does
        //! not exist is a set of decisions nobody has to make.
        enum class ViewLayout
        {
            //! One viewport.
            Single,

            //! Two side by side.
            Two,

            //! One on the left, two stacked on the right.
            Three,

            //! Two by two.
            Four,

            Count,
            First = Single
        };

        std::vector<std::string> getViewLayoutLabels();
        std::string getLabel(ViewLayout);

        //! Get the number of viewports an arrangement shows.
        int getViewCount(ViewLayout);

        //! The most viewports any arrangement shows. The viewports are all made
        //! up front and kept, so this is a real limit rather than a hint.
        const int viewCountMax = 4;

        //! Get the view a viewport starts on. The defaults spell out the
        //! familiar four-up: perspective, top, front, side.
        ViewType getDefaultViewType(int index);
    }
}
