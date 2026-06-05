#pragma once

#include <QString>

struct SubtitleSegment {
    int     segmentId = -1;
    int     projectId = -1;
    int     segmentIndex = 0;
    qint64  startMs = 0;
    qint64  endMs = 0;
    QString originalText;
    QString finalText;
    qint64  finalStartMs = 0;
    qint64  finalEndMs = 0;
    bool    isResolved = false;

    QString effectiveText() const { return finalText.isEmpty() ? originalText : finalText; }
    qint64 effectiveStartMs() const { return finalStartMs > 0 ? finalStartMs : startMs; }
    qint64 effectiveEndMs() const { return finalEndMs > 0 ? finalEndMs : endMs; }
    qint64 durationMs() const { return effectiveEndMs() - effectiveStartMs(); }
};
