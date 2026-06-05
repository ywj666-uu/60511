#pragma once

#include <QString>

namespace TimeUtil {

QString msToDisplay(qint64 ms);
QString msToSRTTime(qint64 ms);
QString msToASSTime(qint64 ms);
qint64 srtTimeToMs(const QString &t);
qint64 assTimeToMs(const QString &t);

} // namespace TimeUtil
