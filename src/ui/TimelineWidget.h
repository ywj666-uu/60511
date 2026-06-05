#pragma once

#include <QWidget>
#include <QVector>
#include "model/SubtitleSegment.h"

class TimelineWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget *parent = nullptr);

    void setDuration(qint64 durationMs);
    void setSegments(const QVector<SubtitleSegment> &segments);
    void setFrameDuration(double frameDurationMs);

public slots:
    void setPlayheadPosition(qint64 positionMs);

signals:
    void segmentSelected(int segmentIndex);
    void segmentTimingChanged(int segmentIndex, qint64 newStartMs, qint64 newEndMs);
    void playheadMoved(qint64 positionMs);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    enum class DragMode { None, MoveSegment, ResizeLeft, ResizeRight, Playhead };

    static constexpr int RULER_HEIGHT = 30;
    static constexpr int SEGMENT_HEIGHT = 40;
    static constexpr int SEGMENT_Y = 40;
    static constexpr int EDGE_GRAB_WIDTH = 6;
    static constexpr qint64 SNAP_THRESHOLD_MS = 80;

    QRectF segmentRect(int index) const;
    qint64 pixelToTime(double px) const;
    double timeToPixel(qint64 ms) const;
    int hitTestSegment(const QPoint &pos) const;
    DragMode hitTestEdge(const QPoint &pos, int segmentIndex) const;

    qint64 snapToAdjacentBoundary(qint64 timeMs, int excludeIndex) const;
    QPair<qint64, qint64> snapSegmentEdges(qint64 startMs, qint64 endMs, int excludeIndex, DragMode mode) const;
    void frameStepSelectedSegment(int direction, bool moveStart, bool moveEnd);

    QVector<SubtitleSegment> m_segments;
    qint64   m_durationMs = 0;
    qint64   m_playheadMs = 0;
    double   m_pixelsPerSecond = 100.0;
    double   m_scrollOffset = 0.0;
    int      m_selectedIndex = -1;
    DragMode m_dragMode = DragMode::None;
    int      m_dragSegmentIndex = -1;
    QPoint   m_dragStartPos;
    qint64   m_dragOrigStartMs = 0;
    qint64   m_dragOrigEndMs = 0;
    double   m_frameDurationMs = 33.3667; // default ~30fps
    bool     m_snappedThisDrag = false;
};
