// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the ftk-fx project.

#pragma once

#include <fx/App/CurveEditor.h>

#include <ftk/UI/IMouseWidget.h>

#include <ftk/Core/Observable.h>

namespace fx
{
    namespace app
    {
        class SceneModel;

        //! The plot: the curves themselves, and the keys that can be dragged.
        class CurveGraph : public ftk::IMouseWidget
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent);

            CurveGraph() = default;

        public:
            virtual ~CurveGraph();

            static std::shared_ptr<CurveGraph> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<SceneModel>&,
                const std::shared_ptr<ftk::IWidget>& parent = nullptr);

            //! Set which parameters are drawn. The order fixes the colours, so
            //! a channel keeps its colour as others are shown and hidden.
            void setChannels(const std::vector<ParameterInfo>&);

            void setValueMode(CurveValueMode);

            ftk::Size2I getSizeHint() const override;
            void styleEvent(const ftk::StyleEvent&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;
            void mouseReleaseEvent(ftk::MouseClickEvent&) override;
            void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            void keyPressEvent(ftk::KeyEvent&) override;

        private:
            //! Recompute the value range the plot covers, from the keys rather
            //! than from the parameters' slider ranges: a rate that lives
            //! between 100 and 200 should not be drawn as a flat line across
            //! the bottom of a plot that goes to 2000.
            //!
            //! Never called during a drag. The range is derived from the keys,
            //! so recomputing it while one is being moved re-scales the axis
            //! under the pointer, which moves the key, which re-scales the
            //! axis: the key chases the cursor and settles somewhere neither
            //! of them chose.
            void _valueRangeUpdate();

            //! The range a channel is drawn against: its own when normalized,
            //! the one shared by all of them when not.
            const ftk::RangeF& _getValueRange(size_t channel) const;

            //! Frame and value to a point in the widget, and back.
            ftk::V2F _toPos(size_t channel, double frame, float value) const;
            double _toFrame(int x) const;
            float _toValue(size_t channel, int y) const;

            //! The key nearest the position, within grabbing distance.
            bool _hit(const ftk::V2I&, size_t& channel, size_t& key) const;

            //! Write a key back through the parameter, and tell the model.
            void _moveKey(size_t channel, size_t key, double frame, float value);

            std::weak_ptr<SceneModel> _model;
            std::vector<ParameterInfo> _channels;
            ftk::RangeI _range = ftk::RangeI(1, 120);
            CurveValueMode _valueMode = CurveValueMode::Absolute;

            //! The range shared by every channel, and one per channel.
            ftk::RangeF _valueRange = ftk::RangeF(0.F, 1.F);
            std::vector<ftk::RangeF> _channelRanges;
            int _currentFrame = 1;

            //! Which key is selected, and which is being dragged. The same key
            //! most of the time; selection outlives the drag so that Delete
            //! has something to act on.
            size_t _selectedChannel = 0;
            size_t _selectedKey = 0;
            bool _hasSelection = false;
            bool _dragging = false;

            struct SizeData
            {
                bool init = true;
                int margin = 0;
                int border = 0;
                int handle = 0;
                ftk::FontInfo fontInfo;
                ftk::FontMetrics fontMetrics;
            };
            SizeData _size;

            std::shared_ptr<ftk::Observer<ftk::RangeI> > _rangeObserver;
            std::shared_ptr<ftk::Observer<int> > _currentFrameObserver;
            std::shared_ptr<ftk::Observer<int> > _parameterObserver;
            std::shared_ptr<ftk::Observer<int> > _sceneObserver;
        };
    }
}
