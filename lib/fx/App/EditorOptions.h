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
        //! What an editor is showing.
        //!
        //! The main region holds editors rather than only viewports because the
        //! editors coming in §4a, §9, §10a and §12 are viewport-shaped: a curve
        //! editor is a graph to box-select in, a spreadsheet is a table. None
        //! of them would fit in the panel column, and giving them a mechanism
        //! of their own would be two panel systems instead of one.
        //!
        //! Everything but View is a stand-in today. They are here to be
        //! arranged and switched between, so that the mechanism is exercised
        //! before any of the editors are written, and so there is something
        //! concrete to replace one at a time.
        enum class EditorType
        {
            View,
            Curves,
            Spreadsheet,
            Expression,
            Compositor,

            Count,
            First = View
        };

        std::vector<std::string> getEditorTypeLabels();
        std::string getLabel(EditorType);
        bool fromString(const std::string&, EditorType&);

        //! Get the line a stand-in shows about what will eventually be there.
        std::string getEditorTypeDescription(EditorType);

        //! What a viewport editor is looking through.
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

        //! How the editors are arranged.
        //!
        //! Fixed arrangements rather than dockable panels: one arrangement per
        //! editor count is most of the value, and every arrangement that does
        //! not exist is a set of decisions nobody has to make.
        enum class EditorLayout
        {
            //! One editor.
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

        std::vector<std::string> getEditorLayoutLabels();
        std::string getLabel(EditorLayout);

        //! How the particles are drawn.
        //!
        //! A display setting rather than a scene one, like the point size:
        //! it is how the artist is looking at the simulation. §10's render
        //! types are the scene's own answer and are a later thing.
        enum class DrawType
        {
            //! A flat disc.
            Point,

            //! The same disc shaded as the silhouette of a sphere, which is
            //! what makes a plume read as volume rather than as confetti.
            Sphere,

            Count,
            First = Point
        };

        std::vector<std::string> getDrawTypeLabels();
        std::string getLabel(DrawType);
        bool fromString(const std::string&, DrawType&);

        //! What the viewport's manipulator does with the arm that is grabbed.
        //!
        //! One manipulator with three modes rather than three manipulators:
        //! they share an origin, a pick, a drag and a command, and differ only
        //! in what the drag means.
        enum class GizmoMode
        {
            //! Arms. Sliding one moves the emitter along that axis.
            Translate,

            //! Rings. Turning one turns the emitter about that axis.
            Rotate,

            //! Arms with square ends. Sliding one scales along that axis.
            Scale,

            Count,
            First = Translate
        };

        std::vector<std::string> getGizmoModeLabels();
        std::string getLabel(GizmoMode);
        bool fromString(const std::string&, GizmoMode&);

        //! Get the number of editors an arrangement shows.
        int getEditorCount(EditorLayout);

        //! The most editors any arrangement shows. The editor slots are all
        //! made up front and kept, so this is a real limit rather than a hint.
        const int editorCountMax = 4;

        //! Get the view an editor starts on. The defaults spell out the
        //! familiar four-up: perspective, top, front, side.
        ViewType getDefaultViewType(int index);
    }
}
