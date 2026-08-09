// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#include <fx/App/Capture.h>

#include <fx/App/App.h>
#include <fx/App/MainWindow.h>
#include <fx/App/Panels.h>
#include <fx/App/SceneModel.h>
#include <fx/App/Pane.h>
#include <fx/App/Panes.h>

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
            const int timeoutTicks = 200; // 6s hard cap

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
            app->setOffscreen(true);
            window->show();

            try
            {
                for (const auto& step : p.shot.value("setup", nlohmann::json::array()))
                {
                    if (step.contains("click") || step.contains("drag"))
                    {
                        p.lateSteps.push_back(step);
                        continue;
                    }
                    _applyStep(step);
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
            auto panes = app->getMainWindow()->getPanes();

            if (step.contains("frame"))
            {
                model->setCurrentFrame(step.at("frame").get<int>());
            }
            if (step.contains("layout"))
            {
                const int count = step.at("layout").get<int>();
                if (count < 1 || count > paneCountMax)
                    throw std::runtime_error(ftk::Format(
                        "layout must be 1 to {0}").arg(paneCountMax));
                panes->setLayout(static_cast<PaneLayout>(count - 1));
            }
            if (step.contains("view"))
            {
                const auto& view = step.at("view");
                const int index = view.at("index").get<int>();
                if (index < 0 || index >= paneCountMax)
                    throw std::runtime_error("view index out of range");
                ViewType type = ViewType::First;
                const std::string name = view.at("type").get<std::string>();
                if (!fromString(name, type))
                    throw std::runtime_error(ftk::Format(
                        "unknown view type \"{0}\"").arg(name));
                panes->getPane(index)->setViewType(type);
            }
            if (step.contains("pane"))
            {
                const auto& pane = step.at("pane");
                const int index = pane.at("index").get<int>();
                if (index < 0 || index >= paneCountMax)
                    throw std::runtime_error("pane index out of range");
                PaneType type = PaneType::First;
                const std::string name = pane.at("type").get<std::string>();
                if (!fromString(name, type))
                    throw std::runtime_error(ftk::Format(
                        "unknown pane type \"{0}\"").arg(name));
                panes->getPane(index)->setPaneType(type);
            }
            if (step.contains("current"))
            {
                panes->setCurrentIndex(step.at("current").get<int>());
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
                const auto& v = step.at("click");
                if (!v.is_array() || v.size() < 2)
                    throw std::runtime_error("click needs an x and a y");
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
                    ftk::V2I(v[0].get<int>(), v[1].get<int>()),
                    modifiers);
            }
            if (step.contains("drag"))
            {
                const auto& v = step.at("drag");
                if (!v.is_array() || v.size() != 2 ||
                    !v[0].is_array() || !v[1].is_array())
                    throw std::runtime_error("drag needs a from and a to");
                app->getMainWindow()->drag(
                    ftk::V2I(v[0][0].get<int>(), v[0][1].get<int>()),
                    ftk::V2I(v[1][0].get<int>(), v[1][1].get<int>()));
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
            if (p.ticks > timeoutTicks)
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

            std::ofstream f(path);
            f << out.dump(2) << std::endl;
        }
    }
}
