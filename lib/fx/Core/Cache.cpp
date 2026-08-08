// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/Core/Cache.h>

#include <algorithm>
#include <cstdlib>

namespace fx
{
    namespace core
    {
        namespace
        {
            size_t byteCount(const std::shared_ptr<const Frame>& frame)
            {
                return frame ? frame->getByteCount() : 0;
            }
        }

        const ftk::RangeI& Cache::getRange() const
        {
            return _range;
        }

        void Cache::setRange(const ftk::RangeI& value)
        {
            if (value == _range)
                return;
            const ftk::RangeI prev = _range;
            std::vector<Entry> prevEntries;
            prevEntries.swap(_entries);

            _range = value;
            _entries = std::vector<Entry>(
                std::max(0, value.max() - value.min() + 1));
            _byteCount = 0;

            // Keep the frames the two ranges have in common, so that changing
            // the range does not throw away work that is still good.
            const int min = std::max(prev.min(), value.min());
            const int max = std::min(prev.max(), value.max());
            for (int frame = min; frame <= max; ++frame)
            {
                Entry& from = prevEntries[frame - prev.min()];
                if (FrameState::Empty == from.state)
                    continue;
                Entry& to = _entries[frame - value.min()];
                to.state = from.state;
                to.frame = std::move(from.frame);
                _byteCount += byteCount(to.frame);
            }
        }

        int Cache::_index(int frame) const
        {
            return (frame >= _range.min() && frame <= _range.max()) ?
                frame - _range.min() :
                -1;
        }

        FrameState Cache::getState(int frame) const
        {
            const int i = _index(frame);
            return i >= 0 ? _entries[i].state : FrameState::Empty;
        }

        std::vector<FrameState> Cache::getStates() const
        {
            std::vector<FrameState> out;
            out.reserve(_entries.size());
            for (const auto& entry : _entries)
            {
                out.push_back(entry.state);
            }
            return out;
        }

        std::shared_ptr<const Frame> Cache::get(int frame) const
        {
            const int i = _index(frame);
            return (i >= 0 && _entries[i].state != FrameState::Empty) ?
                _entries[i].frame :
                nullptr;
        }

        void Cache::set(int frame, const std::shared_ptr<const Frame>& value)
        {
            const int i = _index(frame);
            if (i < 0 || !value)
                return;
            Entry& entry = _entries[i];
            if (FrameState::Locked == entry.state)
                return;
            _byteCount -= byteCount(entry.frame);
            entry.frame = value;
            entry.state = FrameState::Simulated;
            _byteCount += byteCount(entry.frame);
        }

        std::optional<int> Cache::getLastValid(int frame) const
        {
            for (int i = std::min(frame, _range.max()); i >= _range.min(); --i)
            {
                if (_entries[i - _range.min()].state != FrameState::Empty)
                    return i;
            }
            return std::nullopt;
        }

        void Cache::setLocked(int frame, bool value)
        {
            const int i = _index(frame);
            if (i < 0)
                return;
            Entry& entry = _entries[i];
            if (value)
            {
                // Only a simulated frame can be locked: there is nothing to
                // freeze about a frame that has not been simulated.
                if (FrameState::Simulated == entry.state)
                {
                    entry.state = FrameState::Locked;
                }
            }
            else if (FrameState::Locked == entry.state)
            {
                entry.state = FrameState::Simulated;
            }
        }

        void Cache::setLocked(const ftk::RangeI& range, bool value)
        {
            for (int frame = range.min(); frame <= range.max(); ++frame)
            {
                setLocked(frame, value);
            }
        }

        void Cache::_drop(Entry& entry)
        {
            _byteCount -= byteCount(entry.frame);
            entry.frame.reset();
            entry.state = FrameState::Empty;
        }

        void Cache::invalidateFrom(int frame)
        {
            for (int i = std::max(frame, _range.min()); i <= _range.max(); ++i)
            {
                Entry& entry = _entries[i - _range.min()];
                if (FrameState::Simulated == entry.state)
                {
                    _drop(entry);
                }
            }
        }

        void Cache::clear()
        {
            for (auto& entry : _entries)
            {
                entry = Entry();
            }
            _byteCount = 0;
        }

        size_t Cache::getByteCount() const
        {
            return _byteCount;
        }

        size_t Cache::getMemoryBudget() const
        {
            return _memoryBudget;
        }

        void Cache::setMemoryBudget(size_t value)
        {
            _memoryBudget = value;
        }

        void Cache::evict(int playhead)
        {
            if (_byteCount <= _memoryBudget)
                return;

            std::vector<int> candidates;
            for (int frame = _range.min(); frame <= _range.max(); ++frame)
            {
                if (FrameState::Simulated == _entries[frame - _range.min()].state)
                {
                    candidates.push_back(frame);
                }
            }
            std::sort(
                candidates.begin(),
                candidates.end(),
                [playhead](int a, int b)
                {
                    return std::abs(a - playhead) > std::abs(b - playhead);
                });

            for (int frame : candidates)
            {
                if (_byteCount <= _memoryBudget)
                    break;
                _drop(_entries[frame - _range.min()]);
            }
        }
    }
}
