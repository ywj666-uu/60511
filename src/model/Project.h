#pragma once

#include <QString>

struct Project {
    int     projectId = -1;
    QString name;
    QString videoPath;
    qint64  durationMs = 0;
    QString createdAt;
    QString updatedAt;
};
