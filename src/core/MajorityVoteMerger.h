#pragma once

#include <QVector>
#include <QString>
#include <QMap>
#include "model/UserEdit.h"

class DatabaseManager;

class MajorityVoteMerger
{
public:
    struct FieldResult {
        QString fieldType;      // "text", "start_ms", "end_ms"
        QString winningValue;
        int voteCount;
        int totalVotes;
    };

    struct MergeResult {
        int     segmentId;
        QString winningText;
        qint64  winningStartMs;
        qint64  winningEndMs;
        QVector<FieldResult> fieldResults;
    };

    explicit MajorityVoteMerger(DatabaseManager *db);

    MergeResult resolveSegment(int segmentId);
    QVector<MergeResult> resolveAll(int projectId);

private:
    FieldResult voteOnTextField(const QVector<UserEdit> &edits);
    FieldResult voteOnTimingField(const QVector<UserEdit> &edits, const QString &fieldType);
    QString pickByMajority(const QMap<QString, int> &counts, const QMap<QString, QString> &earliestTimestamps);

    DatabaseManager *m_db;
};
