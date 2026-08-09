// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/Core/Parameter.h>

#include <nlohmann/json.hpp>

namespace fx
{
    namespace core
    {
        //! \name Serialization
        //!
        //! Found by argument-dependent lookup, so these live beside the types
        //! rather than beside the file writer. A parameter writes itself as the
        //! form it is actually in -- a constant is a bare number, not an object
        //! with a type tag and an empty curve beside it -- because the scene
        //! file is meant to be read and diffed by people.
        ///@{

        //! Widen a float for JSON without showing the noise.
        //!
        //! A float promoted to double prints as the double nearest the float,
        //! so a tenth becomes 0.10000000149011612 and the file stops being
        //! diffable by eye. The shortest form that still reads back as the
        //! same float is used instead, so nothing is lost.
        double toJson(float);

        void to_json(nlohmann::json&, Interp);
        void from_json(const nlohmann::json&, Interp&);

        void to_json(nlohmann::json&, Infinity);
        void from_json(const nlohmann::json&, Infinity&);

        void to_json(nlohmann::json&, const Key&);
        void from_json(const nlohmann::json&, Key&);

        void to_json(nlohmann::json&, const Curve&);
        void from_json(const nlohmann::json&, Curve&);

        void to_json(nlohmann::json&, const Parameter&);
        void from_json(const nlohmann::json&, Parameter&);

        void to_json(nlohmann::json&, const V3Parameter&);
        void from_json(const nlohmann::json&, V3Parameter&);

        ///@}
    }
}
