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
        //! What a pane is showing.
        //!
        //! The main region holds panes rather than only viewports because the
        //! editors coming in §4a, §9, §10a and §12 are viewport-shaped: a curve
        //! editor is a graph to box-select in, a spreadsheet is a table. None
        //! of them would fit in the panel column, and giving them a mechanism
        //! of their own would be two panel systems instead of one.
        //!
        //! Everything but View is a stand-in today. They are here to be
        //! arranged and switched between, so that the mechanism is exercised
        //! before any of the editors are written, and so there is something
        //! concrete to replace one at a time.
        enum class PaneType
        {
            View,
            Curves,
            Spreadsheet,
            Expression,
            Compositor,

            Count,
            First = View
        };

        std::vector<std::string> getPaneTypeLabels();
        std::string getLabel(PaneType);
        bool fromString(const std::string&, PaneType&);

        //! Get the line a stand-in shows about what will eventually be there.
        std::string getPaneTypeDescription(PaneType);

        //! What a viewport pane is looking through.
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

        //! How the panes are arranged.
        //!
        //! Fixed arrangements rather than dockable panels: one arrangement per
        //! pane count is most of the value, and every arrangement that does not
        //! exist is a set of decisions nobody has to make.
        enum class PaneLayout
        {
            //! One pane.
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

        std::vector<std::string> getPaneLayoutLabels();
        std::string getLabel(PaneLayout);

        //! Get the number of panes an arrangement shows.
        int getPaneCount(PaneLayout);

        //! The most panes any arrangement shows. The pane slots are all made up
        //! front and kept, so this is a real limit rather than a hint.
        const int paneCountMax = 4;

        //! Get the view a pane starts on. The defaults spell out the familiar
        //! four-up: perspective, top, front, side.
        ViewType getDefaultViewType(int index);
    }
}
