// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Sim/Scene.h>

#include <fx/Core/Serialize.h>
#include <fx/Core/Version.h>

#include <ftk/Core/Format.h>

#include <stdexcept>

#include <fstream>

namespace fx
{
    namespace sim
    {
        bool Scene::operator == (const Scene& other) const
        {
            return
                range == other.range &&
                frameRate == other.frameRate &&
                system == other.system;
        }

        bool Scene::operator != (const Scene& other) const
        {
            return !(*this == other);
        }

        void to_json(nlohmann::json& json, const Emitter& value)
        {
            json = nlohmann::json
            {
                { "enabled", value.enabled },
                { "seed", value.seed },
                { "shape", getLabel(value.shape) },
                { "surface", value.surface },
                { "position", value.position },
                { "size", value.size },
                { "rate", value.rate },
                { "direction", value.direction },
                { "spread", value.spread },
                { "speed", value.speed },
                { "speedVariance", value.speedVariance },
                { "lifespan", value.lifespan },
                { "lifespanVariance", value.lifespanVariance }
            };
        }

        void from_json(const nlohmann::json& json, Emitter& out)
        {
            // Every field optional and defaulted from a fresh emitter, so that
            // a scene written by an older build still loads when a field is
            // added. The alternative is a version number that has to be bumped
            // for every addition and a migration for every bump.
            out = Emitter();
            if (json.contains("enabled")) json.at("enabled").get_to(out.enabled);
            if (json.contains("seed")) json.at("seed").get_to(out.seed);
            if (json.contains("shape"))
            {
                const std::string name = json.at("shape").get<std::string>();
                if (!fromString(name, out.shape))
                    throw std::runtime_error(ftk::Format(
                        "unknown emitter shape \"{0}\"").arg(name));
            }
            if (json.contains("surface")) json.at("surface").get_to(out.surface);
            if (json.contains("size")) json.at("size").get_to(out.size);
            if (json.contains("position")) json.at("position").get_to(out.position);
            if (json.contains("rate")) json.at("rate").get_to(out.rate);
            if (json.contains("direction")) json.at("direction").get_to(out.direction);
            if (json.contains("spread")) json.at("spread").get_to(out.spread);
            if (json.contains("speed")) json.at("speed").get_to(out.speed);
            if (json.contains("speedVariance")) json.at("speedVariance").get_to(out.speedVariance);
            if (json.contains("lifespan")) json.at("lifespan").get_to(out.lifespan);
            if (json.contains("lifespanVariance")) json.at("lifespanVariance").get_to(out.lifespanVariance);
        }

        void to_json(nlohmann::json& json, const Forces& value)
        {
            json = nlohmann::json
            {
                { "gravity", value.gravity },
                { "drag", value.drag }
            };
        }

        void from_json(const nlohmann::json& json, Forces& out)
        {
            out = Forces();
            if (json.contains("gravity")) json.at("gravity").get_to(out.gravity);
            if (json.contains("drag")) json.at("drag").get_to(out.drag);
        }

        void to_json(nlohmann::json& json, const System& value)
        {
            json = nlohmann::json
            {
                { "name", value.getName() },
                { "enabled", value.isEnabled() },
                { "substeps", value.getSubsteps() },
                { "emitter", value.getEmitter() },
                { "forces", value.getForces() }
            };
        }

        void from_json(const nlohmann::json& json, System& out)
        {
            out = System();
            if (json.contains("name")) out.setName(json.at("name").get<std::string>());
            if (json.contains("enabled")) out.setEnabled(json.at("enabled").get<bool>());
            if (json.contains("substeps")) out.setSubsteps(json.at("substeps").get<int>());
            if (json.contains("emitter")) json.at("emitter").get_to(out.getEmitter());
            if (json.contains("forces")) json.at("forces").get_to(out.getForces());
        }

        void to_json(nlohmann::json& json, const Scene& value)
        {
            json = nlohmann::json
            {
                { "version", core::getVersion() },
                { "range", value.range },
                { "frameRate", value.frameRate },
                // A list with one entry in it. There is only ever one system
                // today, and a file that has to grow an array later is a file
                // every reader has to learn twice.
                { "systems", nlohmann::json::array({ value.system }) }
            };
        }

        void from_json(const nlohmann::json& json, Scene& out)
        {
            out = Scene();
            if (json.contains("range")) json.at("range").get_to(out.range);
            if (json.contains("frameRate")) json.at("frameRate").get_to(out.frameRate);
            if (json.contains("systems"))
            {
                const auto& systems = json.at("systems");
                if (!systems.is_array())
                    throw std::runtime_error("\"systems\" is not a list");
                if (!systems.empty())
                {
                    systems.at(0).get_to(out.system);
                }
            }
            if (out.range.min() > out.range.max())
                throw std::runtime_error("the frame range runs backwards");
            if (out.frameRate <= 0.0)
                throw std::runtime_error("the frame rate is not positive");
        }

        Scene read(const std::filesystem::path& path)
        {
            std::ifstream is(path);
            if (!is.is_open())
                throw std::runtime_error(ftk::Format("cannot open \"{0}\"").
                    arg(path.string()));
            nlohmann::json json;
            try
            {
                is >> json;
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error(ftk::Format("{0}: {1}").
                    arg(path.string()).arg(e.what()));
            }
            try
            {
                return json.get<Scene>();
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error(ftk::Format("{0}: {1}").
                    arg(path.string()).arg(e.what()));
            }
        }

        void write(const std::filesystem::path& path, const Scene& scene)
        {
            std::ofstream os(path);
            if (!os.is_open())
                throw std::runtime_error(ftk::Format("cannot write \"{0}\"").
                    arg(path.string()));
            os << nlohmann::json(scene).dump(4) << std::endl;
            if (!os)
                throw std::runtime_error(ftk::Format("cannot write \"{0}\"").
                    arg(path.string()));
        }
    }
}
