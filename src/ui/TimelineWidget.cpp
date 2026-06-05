#include "TimelineWidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QFontMetrics>
#include <QtMath>

TimelineWidget::TimelineWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(100);
    setFocusPolicy(Qt::ClickFocus);
}

void TimelineWidget::setDuration(qint64 durationMs)
{
    m_durationMs = durationMs;
    update();
}

void TimelineWidget::setSegments(const QVector<SubtitleSegment> &segments)
{
    m_segments = segments;
    update();
}

void TimelineWidget::setPlayheadPosition(qint64 positionMs)
{
    m_playheadMs = positionMs;
    update();
}

void TimelineWidget::setFrameDuration(double frameDurationMs)
{
    m_frameDurationMs = frameDurationMs;
}

QSize TimelineWidget::sizeHint() const
{
    return QSize(800, 120);
}

QSize TimelineWidget::minimumSizeHint() const
{
    return QSize(400, 80);
}

double TimelineWidget::timeToPixel(qint64 ms) const
{
    return (ms / 1000.0) * m_pixelsPerSecond - m_scrollOffset;
}

qint64 TimelineWidget::pixelToTime(double px) const
{
    return static_cast<qint64>(((px + m_scrollOffset) / m_pixelsPerSecond) * 1000.0);
}

QRectF TimelineWidget::segmentRect(int index) const
{
    if (index < 0 || index >= m_segments.size()) return QRectF();
    const auto &seg = m_segments[index];
    double x1 = timeToPixel(seg.effectiveStartMs());
    double x2 = timeToPixel(seg.effectiveEndMs());
    return QRectF(x1, SEGMENT_Y, x2 - x1, SEGMENT_HEIGHT);
}

int TimelineWidget::hitTestSegment(const QPoint &pos) const
{
    for (int i = 0; i < m_segments.size(); ++i) {
        if (segmentRect(i).contains(pos)) {
            return i;
        }
    }
    return -1;
}

TimelineWidget::DragMode TimelineWidget::hitTestEdge(const QPoint &pos, int segmentIndex) const
{
    QRectF rect = segmentRect(segmentIndex);
    if (qAbs(pos.x() - rect.left()) <= EDGE_GRAB_WIDTH) return DragMode::ResizeLeft;
    if (qAbs(pos.x() - rect.right()) <= EDGE_GRAB_WIDTH) return DragMode::ResizeRight;
    return DragMode::MoveSegment;
}

qint64 TimelineWidget::snapToAdjacentBoundary(qint64 timeMs, int excludeIndex) const
{
    qint64 bestSnap = timeMs;
    qint64 bestDist = SNAP_THRESHOLD_MS + 1;

    for (int i = 0; i < m_segments.size(); ++i) {
        if (i == excludeIndex) continue;
        const auto &seg = m_segments[i];

        qint64 segStart = seg.effectiveStartMs();
        qint64 segEnd = seg.effectiveEndMs();

        qint64 distToStart = qAbs(timeMs - segStart);
        if (distToStart < bestDist) {
            bestDist = distToStart;
            bestSnap = segStart;
        }

        qint64 distToEnd = qAbs(timeMs - segEnd);
        if (distToEnd < bestDist) {
            bestDist = distToEnd;
            bestSnap = segEnd;
        }
    }

    return bestSnap;
}

QPair<qint64, qint64> TimelineWidget::snapSegmentEdges(qint64 startMs, qint64 endMs, int excludeIndex, DragMode mode) const
{
    qint64 snappedStart = startMs;
    qint64 snappedEnd = endMs;

    switch (mode) {
    case DragMode::MoveSegment: {
        qint64 snapStart = snapToAdjacentBoundary(startMs, excludeIndex);
        qint64 snapEnd = snapToAdjacentBoundary(endMs, excludeIndex);
        qint64 distStart = qAbs(snapStart - startMs);
        qint64 distEnd = qAbs(snapEnd - endMs);

        if (distStart <= SNAP_THRESHOLD_MS && distStart <= distEnd) {
            qint64 duration = endMs - startMs;
            snappedStart = snapStart;
            snappedEnd = snapStart + duration;
        } else if (distEnd <= SNAP_THRESHOLD_MS) {
            qint64 duration = endMs - startMs;
            snappedEnd = snapEnd;
            snappedStart = snapEnd - duration;
        }
        break;
    }
    case DragMode::ResizeLeft: {
        qint64 snap = snapToAdjacentBoundary(startMs, excludeIndex);
        if (qAbs(snap - startMs) <= SNAP_THRESHOLD_MS) {
            snappedStart = snap;
        }
        break;
    }
    case DragMode::ResizeRight: {
        qint64 snap = snapToAdjacentBoundary(endMs, excludeIndex);
        if (qAbs(snap - endMs) <= SNAP_THRESHOLD_MS) {
            snappedEnd = snap;
        }
        break;
    }
    default:
        break;
    }

    return {snappedStart, snappedEnd};
}

void TimelineWidget::frameStepSelectedSegment(int direction, bool moveStart, bool moveEnd)
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_segments.size()) return;

    auto &seg = m_segments[m_selectedIndex];
    qint64 stepMs = static_cast<qint64>(qRound(m_frameDurationMs));
    qint64 delta = direction * stepMs;

    qint64 newStart = seg.effectiveStartMs();
    qint64 newEnd = seg.effectiveEndMs();

    if (moveStart) newStart = qMax(qint64(0), newStart + delta);
    if (moveEnd) newEnd = qMax(newStart + stepMs, newEnd + delta);

    seg.finalStartMs = newStart;
    seg.finalEndMs = newEnd;

    emit segmentTimingChanged(m_selectedIndex, newStart, newEnd);
    update();
}

void TimelineWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), QColor(40, 40, 40));

    // Time ruler
    p.setPen(QColor(180, 180, 180));
    QFont rulerFont = font();
    rulerFont.setPointSize(8);
    p.setFont(rulerFont);

    double startSec = m_scrollOffset / m_pixelsPerSecond;
    double endSec = startSec + width() / m_pixelsPerSecond;

    double tickInterval = 1.0;
    if (m_pixelsPerSecond < 20) tickInterval = 10.0;
    else if (m_pixelsPerSecond < 50) tickInterval = 5.0;

    for (double t = std::floor(startSec / tickInterval) * tickInterval; t <= endSec; t += tickInterval) {
        if (t < 0) continue;
        double x = timeToPixel(static_cast<qint64>(t * 1000));
        p.drawLine(QPointF(x, 0), QPointF(x, RULER_HEIGHT));

        int sec = static_cast<int>(t);
        int mm = sec / 60;
        int ss = sec % 60;
        p.drawText(QPointF(x + 2, RULER_HEIGHT - 5), QString("%1:%2").arg(mm, 2, 10, QChar('0')).arg(ss, 2, 10, QChar('0')));
    }

    p.setPen(QColor(100, 100, 100));
    p.drawLine(0, RULER_HEIGHT, width(), RULER_HEIGHT);

    // Draw segments
    for (int i = 0; i < m_segments.size(); ++i) {
        QRectF r = segmentRect(i);
        if (r.right() < 0 || r.left() > width()) continue;

        QColor fillColor(70, 130, 200);
        if (m_segments[i].isResolved) {
            fillColor = QColor(70, 180, 100);
        }
        if (i == m_selectedIndex) {
            fillColor = fillColor.lighter(140);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(fillColor);
        p.drawRoundedRect(r, 3, 3);

        if (i == m_selectedIndex) {
            p.setPen(QPen(QColor(255, 180, 0), 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r, 3, 3);
        }

        // Snap indicator: draw a thin vertical yellow line at snapped edges
        if (m_snappedThisDrag && i == m_dragSegmentIndex) {
            p.setPen(QPen(QColor(255, 255, 0), 1, Qt::DashLine));
            p.drawLine(QPointF(r.left(), SEGMENT_Y - 5), QPointF(r.left(), SEGMENT_Y + SEGMENT_HEIGHT + 5));
            p.drawLine(QPointF(r.right(), SEGMENT_Y - 5), QPointF(r.right(), SEGMENT_Y + SEGMENT_HEIGHT + 5));
        }

        p.setPen(Qt::white);
        QRectF textRect = r.adjusted(4, 2, -4, -2);
        QString text = m_segments[i].effectiveText();
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                   p.fontMetrics().elidedText(text, Qt::ElideRight, static_cast<int>(textRect.width())));
    }

    // Playhead
    double phX = timeToPixel(m_playheadMs);
    p.setPen(QPen(QColor(255, 50, 50), 2));
    p.drawLine(QPointF(phX, 0), QPointF(phX, height()));

    QPolygonF triangle;
    triangle << QPointF(phX - 5, 0) << QPointF(phX + 5, 0) << QPointF(phX, 8);
    p.setBrush(QColor(255, 50, 50));
    p.setPen(Qt::NoPen);
    p.drawPolygon(triangle);
}

void TimelineWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    QPoint pos = event->pos();
    m_snappedThisDrag = false;

    if (pos.y() < RULER_HEIGHT) {
        m_dragMode = DragMode::Playhead;
        qint64 time = pixelToTime(pos.x());
        m_playheadMs = qBound(qint64(0), time, m_durationMs);
        emit playheadMoved(m_playheadMs);
        update();
        return;
    }

    int hitIndex = hitTestSegment(pos);
    if (hitIndex >= 0) {
        m_selectedIndex = hitIndex;
        m_dragSegmentIndex = hitIndex;
        m_dragMode = hitTestEdge(pos, hitIndex);
        m_dragStartPos = pos;
        m_dragOrigStartMs = m_segments[hitIndex].effectiveStartMs();
        m_dragOrigEndMs = m_segments[hitIndex].effectiveEndMs();
        emit segmentSelected(hitIndex);
    } else {
        m_selectedIndex = -1;
        m_dragMode = DragMode::Playhead;
        qint64 time = pixelToTime(pos.x());
        m_playheadMs = qBound(qint64(0), time, m_durationMs);
        emit playheadMoved(m_playheadMs);
    }

    update();
}

void TimelineWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();

    if (m_dragMode == DragMode::Playhead) {
        qint64 time = pixelToTime(pos.x());
        m_playheadMs = qBound(qint64(0), time, m_durationMs);
        emit playheadMoved(m_playheadMs);
        update();
        return;
    }

    if (m_dragMode == DragMode::None) {
        int hitIndex = hitTestSegment(pos);
        if (hitIndex >= 0) {
            DragMode mode = hitTestEdge(pos, hitIndex);
            if (mode == DragMode::ResizeLeft || mode == DragMode::ResizeRight) {
                setCursor(Qt::SizeHorCursor);
            } else {
                setCursor(Qt::OpenHandCursor);
            }
        } else {
            setCursor(Qt::ArrowCursor);
        }
        return;
    }

    if (m_dragSegmentIndex < 0) return;

    double dx = pos.x() - m_dragStartPos.x();
    qint64 dtMs = static_cast<qint64>((dx / m_pixelsPerSecond) * 1000.0);

    qint64 rawStart, rawEnd;

    switch (m_dragMode) {
    case DragMode::MoveSegment: {
        rawStart = qMax(qint64(0), m_dragOrigStartMs + dtMs);
        qint64 duration = m_dragOrigEndMs - m_dragOrigStartMs;
        rawEnd = rawStart + duration;
        break;
    }
    case DragMode::ResizeLeft: {
        rawStart = qBound(qint64(0), m_dragOrigStartMs + dtMs, m_dragOrigEndMs - 100);
        rawEnd = m_dragOrigEndMs;
        break;
    }
    case DragMode::ResizeRight: {
        rawStart = m_dragOrigStartMs;
        rawEnd = qMax(m_dragOrigStartMs + 100, m_dragOrigEndMs + dtMs);
        break;
    }
    default:
        return;
    }

    // Apply snap-to-adjacent
    auto [snappedStart, snappedEnd] = snapSegmentEdges(rawStart, rawEnd, m_dragSegmentIndex, m_dragMode);
    m_snappedThisDrag = (snappedStart != rawStart || snappedEnd != rawEnd);

    m_segments[m_dragSegmentIndex].finalStartMs = snappedStart;
    m_segments[m_dragSegmentIndex].finalEndMs = snappedEnd;

    update();
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent *)
{
    if (m_dragMode != DragMode::None && m_dragMode != DragMode::Playhead && m_dragSegmentIndex >= 0) {
        const auto &seg = m_segments[m_dragSegmentIndex];
        emit segmentTimingChanged(m_dragSegmentIndex, seg.effectiveStartMs(), seg.effectiveEndMs());
    }

    m_dragMode = DragMode::None;
    m_dragSegmentIndex = -1;
    m_snappedThisDrag = false;
    setCursor(Qt::ArrowCursor);
    update();
}

void TimelineWidget::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();

    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom
        double factor = delta > 0 ? 1.2 : 0.8;
        double mouseTimeMs = pixelToTime(event->position().x());
        m_pixelsPerSecond = qBound(10.0, m_pixelsPerSecond * factor, 500.0);
        m_scrollOffset = (mouseTimeMs / 1000.0) * m_pixelsPerSecond - event->position().x();
        m_scrollOffset = qMax(0.0, m_scrollOffset);
        update();
    } else if (event->modifiers() & Qt::ShiftModifier) {
        // Shift+Wheel: frame-step the end point only
        int direction = delta > 0 ? 1 : -1;
        frameStepSelectedSegment(direction, false, true);
    } else if (m_selectedIndex >= 0) {
        // Plain wheel on selected segment: frame-step both start and end (shift entire segment)
        int direction = delta > 0 ? 1 : -1;
        frameStepSelectedSegment(direction, true, true);
    } else {
        // No selection: horizontal scroll
        m_scrollOffset -= delta * 0.5;
        m_scrollOffset = qMax(0.0, m_scrollOffset);
        update();
    }
}
