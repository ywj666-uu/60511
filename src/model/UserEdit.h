#pragma once

#include <QString>

struct UserEdit {
    int     editId = -1;
    int     segmentId = -1;
    int     userId = -1;
    QString editedText;
    qint64  editedStartMs = 0;
    qint64  editedEndMs = 0;
    QString createdAt;
};
