// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Core/Pool.h>

namespace fx
{
    namespace core
    {
        size_t Pool::size() const
        {
            return id.size();
        }

        size_t Pool::getAliveCount() const
        {
            return _aliveCount;
        }

        size_t Pool::birth(size_t count)
        {
            const size_t out = id.size();
            const size_t newSize = out + count;
            id.resize(newSize);
            position.resize(newSize, ftk::V3F());
            velocity.resize(newSize, ftk::V3F());
            age.resize(newSize, 0.F);
            lifespan.resize(newSize, 0.F);
            alive.resize(newSize, 1);
            for (size_t i = out; i < newSize; ++i)
            {
                id[i] = _nextId++;
            }
            _aliveCount += count;
            return out;
        }

        void Pool::kill(size_t index)
        {
            if (index < alive.size() && alive[index])
            {
                alive[index] = 0;
                --_aliveCount;
            }
        }

        void Pool::compact()
        {
            if (_aliveCount == id.size())
                return;
            size_t out = 0;
            for (size_t i = 0; i < id.size(); ++i)
            {
                if (alive[i])
                {
                    if (out != i)
                    {
                        id[out] = id[i];
                        position[out] = position[i];
                        velocity[out] = velocity[i];
                        age[out] = age[i];
                        lifespan[out] = lifespan[i];
                        alive[out] = 1;
                    }
                    ++out;
                }
            }
            id.resize(out);
            position.resize(out);
            velocity.resize(out);
            age.resize(out);
            lifespan.resize(out);
            alive.resize(out);
        }

        void Pool::clear()
        {
            id.clear();
            position.clear();
            velocity.clear();
            age.clear();
            lifespan.clear();
            alive.clear();
            _nextId = 1;
            _aliveCount = 0;
        }

        uint64_t Pool::getEmittedCount() const
        {
            return _nextId - 1;
        }

        size_t Pool::getByteCount() const
        {
            const size_t n = id.size();
            return
                n * sizeof(uint64_t) +
                n * sizeof(ftk::V3F) * 2 +
                n * sizeof(float) * 2 +
                n * sizeof(uint8_t);
        }
    }
}
