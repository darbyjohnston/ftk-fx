// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/SceneModel.h>

#include <ftk/Core/Context.h>
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
            class SystemCommand : public ftk::ICommand
            {
            public:
                SystemCommand(
                    SceneModel* model,
                    const std::string& name,
                    const sim::System& before,
                    const sim::System& after) :
                    _model(model),
                    _name(name),
                    _before(before),
                    _after(after)
                {}

                const std::string& getName() const { return _name; }
                const sim::System& getAfter() const { return _after; }
                void setAfter(const sim::System& value) { _after = value; }

                void exec() override { _model->applySystem(_after); }
                void undo() override { _model->applySystem(_before); }

            private:
                SceneModel* _model = nullptr;
                std::string _name;
                sim::System _before;
                sim::System _after;
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
            _pointSize = Observable<float>::create(3.F);
            _commands = CommandStack::create();
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
            out.system = _system;
            return out;
        }

        void SceneModel::setScene(const sim::Scene& value)
        {
            _system = value.system;
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

        const sim::System& SceneModel::getSystem() const
        {
            return _system;
        }

        sim::System& SceneModel::getSystem()
        {
            return _system;
        }

        void SceneModel::applySystem(const sim::System& value)
        {
            _system = value;
            _cache.invalidateFrom(_range->get().min());
            _simulate(_currentFrame->get());
            _modifiedUpdate();
            _parameterChanged->setAlways(_parameterChanged->get() + 1);
        }

        void SceneModel::systemChanged(
            const std::string& name,
            const sim::System& before)
        {
            // Already applied: the caller edited the system in place. What is
            // left is to record it and to throw away what it invalidated.
            if (_editDepth > 0)
            {
                // Inside a drag. endEdit() records the whole of it.
                applySystem(_system);
                return;
            }
            auto command = std::make_shared<SystemCommand>(
                this, name, before, _system);
            _lastCommand = command;
            // push() runs exec(), which is what applies and re-simulates.
            _commands->push(command);
        }

        void SceneModel::beginEdit()
        {
            if (0 == _editDepth++)
            {
                _editBefore = _system;
            }
        }

        void SceneModel::endEdit(const std::string& name)
        {
            if (_editDepth > 0 && 0 == --_editDepth && _editBefore != _system)
            {
                auto command = std::make_shared<SystemCommand>(
                    this, name, _editBefore, _system);
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

        float SceneModel::getPointSize() const
        {
            return _pointSize->get();
        }

        std::shared_ptr<IObservable<float> > SceneModel::observePointSize() const
        {
            return _pointSize;
        }

        void SceneModel::setPointSize(float value)
        {
            _pointSize->setIfChanged(value);
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
            return frame ? frame->pool.size() : 0;
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

            while (at < frame)
            {
                ++at;
                auto next = std::make_shared<const core::Frame>(
                    _system.step(*state, at, _frameRate));
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
