#include "SubtitleImporter.h"
#include "util/TimeUtil.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QRegularExpression>

QVector<SubtitleSegment> SubtitleImporter::importSRT(const QString &filePath)
{
    QVector<SubtitleSegment> segments;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return segments;

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    int index = 0;
    enum State { ExpectIndex, ExpectTime, ExpectText };
    State state = ExpectIndex;
    SubtitleSegment current;
    QStringList textLines;

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        switch (state) {
        case ExpectIndex:
            if (!line.trimmed().isEmpty()) {
                state = ExpectTime;
            }
            break;

        case ExpectTime: {
            static QRegularExpression timeRe(
                R"((\d{2}:\d{2}:\d{2},\d{3})\s*-->\s*(\d{2}:\d{2}:\d{2},\d{3}))");
            auto match = timeRe.match(line);
            if (match.hasMatch()) {
                current.startMs = TimeUtil::srtTimeToMs(match.captured(1));
                current.endMs = TimeUtil::srtTimeToMs(match.captured(2));
                state = ExpectText;
                textLines.clear();
            }
            break;
        }

        case ExpectText:
            if (line.trimmed().isEmpty()) {
                current.segmentIndex = index++;
                current.originalText = textLines.join("\n");
                segments.append(current);
                current = SubtitleSegment();
                state = ExpectIndex;
            } else {
                textLines.append(line);
            }
            break;
        }
    }

    if (state == ExpectText && !textLines.isEmpty()) {
        current.segmentIndex = index;
        current.originalText = textLines.join("\n");
        segments.append(current);
    }

    return segments;
}

QVector<SubtitleSegment> SubtitleImporter::importASS(const QString &filePath)
{
    QVector<SubtitleSegment> segments;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return segments;

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    bool inEvents = false;
    int index = 0;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (line.startsWith("[Events]")) {
            inEvents = true;
            continue;
        }
        if (line.startsWith("[") && line.endsWith("]")) {
            inEvents = false;
            continue;
        }

        if (inEvents && line.startsWith("Dialogue:")) {
            QString content = line.mid(9).trimmed();
            QStringList fields = content.split(',');
            if (fields.size() >= 10) {
                SubtitleSegment seg;
                seg.segmentIndex = index++;
                seg.startMs = TimeUtil::assTimeToMs(fields[1].trimmed());
                seg.endMs = TimeUtil::assTimeToMs(fields[2].trimmed());
                // Text is everything after the 9th comma
                QStringList textParts = fields.mid(9);
                QString text = textParts.join(',');
                text.replace("\\N", "\n");
                text.replace("\\n", "\n");
                // Remove ASS style overrides like {\b1}
                static QRegularExpression styleRe(R"(\{[^}]*\})");
                text.remove(styleRe);
                seg.originalText = text.trimmed();
                segments.append(seg);
            }
        }
    }

    return segments;
}

QVector<SubtitleSegment> SubtitleImporter::importFile(const QString &filePath)
{
    QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == "srt") {
        return importSRT(filePath);
    } else if (suffix == "ass" || suffix == "ssa") {
        return importASS(filePath);
    }
    return {};
}
