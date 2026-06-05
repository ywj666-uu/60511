#pragma once

#include <QString>
#include <QVector>
#include "model/SubtitleSegment.h"

class SubtitleImporter
{
public:
    static QVector<SubtitleSegment> importSRT(const QString &filePath);
    static QVector<SubtitleSegment> importASS(const QString &filePath);
    static QVector<SubtitleSegment> importFile(const QString &filePath);
};
