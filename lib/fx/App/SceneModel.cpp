// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/SceneModel.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Math.h>
#include <ftk/Core/Timer.h>

using namespace ftk;

namespace fx
{
    namespace app
    {
        namespace
        {
            //! One command for every kind of edit.
            //!
            //! It holds the whole system before and after rather than a
            //! description of what changed, which sounds wasteful and is not:
            //! a system is a recipe of a few dozen parameters, copying one is
            //! cheaper than the re-simulation the edit causes anyway, and it
            //! means setting a constant, moving a key and re-rolling a seed are
            //! the same command rather than three.
            //!
            //! The model is held by raw pointer because the model owns the
            //! stack that owns this, so it cannot outlive it.
            class StateCommand : public ftk::ICommand
            {
            public:
                StateCommand(
                    SceneModel* model,
                    const std::string& name,
                    const std::vector<sim::System>& beforeSystems,
                    size_t beforeCurrentSystem,
                    float beforeParticleSize,
                    const std::vector<sim::System>& afterSystems,
                    size_t afterCurrentSystem,
                    float afterParticleSize) :
                    _model(model),
                    _name(name),
                    _beforeSystems(beforeSystems),
                    _beforeCurrentSystem(beforeCurrentSystem),
                    _beforeParticleSize(beforeParticleSize),
                    _afterSystems(afterSystems),
                    _afterCurrentSystem(afterCurrentSystem),
                    _afterParticleSize(afterParticleSize)
                {}

                const std::string& getName() const { return _name; }

                void exec() override
                {
                    _model->applyState(
                        _afterSystems, _afterCurrentSystem, _afterParticleSize);
                }

                void undo() override
                {
                    _model->applyState(
                        _beforeSystems, _beforeCurrentSystem, _beforeParticleSize);
                }

            private:
                SceneModel* _model = nullptr;
                std::string _name;
                std::vector<sim::System> _beforeSystems;
                size_t _beforeCurrentSystem = 0;
                float _beforeParticleSize = 3.F;
                std::vector<sim::System> _afterSystems;
                size_t _afterCurrentSystem = 0;
                float _afterParticleSize = 3.F;
            };
        }

        void SceneModel::_init(const std::shared_ptr<Context>& context)
        {
            _context = context;

            _range = Observable<RangeI>::create(RangeI(1, 120));
            _currentFrame = Observable<int>::create(1);
            _playing = Observable<bool>::create(false);
            _frame = Observable<std::shared_ptr<const core::Frame> >::create();
            _cacheStates = ObservableList<core::FrameState>::create();
            _cacheByteCount = Observable<size_t>::create(0);
            _path = Observable<std::filesystem::path>::create();
            _modified = Observable<bool>::create(false);
            _sceneChanged = Observable<int>::create(0);
            _parameterChanged = Observable<int>::create(0);
            _particleSize = Observable<float>::create(3.F);
            _drawType = Observable<DrawType>::create(DrawType::Point);
            _currentSystem = Observable<size_t>::create(0);
            _commands = CommandStack::create();
            _setSystems(sim::Scene().systems);
            _saved = getScene();

            _cache.setRange(_range->get());
            _cache.setMemoryBudget(256 * 1024 * 1024);

            _playbackTimer = Timer::create(context);
            _playbackTimer->setRepeating(true);

            _simulate(_currentFrame->get());
        }

        SceneModel::~SceneModel()
        {}

        std::shared_ptr<SceneModel> SceneModel::create(
            const std::shared_ptr<Context>& context)
        {
            auto out = std::shared_ptr<SceneModel>(new SceneModel);
            out->_init(context);
            return out;
        }

        sim::Scene SceneModel::getScene() const
        {
            sim::Scene out;
            out.range = _range->get();
            out.frameRate = _frameRate;
            out.systems = _systemValues();
            return out;
        }

        std::vector<sim::System> SceneModel::_systemValues() const
        {
            std::vector<sim::System> out;
            out.reserve(_systems.size());
            for (const auto& system : _systems)
            {
                out.push_back(*system);
            }
            return out;
        }

        void SceneModel::_setSystems(const std::vector<sim::System>& value)
        {
            for (size_t i = 0; i < value.size(); ++i)
            {
                if (i < _systems.size())
                {
                    *_systems[i] = value[i];
                }
                else
                {
                    _systems.push_back(
                        std::make_shared<sim::System>(value[i]));
                }
            }
            _systems.resize(std::max<size_t>(1, value.size()));
            if (!_systems[0])
            {
                _systems[0] = std::make_shared<sim::System>();
            }
        }

        void SceneModel::setScene(const sim::Scene& value)
        {
            _setSystems(value.systems);
            _currentSystemUpdate();
            _frameRate = value.frameRate;
            _range->setIfChanged(value.range);
            _cache.setRange(value.range);
            _cache.clear();
            _currentFrame->setIfChanged(
                clamp(_currentFrame->get(), value.range.min(), value.range.max()));
            _simulate(_currentFrame->get());
            _modifiedUpdate();
            _sceneChanged->setAlways(_sceneChanged->get() + 1);
            // A new scene is not something to undo back out of, and the
            // commands describe a system that no longer exists.
            _commands->clear();
            _lastCommand.reset();
        }

        void SceneModel::newScene()
        {
            setScene(sim::Scene());
            _saved = getScene();
            _path->setIfChanged(std::filesystem::path());
            _modifiedUpdate();
        }

        void SceneModel::open(const std::filesystem::path& path)
        {
            // Read before anything is changed, so a file that throws leaves
            // the scene that is already open alone.
            const sim::Scene scene = sim::read(path);
            setScene(scene);
            _saved = getScene();
            _path->setIfChanged(path);
            _modifiedUpdate();
        }

        void SceneModel::save(const std::filesystem::path& path)
        {
            const sim::Scene scene = getScene();
            sim::write(path, scene);
            _saved = scene;
            _path->setIfChanged(path);
            _modifiedUpdate();
        }

        const std::filesystem::path& SceneModel::getPath() const
        {
            return _path->get();
        }

        std::shared_ptr<IObservable<std::filesystem::path> > SceneModel::observePath() const
        {
            return _path;
        }

        std::shared_ptr<IObservable<int> > SceneModel::observeSceneChanged() const
        {
            return _sceneChanged;
        }

        std::shared_ptr<IObservable<int> > SceneModel::observeParameterChanged() const
        {
            return _parameterChanged;
        }

        bool SceneModel::isModified() const
        {
            return _modified->get();
        }

        std::shared_ptr<IObservable<bool> > SceneModel::observeModified() const
        {
            return _modified;
        }

        void SceneModel::_modifiedUpdate()
        {
            _modified->setIfChanged(getScene() != _saved);
        }

        size_t SceneModel::getSystemCount() const
        {
            return _systems.size();
        }

        const sim::System& SceneModel::getSystem(size_t index) const
        {
            return *_systems[index];
        }

        const sim::System& SceneModel::getSystem() const
        {
            return *_systems[_currentSystem->get()];
        }

        sim::System& SceneModel::getSystem()
        {
            return *_systems[_currentSystem->get()];
        }

        size_t SceneModel::getCurrentSystem() const
        {
            return _currentSystem->get();
        }

        std::shared_ptr<IObservable<size_t> > SceneModel::observeCurrentSystem() const
        {
            return _currentSystem;
        }

        void SceneModel::setCurrentSystem(size_t value)
        {
            if (value >= _systems.size())
                return;
            // Not recorded. Which system is being looked at is not an edit to
            // the scene: nothing about the particles changes, and a scene that
            // came back from disk selecting whatever was selected when it was
            // saved would be a file that differs from itself.
            _currentSystem->setIfChanged(value);
        }

        void SceneModel::addSystem()
        {
            const auto beforeSystems = _systemValues();
            const size_t beforeCurrent = _currentSystem->get();
            auto systems = beforeSystems;
            sim::System system;
            system.setName(_systemName("particles"));
            systems.insert(systems.begin() + beforeCurrent + 1, system);
            _setSystems(systems);
            _currentSystem->setIfChanged(beforeCurrent + 1);
            _record("Add System", beforeSystems, beforeCurrent, _particleSize->get());
        }

        void SceneModel::duplicateSystem(size_t index)
        {
            if (index >= _systems.size())
                return;
            const auto beforeSystems = _systemValues();
            const size_t beforeCurrent = _currentSystem->get();
            auto systems = beforeSystems;
            sim::System system = systems[index];
            system.setName(_systemName(system.getName()));
            systems.insert(systems.begin() + index + 1, system);
            _setSystems(systems);
            _currentSystem->setIfChanged(index + 1);
            _record("Duplicate System", beforeSystems, beforeCurrent, _particleSize->get());
        }

        void SceneModel::removeSystem(size_t index)
        {
            if (index >= _systems.size() || _systems.size() < 2)
                return;
            const auto beforeSystems = _systemValues();
            const size_t beforeCurrent = _currentSystem->get();
            auto systems = beforeSystems;
            systems.erase(systems.begin() + index);
            _setSystems(systems);
            _currentSystemUpdate();
            _record("Remove System", beforeSystems, beforeCurrent, _particleSize->get());
        }

        void SceneModel::setSystemName(size_t index, const std::string& value)
        {
            if (index >= _systems.size() || value == _systems[index]->getName())
                return;
            const auto beforeSystems = _systemValues();
            _systems[index]->setName(value);
            _record(
                "Rename System",
                beforeSystems,
                _currentSystem->get(),
                _particleSize->get());
        }

        void SceneModel::setSystemEnabled(size_t index, bool value)
        {
            if (index >= _systems.size() || value == _systems[index]->isEnabled())
                return;
            const auto beforeSystems = _systemValues();
            _systems[index]->setEnabled(value);
            _record(
                "Enable System",
                beforeSystems,
                _currentSystem->get(),
                _particleSize->get());
        }

        std::string SceneModel::_systemName(const std::string& base) const
        {
            // "particles", then "particles 2". Numbered from what is already
            // there rather than from a counter on the model, so that a name
            // freed by a removal is used again instead of counting upward
            // forever.
            std::string out = base;
            for (int i = 2; ; ++i)
            {
                bool taken = false;
                for (const auto& system : _systems)
                {
                    if (system->getName() == out)
                    {
                        taken = true;
                        break;
                    }
                }
                if (!taken)
                    break;
                out = Format("{0} {1}").arg(base).arg(i);
            }
            return out;
        }

        void SceneModel::_currentSystemUpdate()
        {
            if (_currentSystem->get() >= _systems.size())
            {
                _currentSystem->setIfChanged(_systems.size() - 1);
            }
        }

        void SceneModel::applyState(
            const std::vector<sim::System>& systems,
            size_t currentSystem,
            float particleSize)
        {
            const bool listChanged = systems.size() != _systems.size();
            _setSystems(systems);
            _currentSystem->setIfChanged(
                std::min(currentSystem, _systems.size() - 1));
            _particleSize->setIfChanged(particleSize);
            _cache.invalidateFrom(_range->get().min());
            _simulate(_currentFrame->get());
            _modifiedUpdate();
            _parameterChanged->setAlways(_parameterChanged->get() + 1);
            if (listChanged)
            {
                // A system appeared or went, so the panels have to be built
                // again rather than refreshed: the rows no longer match the
                // systems, and the parameters of a system that went are gone
                // with it. Only when the list changed -- this runs on every
                // step of a drag, and rebuilding every panel each time is what
                // the parameterChanged/sceneChanged split exists to avoid.
                _sceneChanged->setAlways(_sceneChanged->get() + 1);
            }
        }

        void SceneModel::systemChanged(
            const std::string& name,
            const sim::System& before)
        {
            // Already applied, and only to the current system: the caller
            // edited it in place through getSystem(). The before-state is the
            // rest of the list as it stands with that one system put back.
            auto beforeSystems = _systemValues();
            beforeSystems[_currentSystem->get()] = before;
            _record(
                name,
                beforeSystems,
                _currentSystem->get(),
                _particleSize->get());
        }

        void SceneModel::_record(
            const std::string& name,
            const std::vector<sim::System>& beforeSystems,
            size_t beforeCurrentSystem,
            float beforeParticleSize)
        {
            if (_editOpen)
            {
                // Inside a drag. endEdit() records the whole of it.
                applyState(
                    _systemValues(),
                    _currentSystem->get(),
                    _particleSize->get());
                return;
            }
            auto command = std::make_shared<StateCommand>(
                this,
                name,
                beforeSystems,
                beforeCurrentSystem,
                beforeParticleSize,
                _systemValues(),
                _currentSystem->get(),
                _particleSize->get());
            _lastCommand = command;
            // push() runs exec(), which is what applies and re-simulates.
            _commands->push(command);
        }

        void SceneModel::beginEdit()
        {
            if (!_editOpen)
            {
                _editOpen = true;
                _editBeforeSystems = _systemValues();
                _editBeforeCurrentSystem = _currentSystem->get();
                _editBeforeParticleSize = _particleSize->get();
            }
        }

        void SceneModel::endEdit(const std::string& name)
        {
            if (!_editOpen)
                return;
            _editOpen = false;
            if (_editBeforeSystems != _systemValues() ||
                _editBeforeParticleSize != _particleSize->get())
            {
                auto command = std::make_shared<StateCommand>(
                    this,
                    name,
                    _editBeforeSystems,
                    _editBeforeCurrentSystem,
                    _editBeforeParticleSize,
                    _systemValues(),
                    _currentSystem->get(),
                    _particleSize->get());
                _lastCommand = command;
                _commands->push(command);
            }
        }

        std::shared_ptr<IObservable<bool> > SceneModel::observeHasUndo() const
        {
            return _commands->observeHasUndo();
        }

        std::shared_ptr<IObservable<bool> > SceneModel::observeHasRedo() const
        {
            return _commands->observeHasRedo();
        }

        void SceneModel::undo()
        {
            // Whatever was on top is no longer, so nothing may be extended.
            _lastCommand.reset();
            _commands->undo();
        }

        void SceneModel::redo()
        {
            _lastCommand.reset();
            _commands->redo();
        }

        float SceneModel::getParticleSize() const
        {
            return _particleSize->get();
        }

        std::shared_ptr<IObservable<float> > SceneModel::observeParticleSize() const
        {
            return _particleSize;
        }

        void SceneModel::setParticleSize(float value)
        {
            const float before = _particleSize->get();
            if (value == before)
                return;
            _particleSize->setIfChanged(value);
            _record(
                "Set Particle Size",
                _systemValues(),
                _currentSystem->get(),
                before);
        }

        DrawType SceneModel::getDrawType() const
        {
            return _drawType->get();
        }

        std::shared_ptr<IObservable<DrawType> > SceneModel::observeDrawType() const
        {
            return _drawType;
        }

        void SceneModel::setDrawType(DrawType value)
        {
            _drawType->setIfChanged(value);
        }

        const RangeI& SceneModel::getRange() const
        {
            return _range->get();
        }

        std::shared_ptr<IObservable<RangeI> > SceneModel::observeRange() const
        {
            return _range;
        }

        void SceneModel::setRange(const RangeI& value)
        {
            if (!_range->setIfChanged(value))
                return;
            _cache.setRange(value);
            _currentFrame->setIfChanged(
                clamp(_currentFrame->get(), value.min(), value.max()));
            _simulate(_currentFrame->get());
            _modifiedUpdate();
        }

        double SceneModel::getFrameRate() const
        {
            return _frameRate;
        }

        int SceneModel::getCurrentFrame() const
        {
            return _currentFrame->get();
        }

        std::shared_ptr<IObservable<int> > SceneModel::observeCurrentFrame() const
        {
            return _currentFrame;
        }

        void SceneModel::setCurrentFrame(int value)
        {
            const RangeI& range = _range->get();
            if (!_currentFrame->setIfChanged(clamp(value, range.min(), range.max())))
                return;
            _simulate(_currentFrame->get());
        }

        void SceneModel::frameStart()
        {
            setCurrentFrame(_range->get().min());
        }

        void SceneModel::frameEnd()
        {
            setCurrentFrame(_range->get().max());
        }

        void SceneModel::framePrev()
        {
            setCurrentFrame(_currentFrame->get() - 1);
        }

        void SceneModel::frameNext()
        {
            setCurrentFrame(_currentFrame->get() + 1);
        }

        bool SceneModel::isPlaying() const
        {
            return _playing->get();
        }

        std::shared_ptr<IObservable<bool> > SceneModel::observePlaying() const
        {
            return _playing;
        }

        void SceneModel::setPlaying(bool value)
        {
            if (!_playing->setIfChanged(value))
                return;
            if (value)
            {
                auto weak = std::weak_ptr<SceneModel>(shared_from_this());
                _playbackTimer->start(
                    std::chrono::microseconds(
                        static_cast<int64_t>(1000000.0 / _frameRate)),
                    [weak]
                    {
                        if (auto model = weak.lock())
                        {
                            // Loop rather than stop at the end: an effect is
                            // judged by watching it over and over.
                            const RangeI& range = model->_range->get();
                            const int next = model->_currentFrame->get() + 1;
                            model->setCurrentFrame(
                                next > range.max() ? range.min() : next);
                        }
                    });
            }
            else
            {
                _playbackTimer->stop();
            }
        }

        std::shared_ptr<IObservable<std::shared_ptr<const core::Frame> > >
            SceneModel::observeFrame() const
        {
            return _frame;
        }

        std::shared_ptr<IObservableList<core::FrameState> >
            SceneModel::observeCacheStates() const
        {
            return _cacheStates;
        }

        std::shared_ptr<IObservable<size_t> > SceneModel::observeCacheByteCount() const
        {
            return _cacheByteCount;
        }

        void SceneModel::setCurrentLocked(bool value)
        {
            _cache.setLocked(_currentFrame->get(), value);
            _cacheUpdate();
        }

        int64_t SceneModel::getSimTime() const
        {
            return _simTime;
        }

        size_t SceneModel::getParticleCount() const
        {
            const auto& frame = _frame->get();
            return frame ? frame->getParticleCount() : 0;
        }

        size_t SceneModel::getCachedFrameCount() const
        {
            size_t out = 0;
            for (auto state : _cache.getStates())
            {
                if (state != core::FrameState::Empty)
                {
                    ++out;
                }
            }
            return out;
        }

        void SceneModel::_simulate(int frame)
        {
            const auto startTime = std::chrono::steady_clock::now();
            const RangeI& range = _range->get();

            // Start from the last frame the cache still holds, or from nothing
            // at the head of the range.
            const auto lastValid = _cache.getLastValid(frame);
            int at = lastValid ? *lastValid : range.min() - 1;
            std::shared_ptr<const core::Frame> state = lastValid ?
                _cache.get(*lastValid) :
                std::make_shared<core::Frame>();

            // What a system that has no frame yet starts from: the head of the
            // range, and any system added since the frame being stepped from
            // was cached.
            const core::SystemFrame empty;

            while (at < frame)
            {
                ++at;
                auto stepped = std::make_shared<core::Frame>();
                stepped->systems.reserve(_systems.size());
                for (size_t i = 0; i < _systems.size(); ++i)
                {
                    const core::SystemFrame& prev = i < state->systems.size() ?
                        state->systems[i] :
                        empty;
                    stepped->systems.push_back(
                        _systems[i]->step(prev, at, _frameRate));
                }
                std::shared_ptr<const core::Frame> next = stepped;
                _cache.set(at, next);
                // Read it back rather than carrying `next` forward: a locked
                // frame refused the write, and the frame after it has to be
                // built from what is locked, not from what was just computed.
                auto cached = _cache.get(at);
                state = cached ? cached : next;
            }

            _cache.evict(frame);
            // Microseconds. Re-simulating seventy frames is a few
            // milliseconds, and a scrub inside the cache is well under one, so
            // rounding to milliseconds reads zero for everything that is not
            // already slow.
            _simTime = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            _frame->setIfChanged(state);
            _cacheUpdate();
        }

        void SceneModel::_cacheUpdate()
        {
            _cacheStates->setIfChanged(_cache.getStates());
            _cacheByteCount->setIfChanged(_cache.getByteCount());
        }
    }
}
