// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Core/Serialize.h>

#include <ftk/Core/Format.h>

#include <stdexcept>

#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace fx
{
    namespace core
    {
        namespace
        {
            // Written as names rather than as the numbers the enums happen to
            // have, so that inserting an interpolation mode does not silently
            // reinterpret every scene file already on disk.
            const std::array<std::string, 4> interpNames =
            {
                "Step",
                "Linear",
                "Smooth",
                "Bezier"
            };

            const std::array<std::string, 5> infinityNames =
            {
                "Constant",
                "Linear",
                "Cycle",
                "CycleOffset",
                "Oscillate"
            };

            template<typename T, size_t N>
            void nameToEnum(
                const nlohmann::json& json,
                const std::array<std::string, N>& names,
                const std::string& what,
                T& out)
            {
                const std::string s = json.get<std::string>();
                for (size_t i = 0; i < names.size(); ++i)
                {
                    if (names[i] == s)
                    {
                        out = static_cast<T>(i);
                        return;
                    }
                }
                throw std::runtime_error(ftk::Format("unknown {0} \"{1}\"").
                    arg(what).arg(s));
            }
        }

        double toJson(float value)
        {
            if (!std::isfinite(value))
                return value;
            // The shortest of the three precisions that survives the trip.
            // digits10 is enough for almost every value a person types;
            // max_digits10 is enough for all of them.
            for (int precision = std::numeric_limits<float>::digits10;
                precision <= std::numeric_limits<float>::max_digits10;
                ++precision)
            {
                std::ostringstream ss;
                ss.imbue(std::locale::classic());
                ss << std::setprecision(precision) << value;
                const double out = std::stod(ss.str());
                if (static_cast<float>(out) == value)
                {
                    return out;
                }
            }
            return value;
        }

        void to_json(nlohmann::json& json, Interp value)
        {
            json = interpNames[static_cast<size_t>(value)];
        }

        void from_json(const nlohmann::json& json, Interp& out)
        {
            nameToEnum(json, interpNames, "interpolation", out);
        }

        void to_json(nlohmann::json& json, Infinity value)
        {
            json = infinityNames[static_cast<size_t>(value)];
        }

        void from_json(const nlohmann::json& json, Infinity& out)
        {
            nameToEnum(json, infinityNames, "infinity", out);
        }

        void to_json(nlohmann::json& json, const Key& value)
        {
            json = nlohmann::json
            {
                { "frame", value.frame },
                { "value", toJson(value.value) },
                { "interp", value.interp }
            };
            // Only a Bezier key's slopes are its own; the others are either
            // unused or taken from the neighbours, and writing them would put
            // numbers in the file that editing a neighbour silently invalidates.
            if (Interp::Bezier == value.interp)
            {
                json["inSlope"] = toJson(value.inSlope);
                json["outSlope"] = toJson(value.outSlope);
            }
        }

        void from_json(const nlohmann::json& json, Key& out)
        {
            out = Key();
            json.at("frame").get_to(out.frame);
            json.at("value").get_to(out.value);
            if (json.contains("interp"))
            {
                json.at("interp").get_to(out.interp);
            }
            if (json.contains("inSlope"))
            {
                json.at("inSlope").get_to(out.inSlope);
            }
            if (json.contains("outSlope"))
            {
                json.at("outSlope").get_to(out.outSlope);
            }
        }

        void to_json(nlohmann::json& json, const Curve& value)
        {
            json = nlohmann::json{ { "keys", value.getKeys() } };
            // Omitted when they are the default, which is what they are on
            // almost every curve. A file full of "preInfinity": "Constant" is
            // harder to read than one without it.
            if (Infinity::Constant != value.getPreInfinity())
            {
                json["preInfinity"] = value.getPreInfinity();
            }
            if (Infinity::Constant != value.getPostInfinity())
            {
                json["postInfinity"] = value.getPostInfinity();
            }
        }

        void from_json(const nlohmann::json& json, Curve& out)
        {
            out = Curve();
            if (json.contains("keys"))
            {
                out.setKeys(json.at("keys").get<std::vector<Key> >());
            }
            if (json.contains("preInfinity"))
            {
                out.setPreInfinity(json.at("preInfinity").get<Infinity>());
            }
            if (json.contains("postInfinity"))
            {
                out.setPostInfinity(json.at("postInfinity").get<Infinity>());
            }
        }

        void to_json(nlohmann::json& json, const Parameter& value)
        {
            switch (value.getType())
            {
            case Parameter::Type::Curve:
                // The constant travels with the curve so that turning the
                // animation off returns the parameter to the value it had,
                // rather than to zero.
                json = nlohmann::json
                {
                    { "constant", toJson(value.getConstant()) },
                    { "curve", value.getCurve() }
                };
                break;
            default:
                json = toJson(value.getConstant());
                break;
            }
        }

        void from_json(const nlohmann::json& json, Parameter& out)
        {
            if (json.is_number())
            {
                out = Parameter(json.get<float>());
            }
            else
            {
                out = Parameter(json.value("constant", 0.F));
                if (json.contains("curve"))
                {
                    out.setCurve(json.at("curve").get<Curve>());
                }
            }
        }

        void to_json(nlohmann::json& json, const V3Parameter& value)
        {
            json = nlohmann::json{ value.x, value.y, value.z };
        }

        void from_json(const nlohmann::json& json, V3Parameter& out)
        {
            if (!json.is_array() || json.size() != 3)
                throw std::runtime_error("a vector parameter needs three components");
            json.at(0).get_to(out.x);
            json.at(1).get_to(out.y);
            json.at(2).get_to(out.z);
        }
    }
}
