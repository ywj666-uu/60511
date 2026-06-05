#include "TimeUtil.h"
#include <QRegularExpression>

namespace TimeUtil {

QString msToDisplay(qint64 ms)
{
    int h = static_cast<int>(ms / 3600000);
    int m = static_cast<int>((ms % 3600000) / 60000);
    int s = static_cast<int>((ms % 60000) / 1000);
    int millis = static_cast<int>(ms % 1000);
    return QString("%1:%2:%3.%4")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'))
        .arg(millis, 3, 10, QChar('0'));
}

QString msToSRTTime(qint64 ms)
{
    int h = static_cast<int>(ms / 3600000);
    int m = static_cast<int>((ms % 3600000) / 60000);
    int s = static_cast<int>((ms % 60000) / 1000);
    int millis = static_cast<int>(ms % 1000);
    return QString("%1:%2:%3,%4")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'))
        .arg(millis, 3, 10, QChar('0'));
}

QString msToASSTime(qint64 ms)
{
    int h = static_cast<int>(ms / 3600000);
    int m = static_cast<int>((ms % 3600000) / 60000);
    int s = static_cast<int>((ms % 60000) / 1000);
    int cs = static_cast<int>((ms % 1000) / 10);
    return QString("%1:%2:%3.%4")
        .arg(h)
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'))
        .arg(cs, 2, 10, QChar('0'));
}

qint64 srtTimeToMs(const QString &t)
{
    // Format: 00:01:23,456
    QStringList parts = t.split(QRegularExpression("[:,]"));
    if (parts.size() != 4) return 0;
    qint64 h = parts[0].toLongLong();
    qint64 m = parts[1].toLongLong();
    qint64 s = parts[2].toLongLong();
    qint64 ms = parts[3].toLongLong();
    return h * 3600000 + m * 60000 + s * 1000 + ms;
}

qint64 assTimeToMs(const QString &t)
{
    // Format: 0:01:23.46
    QStringList parts = t.split(QRegularExpression("[:\\.]"));
    if (parts.size() != 4) return 0;
    qint64 h = parts[0].toLongLong();
    qint64 m = parts[1].toLongLong();
    qint64 s = parts[2].toLongLong();
    qint64 cs = parts[3].toLongLong();
    return h * 3600000 + m * 60000 + s * 1000 + cs * 10;
}

} // namespace TimeUtil
