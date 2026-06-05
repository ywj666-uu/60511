#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "model/SubtitleSegment.h"
#include "model/UserEdit.h"
#include "model/Project.h"

struct EditHistoryEntry {
    int historyId = -1;
    int segmentId = -1;
    int userId = -1;
    QString fieldType;  // "text", "start_ms", "end_ms"
    QString oldValue;
    QString newValue;
    QString createdAt;
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(const QString &dbPath, QObject *parent = nullptr);
    ~DatabaseManager();

    bool initialize();

    // User operations
    int createUser(const QString &username, const QString &displayName);
    int getUserId(const QString &username);

    // Project operations
    int createProject(const QString &name, const QString &videoPath, qint64 durationMs);
    Project getProject(int projectId);
    QVector<Project> listProjects();
    void updateProjectDuration(int projectId, qint64 durationMs);

    // Segment operations
    int addSegment(int projectId, int index, qint64 startMs, qint64 endMs, const QString &text);
    QVector<SubtitleSegment> getSegments(int projectId);
    SubtitleSegment getSegment(int segmentId);
    void updateSegmentFinal(int segmentId, const QString &text, qint64 startMs, qint64 endMs);
    void markSegmentResolved(int segmentId);
    void clearSegments(int projectId);

    // Edit operations (composite snapshot per user)
    void saveUserEdit(int segmentId, int userId, const QString &text, qint64 startMs, qint64 endMs);
    QVector<UserEdit> getEditsForSegment(int segmentId);
    int getEditCount(int segmentId);

    // Per-field edit history
    void recordFieldEdit(int segmentId, int userId, const QString &fieldType,
                         const QString &oldValue, const QString &newValue);
    QVector<EditHistoryEntry> getFieldHistory(int segmentId, const QString &fieldType);
    QVector<EditHistoryEntry> getAllHistory(int segmentId);

    // Per-field conflict records
    void saveFieldConflictRecord(int segmentId, const QString &fieldType, int numVariants,
                                 const QString &winningValue, int voteCount, int totalVotes);

private:
    bool execSchema();
    QSqlDatabase m_db;
    QString m_dbPath;
};
