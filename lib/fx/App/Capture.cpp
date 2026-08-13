// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Capture.h>

#include <fx/App/App.h>
#include <fx/App/MainWindow.h>
#include <fx/App/Panels.h>
#include <fx/App/ParameterList.h>

#include <ftk/Core/LogSystem.h>
#include <fx/App/SceneModel.h>

#include <fx/Core/Serialize.h>
#include <fx/App/Editor.h>
#include <fx/App/Editors.h>
#include <fx/App/CurveEditor.h>
#include <fx/App/CurveEditorPrivate.h>
#include <fx/App/Viewport.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/IButton.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/LineEdit.h>
#include <ftk/UI/ScreenshotTag.h>

#include <ftk/Core/Context.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

#include <algorithm>
#include <fstream>
#include <iostream>

namespace fx
{
    namespace app
    {
        namespace
        {
            // Budgets in ticks of the timer below, which fires on the clock
            // rather than once per drawn frame -- so these are durations, and
            // the machine being fast or slow does not change them.
            const std::chrono::milliseconds tickInterval(30);
            const int settleTicks = 10;   // 300ms to settle before capturing
            const int timeoutTicks = 200; // 6s hard cap for the default settle

            // Console diagnostics, so a failure is never silent.
            void note(const std::string& shot, const std::string& msg)
            {
                std::cerr << "ftk-fx capture [" << shot << "]: " << msg << std::endl;
            }

            void collect(
                const std::shared_ptr<ftk::IWidget>& widget,
                std::vector<std::shared_ptr<ftk::IWidget> >& out)
            {
                if (!widget)
                    return;
                if (ftk::hasScreenshotTag(widget) && widget->isVisible(true))
                    out.push_back(widget);
                for (const auto& child : widget->getChildren())
                    collect(child, out);
            }

            //! The text a widget is showing, for the kinds that show any. A
            //! tagged widget is often a container rather than the label itself,
            //! so this looks down the tree and joins what it finds: the point
            //! is to make what is on screen readable from the sidecar instead
            //! of by cropping the image and looking at it.
            std::string widgetText(const std::shared_ptr<ftk::IWidget>& widget)
            {
                // Only what is on screen. A hidden child still has its text,
                // and reporting it would say the capture shows something it
                // does not, which is the one thing this must not do.
                if (!widget->isVisible(true))
                    return std::string();

                std::vector<std::string> out;
                if (auto label = std::dynamic_pointer_cast<ftk::Label>(widget))
                {
                    out.push_back(label->getText());
                }
                else if (auto lineEdit = std::dynamic_pointer_cast<ftk::LineEdit>(widget))
                {
                    out.push_back(lineEdit->getText());
                }
                else if (auto comboBox = std::dynamic_pointer_cast<ftk::ComboBox>(widget))
                {
                    const auto& items = comboBox->getItems();
                    const int i = comboBox->getCurrentIndex();
                    if (i >= 0 && i < static_cast<int>(items.size()))
                    {
                        out.push_back(items[i].text);
                    }
                }
                else if (auto button = std::dynamic_pointer_cast<ftk::IButton>(widget))
                {
                    std::string s = button->getText();
                    if (s.empty())
                    {
                        s = button->getIcon();
                    }
                    if (button->isCheckable())
                    {
                        s += button->isChecked() ? " [checked]" : " [unchecked]";
                    }
                    out.push_back(s);
                }
                else
                {
                    for (const auto& child : widget->getChildren())
                    {
                        const std::string s = widgetText(child);
                        if (!s.empty())
                        {
                            out.push_back(s);
                        }
                    }
                }
                return ftk::join(out, ' ');
            }
        }

        struct Capture::Private
        {
            std::weak_ptr<ftk::Context> context;
            std::weak_ptr<App> app;
            std::filesystem::path manifest;
            std::string shotId;
            std::filesystem::path outputDir;
            nlohmann::json shot;

            std::shared_ptr<ftk::Timer> timer;
            int ticks = 0;
            int settleTicksShot = settleTicks;
            int settleLeft = settleTicks;

            // Steps that need the window laid out before they mean anything.
            // A click is a position, and until the first layout there is
            // nothing at any position.
            std::vector<nlohmann::json> lateSteps;
            size_t lateNext = 0;

            //! Errors the application logged while the shot was being set up
            //! and drawn. A shot that provokes one is not worth keeping,
            //! however good the picture looks.
            std::vector<std::string> errors;
            std::shared_ptr<ftk::ListObserver<ftk::LogItem> > logObserver;

            bool done = false;
            bool success = false;
        };

        void Capture::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::filesystem::path& manifest,
            const std::string& shotId,
            const std::filesystem::path& outputDir)
        {
            FTK_P();
            p.context = context;
            p.app = app;
            p.manifest = manifest;
            p.shotId = shotId;
            p.outputDir = outputDir;
        }

        Capture::Capture() :
            _p(new Private)
        {}

        Capture::~Capture()
        {}

        std::shared_ptr<Capture> Capture::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::filesystem::path& manifest,
            const std::string& shotId,
            const std::filesystem::path& outputDir)
        {
            auto out = std::shared_ptr<Capture>(new Capture);
            out->_init(context, app, manifest, shotId, outputDir);
            return out;
        }

        bool Capture::begin()
        {
            FTK_P();
            auto context = p.context.lock();
            auto app = p.app.lock();
            if (!context || !app)
                return false;

            try
            {
                std::ifstream f(p.manifest);
                if (!f.is_open())
                    throw std::runtime_error(ftk::Format(
                        "cannot open manifest \"{0}\"").arg(p.manifest.u8string()));
                nlohmann::json doc;
                f >> doc;
                bool found = false;
                for (const auto& shot : doc.at("shots"))
                {
                    if (shot.value("id", std::string()) == p.shotId)
                    {
                        p.shot = shot;
                        found = true;
                        break;
                    }
                }
                if (!found)
                    throw std::runtime_error("shot id not found in manifest");
            }
            catch (const std::exception& e)
            {
                note(p.shotId, e.what());
                return false;
            }

            if (app->getWindows().empty())
            {
                note(p.shotId, "no window was created");
                return false;
            }
            auto window = app->getWindows().front();

            // A shot may widen the settle window, in seconds. The diagnostics
            // graphs fill from a timer, so a shot of them needs longer than a
            // shot of a viewport that is finished as soon as it is drawn.
            const double settleSeconds = p.shot.value("settle", 0.0);
            if (settleSeconds > 0.0)
            {
                p.settleTicksShot = std::max(
                    settleTicks,
                    static_cast<int>(settleSeconds * 1000.0 / tickInterval.count()));
                p.settleLeft = p.settleTicksShot;
            }

            // The same presentation every time. Capture runs should also pass
            // -resetSettings, so saved state cannot override this.
            app->setColorStyle(ftk::ColorStyle::Dark);
            app->setTooltipsEnabled(false);
            if (p.shot.contains("window"))
            {
                const auto& w = p.shot.at("window");
                if (w.contains("w") && w.contains("h"))
                {
                    window->setSize(ftk::Size2I(
                        w.at("w").get<int>(),
                        w.at("h").get<int>()));
                }
                if (w.contains("scale"))
                {
                    app->setDisplayScale(w.at("scale").get<float>());
                }
                if (w.contains("splitter"))
                {
                    app->getMainWindow()->setSplit(
                        w.at("splitter").get<float>());
                }
            }

            // Nobody is watching a capture run, and a window on screen can be
            // hovered or clicked while the shot is being taken, which puts a
            // highlight or a tooltip into it.
            // Watched from before the setup runs. A shader that will not
            // compile, a file that will not open: the application logs it and
            // carries on drawing something, and the sidecar cannot tell.
            p.logObserver = ftk::ListObserver<ftk::LogItem>::create(
                context->getLogSystem()->observeLogItems(),
                [this](const std::vector<ftk::LogItem>& value)
                {
                    FTK_P();
                    for (const auto& item : value)
                    {
                        if (ftk::LogType::Error == item.type)
                        {
                            p.errors.push_back(item.prefix + ": " + item.message);
                        }
                    }
                },
                ftk::ObserverAction::Suppress);

            app->setOffscreen(true);
            window->show();

            try
            {
                // A click or a drag has to wait for the window to be laid
                // out, and everything after one has to wait for the click:
                // a shot that clicks and then undoes is undoing the click.
                // So the first deferred step defers the rest, and the setup
                // runs in the order it is written.
                bool late = false;
                for (const auto& step : p.shot.value("setup", nlohmann::json::array()))
                {
                    late = late ||
                        step.contains("click") ||
                        step.contains("drag") ||
                        step.contains("dragHold") ||
                        step.contains("keyPress");
                    if (late)
                    {
                        p.lateSteps.push_back(step);
                    }
                    else
                    {
                        _applyStep(step);
                    }
                }
            }
            catch (const std::exception& e)
            {
                note(p.shotId, ftk::Format("setup: {0}").arg(e.what()));
                return false;
            }

            // Arm the capture timer. It fires from inside the event loop, after
            // the window is realized and drawing.
            p.timer = ftk::Timer::create(context);
            p.timer->setRepeating(true);
            auto weak = std::weak_ptr<Capture>(shared_from_this());
            p.timer->start(tickInterval, [weak]
                {
                    if (auto self = weak.lock())
                        self->_onTick();
                });
            return true;
        }

        bool Capture::succeeded() const
        {
            return _p->success;
        }

        void Capture::_applyStep(const nlohmann::json& step)
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return;
            auto model = app->getSceneModel();
            auto editors = app->getMainWindow()->getEditors();

            // The system steps run before everything else in a step, so that
            // "add a system and set its rate" is one step and the rate lands
            // on the system that was just added. Adding, copying and removing
            // each choose a current system of their own; an explicit
            // currentSystem after them is what overrules that.
            if (step.contains("addSystem") && step.at("addSystem").get<bool>())
            {
                model->addSystem();
            }
            if (step.contains("duplicateSystem"))
            {
                model->duplicateSystem(
                    step.at("duplicateSystem").get<size_t>());
            }
            if (step.contains("removeSystem"))
            {
                model->removeSystem(step.at("removeSystem").get<size_t>());
            }
            if (step.contains("currentSystem"))
            {
                model->setCurrentSystem(
                    step.at("currentSystem").get<size_t>());
            }
            if (step.contains("systemName"))
            {
                model->setSystemName(
                    model->getCurrentSystem(),
                    step.at("systemName").get<std::string>());
            }
            if (step.contains("systemEnabled"))
            {
                model->setSystemEnabled(
                    model->getCurrentSystem(),
                    step.at("systemEnabled").get<bool>());
            }

            if (step.contains("frame"))
            {
                model->setCurrentFrame(step.at("frame").get<int>());
            }
            if (step.contains("layout"))
            {
                const int count = step.at("layout").get<int>();
                if (count < 1 || count > editorCountMax)
                    throw std::runtime_error(ftk::Format(
                        "layout must be 1 to {0}").arg(editorCountMax));
                editors->setLayout(static_cast<EditorLayout>(count - 1));
            }
            if (step.contains("view"))
            {
                const auto& view = step.at("view");
                const int index = view.at("index").get<int>();
                if (index < 0 || index >= editorCountMax)
                    throw std::runtime_error("view index out of range");
                ViewType type = ViewType::First;
                const std::string name = view.at("type").get<std::string>();
                if (!fromString(name, type))
                    throw std::runtime_error(ftk::Format(
                        "unknown view type \"{0}\"").arg(name));
                editors->getEditor(index)->setViewType(type);
            }
            if (step.contains("editor"))
            {
                const auto& editor = step.at("editor");
                const int index = editor.at("index").get<int>();
                if (index < 0 || index >= editorCountMax)
                    throw std::runtime_error("editor index out of range");
                EditorType type = EditorType::First;
                const std::string name = editor.at("type").get<std::string>();
                if (!fromString(name, type))
                    throw std::runtime_error(ftk::Format(
                        "unknown editor type \"{0}\"").arg(name));
                editors->getEditor(index)->setEditorType(type);
            }
            if (step.contains("current"))
            {
                editors->setCurrentIndex(step.at("current").get<int>());
            }
            if (step.contains("lock"))
            {
                model->setCurrentLocked(step.at("lock").get<bool>());
            }
            if (step.contains("panel"))
            {
                const auto& panel = step.at("panel");
                auto panels = app->getMainWindow()->getPanels();
                const std::string name = panel.at("name").get<std::string>();
                const auto& names = panels->getPanelNames();
                if (std::find(names.begin(), names.end(), name) == names.end())
                    throw std::runtime_error(ftk::Format(
                        "unknown panel \"{0}\"").arg(name));
                panels->setOpen(name, panel.value("open", true));
            }
            if (step.contains("click"))
            {
                // Through the same resolver a drag point uses, so a click
                // can be aimed at the manipulator rather than at the window.
                const ftk::V2I pos = _dragPoint(step.at("click"));
                int modifiers = 0;
                if (step.contains("modifier"))
                {
                    ftk::KeyModifier modifier = ftk::KeyModifier::None;
                    const std::string name =
                        step.at("modifier").get<std::string>();
                    if (!ftk::from_string(name, modifier))
                        throw std::runtime_error(ftk::Format(
                            "unknown modifier \"{0}\"").arg(name));
                    modifiers = static_cast<int>(modifier);
                }
                app->getMainWindow()->click(
                    pos, ftk::MouseButton::Left, modifiers);
            }
            if (step.contains("drag") || step.contains("dragHold"))
            {
                const bool release = !step.contains("dragHold");
                const auto& v = release ?
                    step.at("drag") :
                    step.at("dragHold");
                if (!v.is_array() || v.size() < 2)
                    throw std::runtime_error(
                        "drag needs a from and a to, and takes more");
                std::vector<ftk::V2I> path;
                for (const auto& p : v)
                {
                    path.push_back(_dragPoint(p));
                }
                app->getMainWindow()->drag(path, 0, release);
            }
            if (step.contains("keyPress"))
            {
                // A key sent to the window, which is where shortcuts are
                // dispatched from. Named keyPress rather than key because a
                // "key" here has always meant a keyframe.
                const std::string name = step.at("keyPress").get<std::string>();
                ftk::Key key = ftk::Key::Unknown;
                if (!ftk::from_string(name, key))
                    throw std::runtime_error(ftk::Format(
                        "unknown key \"{0}\"").arg(name));
                int modifiers = 0;
                if (step.contains("modifier"))
                {
                    const std::string m =
                        step.at("modifier").get<std::string>();
                    if ("Shift" == m)
                        modifiers = static_cast<int>(ftk::KeyModifier::Shift);
                    else if ("Control" == m)
                        modifiers = static_cast<int>(ftk::KeyModifier::Control);
                    else if ("Alt" == m)
                        modifiers = static_cast<int>(ftk::KeyModifier::Alt);
                    else if ("Super" == m)
                        modifiers = static_cast<int>(ftk::KeyModifier::Super);
                    else
                        throw std::runtime_error(ftk::Format(
                            "unknown modifier \"{0}\"").arg(m));
                }
                app->getMainWindow()->keyPress(key, modifiers);
            }
            if (step.contains("panelStyle"))
            {
                const std::string name = step.at("panelStyle").get<std::string>();
                auto panels = app->getMainWindow()->getPanels();
                if ("Tabs" == name)
                    panels->setStyle(PanelStyle::Tabs);
                else if ("Column" == name)
                    panels->setStyle(PanelStyle::Column);
                else
                    throw std::runtime_error(ftk::Format(
                        "unknown panel style \"{0}\"").arg(name));
            }
            if (step.contains("key"))
            {
                const auto& k = step.at("key");
                const std::string path = k.at("path").get<std::string>();
                core::Parameter* parameter = nullptr;
                for (const auto& info : getParameters(model->getSystem()))
                {
                    if (info.getPath() == path)
                    {
                        parameter = info.parameter;
                        break;
                    }
                }
                if (!parameter)
                    throw std::runtime_error(ftk::Format(
                        "unknown parameter \"{0}\"").arg(path));
                core::Curve curve =
                    core::Parameter::Type::Curve == parameter->getType() ?
                    parameter->getCurve() :
                    core::Curve();
                core::Key key;
                key.frame = k.at("frame").get<double>();
                key.value = k.at("value").get<float>();
                if (k.contains("interp"))
                {
                    core::from_json(k.at("interp"), key.interp);
                }
                const sim::System before = model->getSystem();
                curve.addKey(key);
                parameter->setCurve(curve);
                model->systemChanged("Key " + path, before);
            }
            if (step.contains("frameView") && step.at("frameView").get<bool>())
            {
                auto editors = app->getMainWindow()->getEditors();
                for (int i = 0; i < getEditorCount(editors->getLayout()); ++i)
                {
                    if (auto viewport = editors->getEditor(i)->getViewport())
                    {
                        viewport->frameView();
                    }
                }
            }
            if (step.contains("undo"))
            {
                for (int i = 0; i < step.at("undo").get<int>(); ++i)
                {
                    model->undo();
                }
            }
            if (step.contains("redo"))
            {
                for (int i = 0; i < step.at("redo").get<int>(); ++i)
                {
                    model->redo();
                }
            }
            if (step.contains("save"))
            {
                model->save(step.at("save").get<std::string>());
            }
            if (step.contains("newScene") && step.at("newScene").get<bool>())
            {
                model->newScene();
            }
            if (step.contains("open"))
            {
                model->open(step.at("open").get<std::string>());
            }
            if (step.contains("curveValueMode"))
            {
                const std::string name =
                    step.at("curveValueMode").get<std::string>();
                const auto labels = getCurveValueModeLabels();
                const auto i = std::find(labels.begin(), labels.end(), name);
                if (i == labels.end())
                    throw std::runtime_error(ftk::Format(
                        "unknown curve value mode \"{0}\"").arg(name));
                auto editors = app->getMainWindow()->getEditors();
                for (int j = 0; j < editorCountMax; ++j)
                {
                    if (auto editor = editors->getEditor(j)->getCurveEditor())
                    {
                        editor->setValueMode(static_cast<CurveValueMode>(
                            std::distance(labels.begin(), i)));
                    }
                }
            }
            if (step.contains("transform"))
            {
                const auto& t = step.at("transform");
                const sim::System before = model->getSystem();
                auto& transform = model->getSystem().getEmitter().transform;
                const auto set = [](core::V3Parameter& p, const nlohmann::json& v)
                {
                    p.x.setConstant(v[0].get<float>());
                    p.y.setConstant(v[1].get<float>());
                    p.z.setConstant(v[2].get<float>());
                };
                if (t.contains("translate")) set(transform.translate, t.at("translate"));
                if (t.contains("rotate")) set(transform.rotate, t.at("rotate"));
                if (t.contains("scale")) set(transform.scale, t.at("scale"));
                model->systemChanged("Set Transform", before);
            }
            if (step.contains("shape"))
            {
                const auto& v = step.at("shape");
                const std::string name = v.at("type").get<std::string>();
                sim::EmitterShape shape = sim::EmitterShape::First;
                if (!fromString(name, shape))
                    throw std::runtime_error(ftk::Format(
                        "unknown emitter shape \"{0}\"").arg(name));
                const sim::System before = model->getSystem();
                auto& emitter = model->getSystem().getEmitter();
                emitter.shape = shape;
                if (v.contains("surface"))
                {
                    emitter.surface = v.at("surface").get<bool>();
                }
                if (v.contains("size"))
                {
                    const auto& size = v.at("size");
                    emitter.size.x.setConstant(size[0].get<float>());
                    emitter.size.y.setConstant(size[1].get<float>());
                    emitter.size.z.setConstant(size[2].get<float>());
                }
                model->systemChanged("Set Shape", before);
            }
            if (step.contains("rate"))
            {
                // The one simulation parameter a shot can set. Frame time is
                // worth measuring against a known particle count, and the
                // count follows the rate.
                const sim::System before = model->getSystem();
                model->getSystem().getEmitter().rate.setConstant(
                    step.at("rate").get<float>());
                model->systemChanged("Set Rate", before);
            }
            if (step.contains("draw"))
            {
                const std::string name = step.at("draw").get<std::string>();
                DrawType drawType = DrawType::First;
                if (!fromString(name, drawType))
                    throw std::runtime_error(ftk::Format(
                        "unknown draw type \"{0}\"").arg(name));
                model->setDrawType(drawType);
            }
            if (step.contains("gizmo"))
            {
                const std::string name = step.at("gizmo").get<std::string>();
                GizmoMode mode = GizmoMode::First;
                if (!fromString(name, mode))
                    throw std::runtime_error(ftk::Format(
                        "unknown manipulator mode \"{0}\"").arg(name));
                model->setGizmoMode(mode);
            }
            if (step.contains("particleSize"))
            {
                model->setParticleSize(step.at("particleSize").get<float>());
            }
            if (step.contains("playing"))
            {
                model->setPlaying(step.at("playing").get<bool>());
            }
        }

        void Capture::_onTick()
        {
            FTK_P();
            if (p.done)
                return;
            ++p.ticks;
            // The cap allows for one settle per late step and one to capture
            // after them, so a shot that asks to sit and watch something for
            // eight seconds is not cut off at six.
            const int timeout = std::max<int>(
                timeoutTicks,
                p.settleTicksShot * static_cast<int>(p.lateSteps.size() + 2));
            if (p.ticks > timeout)
            {
                note(p.shotId, "timed out waiting for the shot to settle");
                _finish(false);
                return;
            }
            if (--p.settleLeft > 0)
                return;

            if (p.lateNext < p.lateSteps.size())
            {
                // One per settle rather than all at once, so each is drawn
                // before the next runs: a click on what a previous click
                // opened needs the popup to have reached the screen.
                const nlohmann::json step = p.lateSteps[p.lateNext++];
                try
                {
                    _applyStep(step);
                }
                catch (const std::exception& e)
                {
                    note(p.shotId, ftk::Format("setup: {0}").arg(e.what()));
                    _finish(false);
                    return;
                }
                p.settleLeft = p.settleTicksShot;
                return;
            }

            if (!p.errors.empty())
            {
                note(p.shotId, ftk::Format("the application logged: {0}").
                    arg(p.errors.front()));
                _finish(false);
                return;
            }

            // Checked before the picture is written, so a shot whose
            // coordinates have gone stale says so instead of quietly
            // photographing a scene nobody edited.
            const std::string expectFailure = _expectFailure();
            if (!expectFailure.empty())
            {
                note(p.shotId, expectFailure);
                _finish(false);
                return;
            }

            std::error_code ec;
            std::filesystem::create_directories(p.outputDir, ec);
            const auto png = p.outputDir / (p.shotId + ".png");
            const auto json = p.outputDir / (p.shotId + ".json");
            const bool ok = _writePNG(png);
            if (ok)
            {
                _writeMetadata(json);
                note(p.shotId, ftk::Format("captured {0}").arg(png.u8string()));
            }
            _finish(ok);
        }

        void Capture::_finish(bool ok)
        {
            FTK_P();
            p.success = ok;
            p.done = true;
            if (p.timer)
                p.timer->stop();
            if (auto app = p.app.lock())
                app->exit();
        }

        bool Capture::_writePNG(const std::filesystem::path& path) const
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app)
                return false;
            if (!app->writeScreenshot(path))
            {
                note(p.shotId, ftk::Format("cannot capture \"{0}\"").
                    arg(path.u8string()));
                return false;
            }
            return true;
        }

        std::string Capture::_expectFailure() const
        {
            FTK_P();
            if (!p.shot.contains("expect"))
                return std::string();
            auto app = p.app.lock();
            if (!app || app->getWindows().empty())
                return "no window to look at";
            std::vector<std::shared_ptr<ftk::IWidget> > tagged;
            collect(app->getWindows().front(), tagged);

            for (const auto& i : p.shot.at("expect").items())
            {
                bool found = false;
                for (const auto& w : tagged)
                {
                    if (ftk::getScreenshotTag(w) != i.key())
                        continue;
                    found = true;
                    const std::string text = widgetText(w);
                    const std::string want = i.value().get<std::string>();
                    if (text.find(want) == std::string::npos)
                        return ftk::Format("{0} reads \"{1}\", expected \"{2}\"").
                            arg(i.key()).arg(text).arg(want);
                    break;
                }
                if (!found)
                    return ftk::Format("nothing tagged {0}").arg(i.key());
            }
            return std::string();
        }

        ftk::V2I Capture::_dragPoint(const nlohmann::json& value) const
        {
            FTK_P();
            if (value.is_array() && value.size() == 2)
            {
                return ftk::V2I(value[0].get<int>(), value[1].get<int>());
            }
            // Anchored on the manipulator instead. A window pixel is a
            // coordinate in the chrome as much as in the scene, so anything
            // that changes the size of a toolbar moves every drag that was
            // aiming at a gizmo -- and moves it just far enough to grab the
            // ring next to the one that was meant.
            if (!value.is_object())
            {
                throw std::runtime_error(
                    "a drag point is [x, y], or one of "
                    "{\"axis\": \"x\", \"at\": 1.5}, {\"arm\": [x, y]}, "
                    "{\"ring\": [radius, degrees]}, {\"units\": [x, y]}, "
                    "{\"curve\": [frame, value]}");
            }
            auto app = p.app.lock();
            auto editor = app ?
                app->getMainWindow()->getEditors()->getCurrent() : nullptr;
            if (!editor)
                throw std::runtime_error("no editor to place a drag against");
            if (value.contains("curve"))
            {
                // A frame and a value in the plot, which is what a curve
                // drag is actually aimed at. In pixels it survives only
                // until the plot's own geometry changes -- and then it goes
                // on passing while dragging whatever moved under it.
                auto curveEditor = editor->getCurveEditor();
                if (!curveEditor)
                    throw std::runtime_error("no curve editor to place a drag against");
                const auto& v = value.at("curve");
                if (!v.is_array() || v.size() != 2)
                    throw std::runtime_error("\"curve\" takes a frame and a value");
                const size_t channel = value.contains("channel") ?
                    value.at("channel").get<size_t>() : 0;
                return curveEditor->getGraph()->getPos(
                    channel, v[0].get<double>(), v[1].get<float>());
            }

            auto viewport = editor->getViewport();
            if (!viewport)
                throw std::runtime_error("no viewport to place a drag against");
            const auto arms = viewport->getGizmoArms();
            ftk::V2F origin;
            float armLength = 0.F;
            float pixelsPerUnit = 0.F;
            for (const auto& arm : arms)
            {
                if (!arm.valid)
                    continue;
                origin = arm.origin;
                const ftk::V2F d(
                    arm.tip.x - arm.origin.x, arm.tip.y - arm.origin.y);
                armLength = std::sqrt(d.x * d.x + d.y * d.y);
                pixelsPerUnit = arm.pixelsPerUnit;
                break;
            }
            if (armLength <= 0.F)
                throw std::runtime_error("the manipulator has no arms to measure");

            if (value.contains("axis"))
            {
                // A distance along one named arm, in arm lengths. This is
                // what an arm drag is actually described as, and unlike an
                // offset it stays right when the camera moves and the arm
                // points somewhere else on screen.
                const std::string name = value.at("axis").get<std::string>();
                const size_t i = std::string("xyz").find(name);
                if (name.size() != 1 || i == std::string::npos)
                    throw std::runtime_error(ftk::Format(
                        "\"axis\" is x, y or z, not \"{0}\"").arg(name));
                if (!arms[i].valid)
                    throw std::runtime_error(ftk::Format(
                        "the {0} arm has nowhere to point").arg(name));
                // Either in arm lengths, or in scene units for the drags
                // whose point is the number the panel ends up showing --
                // those move with the zoom, and the arm does not.
                const float at = value.contains("units") ?
                    value.at("units").get<float>() * arms[i].pixelsPerUnit /
                        armLength :
                    value.at("at").get<float>();
                return ftk::V2I(
                    static_cast<int>(std::round(
                        arms[i].origin.x + arms[i].dir.x * at * armLength)),
                    static_cast<int>(std::round(
                        arms[i].origin.y + arms[i].dir.y * at * armLength)));
            }

            const auto pair = [&value](const char* key, float& a, float& b)
            {
                const auto& v = value.at(key);
                if (!v.is_array() || v.size() != 2)
                    throw std::runtime_error(ftk::Format(
                        "\"{0}\" takes two numbers").arg(key));
                a = v[0].get<float>();
                b = v[1].get<float>();
            };
            float x = 0.F;
            float y = 0.F;
            if (value.contains("arm"))
            {
                // In arm lengths, which is the manipulator's own unit: it is
                // drawn a fixed number of pixels long, so this survives the
                // display scale as well as the chrome.
                pair("arm", x, y);
                x *= armLength;
                y *= armLength;
            }
            else if (value.contains("ring"))
            {
                // Radius in arm lengths, angle in degrees clockwise from the
                // right. Rings are the reason this exists, and a bearing is
                // how a turn round one is actually described.
                float radius = 0.F;
                float degrees = 0.F;
                pair("ring", radius, degrees);
                const float a = ftk::deg2rad(degrees);
                x = std::cos(a) * radius * armLength;
                y = std::sin(a) * radius * armLength;
            }
            else if (value.contains("units"))
            {
                // Scene units per screen axis, y up. For the drags whose
                // point is the number the panel ends up showing: those move
                // with the zoom, which the chrome also changes.
                pair("units", x, y);
                x *= pixelsPerUnit;
                y *= -pixelsPerUnit;
            }
            else
            {
                throw std::runtime_error(
                    "a drag point needs \"axis\", \"arm\", \"ring\" "
                    "or \"units\"");
            }
            return ftk::V2I(
                static_cast<int>(std::round(origin.x + x)),
                static_cast<int>(std::round(origin.y + y)));
        }

        void Capture::_writeMetadata(const std::filesystem::path& path) const
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app || app->getWindows().empty())
                return;
            auto window = app->getWindows().front();

            std::vector<std::shared_ptr<ftk::IWidget> > tagged;
            collect(window, tagged);

            nlohmann::json widgets = nlohmann::json::array();
            for (const auto& w : tagged)
            {
                const ftk::Box2I g = w->getGeometry();
                nlohmann::json widget = {
                    { "id", ftk::getScreenshotTag(w) },
                    { "box", { g.x(), g.y(), g.w(), g.h() } } };
                const std::string text = widgetText(w);
                if (!text.empty())
                {
                    widget["text"] = text;
                }
                // A box is not a thing on screen. A widget scrolled past the
                // bottom of its panel, or clipped by one, still has a
                // perfectly good geometry -- and a shot aimed at it hits
                // nothing. Recorded so that reading the coordinates out of
                // here says which of the two it is.
                if (w->isClipped())
                {
                    widget["clipped"] = true;
                }
                widgets.push_back(widget);
            }

            // The boxes and the image share the offscreen buffer's pixel space
            // (window size times display scale), so they line up whatever the
            // scale is. The scale goes in as "dpr" so a tool can show a
            // high-resolution capture at its logical size: crisp, not enlarged.
            const ftk::Size2I size = window->getGeometry().size();
            nlohmann::json out = {
                { "shot", p.shotId },
                { "image", p.shotId + ".png" },
                { "dpr", app->getDisplayScale() },
                { "window", { { "w", size.w }, { "h", size.h } } },
                { "widgets", widgets } };
            if (p.shot.contains("annotate"))
            {
                out["annotate"] = p.shot.at("annotate");
            }

            // Where the manipulator's arms landed, in the same pixel space as
            // the boxes. A drag aimed at an arm was authored by reading a
            // picture, and a drag that misses still writes a screenshot --
            // this is the arm saying where it is instead.
            if (auto editor = app->getMainWindow()->getEditors()->getCurrent())
            {
                if (auto viewport = editor->getViewport())
                {
                    nlohmann::json arms = nlohmann::json::array();
                    ftk::V2F origin;
                    for (const auto& arm : viewport->getGizmoArms())
                    {
                        if (!arm.valid)
                            continue;
                        origin = arm.origin;
                        // The tip is a fixed distance along the arm, so it
                        // gives the direction and says nothing about scale.
                        // Authoring a drag of a known world distance needs
                        // the pixels a unit covers, which is this.
                        arms.push_back({
                            { "tip", { arm.tip.x, arm.tip.y } },
                            { "pixelsPerUnit", arm.pixelsPerUnit } });
                    }
                    if (!arms.empty())
                    {
                        out["gizmo"] = {
                            { "mode", getLabel(viewport->getGizmoMode()) },
                            { "origin", { origin.x, origin.y } },
                            { "arms", arms } };
                    }
                }
            }

            std::ofstream f(path);
            f << out.dump(2) << std::endl;
        }
    }
}
