#include "SubtitleExporter.h"
#include "util/TimeUtil.h"

#include <QFile>
#include <QTextStream>

bool SubtitleExporter::exportToFile(const QVector<SubtitleSegment> &segments,
                                    const QString &outputPath, Format format)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    if (format == SRT) {
        stream << generateSRT(segments);
    } else {
        stream << generateASS(segments);
    }

    return true;
}

QString SubtitleExporter::generateSRT(const QVector<SubtitleSegment> &segments)
{
    QString output;
    for (int i = 0; i < segments.size(); ++i) {
        const auto &seg = segments[i];
        output += QString::number(i + 1) + "\n";
        output += TimeUtil::msToSRTTime(seg.effectiveStartMs())
                + " --> "
                + TimeUtil::msToSRTTime(seg.effectiveEndMs()) + "\n";
        output += seg.effectiveText() + "\n";
        output += "\n";
    }
    return output;
}

QString SubtitleExporter::generateASS(const QVector<SubtitleSegment> &segments, const QString &title)
{
    QString output;
    output += "[Script Info]\n";
    output += "Title: " + (title.isEmpty() ? "Exported Subtitles" : title) + "\n";
    output += "ScriptType: v4.00+\n";
    output += "WrapStyle: 0\n";
    output += "PlayResX: 1920\n";
    output += "PlayResY: 1080\n";
    output += "\n";

    output += "[V4+ Styles]\n";
    output += "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, "
              "OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, "
              "ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, "
              "Alignment, MarginL, MarginR, MarginV, Encoding\n";
    output += "Style: Default,Arial,48,&H00FFFFFF,&H000000FF,"
              "&H00000000,&H80000000,-1,0,0,0,"
              "100,100,0,0,1,2,1,"
              "2,10,10,10,1\n";
    output += "\n";

    output += "[Events]\n";
    output += "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n";

    for (const auto &seg : segments) {
        QString start = TimeUtil::msToASSTime(seg.effectiveStartMs());
        QString end = TimeUtil::msToASSTime(seg.effectiveEndMs());
        QString text = seg.effectiveText();
        text.replace("\n", "\\N");
        output += QString("Dialogue: 0,%1,%2,Default,,0,0,0,,%3\n").arg(start, end, text);
    }

    return output;
}
