// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Sim/System.h>

#include <ftk/Core/Range.h>

#include <nlohmann/json.hpp>

#include <filesystem>

namespace fx
{
    namespace sim
    {
        //! Everything a scene file holds.
        //!
        //! Deliberately not the application's model: this is the recipe and the
        //! time it runs over, with no playhead, no cache, and no window layout.
        //! What is here is what is needed to reproduce the simulation, which is
        //! the promise the file makes.
        struct Scene
        {
            ftk::RangeI range = ftk::RangeI(1, 120);
            double frameRate = 24.0;

            //! The systems, in the order they are listed and solved. A fresh
            //! scene has one: an empty scene shows nothing and gives the artist
            //! a panel of buttons to read instead of particles to look at.
            std::vector<System> systems = std::vector<System>(1);

            bool operator == (const Scene&) const;
            bool operator != (const Scene&) const;
        };

        void to_json(nlohmann::json&, const Transform&);
        void from_json(const nlohmann::json&, Transform&);

        void to_json(nlohmann::json&, const Emitter&);
        void from_json(const nlohmann::json&, Emitter&);

        void to_json(nlohmann::json&, const Forces&);
        void from_json(const nlohmann::json&, Forces&);

        void to_json(nlohmann::json&, const System&);
        void from_json(const nlohmann::json&, System&);

        void to_json(nlohmann::json&, const Scene&);
        void from_json(const nlohmann::json&, Scene&);

        //! Read a scene file. Throws on anything it cannot make sense of,
        //! rather than returning a half-populated scene: a file that loaded
        //! "mostly" would simulate something the artist never authored.
        Scene read(const std::filesystem::path&);

        //! Write a scene file.
        void write(const std::filesystem::path&, const Scene&);
    }
}
