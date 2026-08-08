// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <ftk/Core/Util.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace ftk
{
    class Context;
}

namespace fx
{
    namespace app
    {
        class App;

        //! Automated screenshot capture, driven by a manifest.
        //!
        //! One shot per process, so no shot can be affected by what the last
        //! one left behind. The capture runs from a timer inside the normal
        //! event loop, which is what realizes and sizes the window and leaves a
        //! buffer worth reading back.
        //!
        //! This exists instead of a command line option per thing worth putting
        //! in a screenshot. Those options multiply, they only ever describe the
        //! states someone happened to need, and they end up as a second way of
        //! setting things that the manifest already covers.
        //!
        //! Each shot writes a PNG and a JSON sidecar. The sidecar carries the
        //! bounding box and the visible text of every widget tagged with
        //! ftk::setScreenshotTag, which is what lets a documentation tool crop
        //! to a widget, or a test assert on what was on screen, without anyone
        //! having to look at the image.
        class Capture : public std::enable_shared_from_this<Capture>
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::filesystem::path& manifest,
                const std::string& shotId,
                const std::filesystem::path& outputDir);

            Capture();

        public:
            ~Capture();

            static std::shared_ptr<Capture> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::filesystem::path& manifest,
                const std::string& shotId,
                const std::filesystem::path& outputDir);

            //! Read the manifest, set the window up the same way every time,
            //! apply the shot's setup, and arm the capture timer. Returns false
            //! on a setup error. After this returns true the caller runs the
            //! event loop.
            bool begin();

            //! Whether the capture finished and wrote its outputs.
            bool succeeded() const;

        private:
            void _onTick();
            void _applyStep(const nlohmann::json&);
            void _finish(bool);

            bool _writePNG(const std::filesystem::path&) const;
            void _writeMetadata(const std::filesystem::path&) const;

            FTK_PRIVATE();
        };
    }
}
