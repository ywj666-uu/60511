#pragma once

#include <QString>
#include <QVector>
#include "model/SubtitleSegment.h"

class SubtitleExporter
{
public:
    enum Format { SRT, ASS };

    static bool exportToFile(const QVector<SubtitleSegment> &segments,
                             const QString &outputPath, Format format);
    static QString generateSRT(const QVector<SubtitleSegment> &segments);
    static QString generateASS(const QVector<SubtitleSegment> &segments, const QString &title = QString());
};
