#pragma once

#include <QVector>
#include <QString>
#include <QMap>
#include "model/UserEdit.h"

class DatabaseManager;

class ConflictDetector
{
public:
    struct FieldConflict {
        int segmentId;
        QString fieldType;      // "text", "start_ms", "end_ms"
        int numVariants;
        QMap<QString, QVector<int>> valueToUsers;  // value -> list of user_ids who chose it
    };

    struct Conflict {
        int segmentId;
        QString conflictType;   // "text", "timing", "both" (for display)
        QVector<UserEdit> edits;
        int numVariants;
        QVector<FieldConflict> fieldConflicts;  // per-field breakdown
    };

    explicit ConflictDetector(DatabaseManager *db);

    QVector<Conflict> detectConflicts(int projectId);
    bool hasConflict(int segmentId);

private:
    FieldConflict detectFieldConflict(int segmentId, const QString &fieldType,
                                      const QVector<UserEdit> &edits);

    DatabaseManager *m_db;
};
