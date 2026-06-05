#include "DatabaseManager.h"
#include "db/Schema.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DatabaseManager::DatabaseManager(const QString &dbPath, QObject *parent)
    : QObject(parent), m_dbPath(dbPath)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::initialize()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qCritical() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    query.exec("PRAGMA foreign_keys = ON");

    return execSchema();
}

bool DatabaseManager::execSchema()
{
    QSqlQuery query(m_db);

    const char *statements[] = {
        Schema::CREATE_USERS_TABLE,
        Schema::CREATE_PROJECTS_TABLE,
        Schema::CREATE_SEGMENTS_TABLE,
        Schema::CREATE_EDIT_HISTORY_TABLE,
        Schema::CREATE_EDITS_TABLE,
        Schema::CREATE_CONFLICTS_TABLE
    };

    for (const char *sql : statements) {
        if (!query.exec(sql)) {
            qCritical() << "Schema error:" << query.lastError().text() << "\nSQL:" << sql;
            return false;
        }
    }

    QStringList indexes = QString(Schema::CREATE_INDEXES).split(';', Qt::SkipEmptyParts);
    for (const QString &idx : indexes) {
        QString trimmed = idx.trimmed();
        if (!trimmed.isEmpty() && !query.exec(trimmed)) {
            qCritical() << "Index error:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

int DatabaseManager::createUser(const QString &username, const QString &displayName)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO users (username, display_name) VALUES (?, ?)");
    query.addBindValue(username);
    query.addBindValue(displayName);
    query.exec();

    return getUserId(username);
}

int DatabaseManager::getUserId(const QString &username)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT user_id FROM users WHERE username = ?");
    query.addBindValue(username);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

int DatabaseManager::createProject(const QString &name, const QString &videoPath, qint64 durationMs)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO projects (name, video_path, duration_ms) VALUES (?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(videoPath);
    query.addBindValue(durationMs);
    if (query.exec()) {
        return query.lastInsertId().toInt();
    }
    return -1;
}

Project DatabaseManager::getProject(int projectId)
{
    Project p;
    QSqlQuery query(m_db);
    query.prepare("SELECT project_id, name, video_path, duration_ms, created_at, updated_at FROM projects WHERE project_id = ?");
    query.addBindValue(projectId);
    if (query.exec() && query.next()) {
        p.projectId = query.value(0).toInt();
        p.name = query.value(1).toString();
        p.videoPath = query.value(2).toString();
        p.durationMs = query.value(3).toLongLong();
        p.createdAt = query.value(4).toString();
        p.updatedAt = query.value(5).toString();
    }
    return p;
}

QVector<Project> DatabaseManager::listProjects()
{
    QVector<Project> projects;
    QSqlQuery query(m_db);
    query.exec("SELECT project_id, name, video_path, duration_ms, created_at, updated_at FROM projects ORDER BY updated_at DESC");
    while (query.next()) {
        Project p;
        p.projectId = query.value(0).toInt();
        p.name = query.value(1).toString();
        p.videoPath = query.value(2).toString();
        p.durationMs = query.value(3).toLongLong();
        p.createdAt = query.value(4).toString();
        p.updatedAt = query.value(5).toString();
        projects.append(p);
    }
    return projects;
}

void DatabaseManager::updateProjectDuration(int projectId, qint64 durationMs)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE projects SET duration_ms = ?, updated_at = datetime('now') WHERE project_id = ?");
    query.addBindValue(durationMs);
    query.addBindValue(projectId);
    query.exec();
}

int DatabaseManager::addSegment(int projectId, int index, qint64 startMs, qint64 endMs, const QString &text)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO subtitle_segments (project_id, segment_index, start_ms, end_ms, original_text) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(projectId);
    query.addBindValue(index);
    query.addBindValue(startMs);
    query.addBindValue(endMs);
    query.addBindValue(text);
    if (query.exec()) {
        return query.lastInsertId().toInt();
    }
    return -1;
}

QVector<SubtitleSegment> DatabaseManager::getSegments(int projectId)
{
    QVector<SubtitleSegment> segments;
    QSqlQuery query(m_db);
    query.prepare("SELECT segment_id, project_id, segment_index, start_ms, end_ms, original_text, "
                  "final_text, final_start_ms, final_end_ms, is_resolved "
                  "FROM subtitle_segments WHERE project_id = ? ORDER BY segment_index");
    query.addBindValue(projectId);
    if (query.exec()) {
        while (query.next()) {
            SubtitleSegment seg;
            seg.segmentId = query.value(0).toInt();
            seg.projectId = query.value(1).toInt();
            seg.segmentIndex = query.value(2).toInt();
            seg.startMs = query.value(3).toLongLong();
            seg.endMs = query.value(4).toLongLong();
            seg.originalText = query.value(5).toString();
            seg.finalText = query.value(6).toString();
            seg.finalStartMs = query.value(7).toLongLong();
            seg.finalEndMs = query.value(8).toLongLong();
            seg.isResolved = query.value(9).toBool();
            segments.append(seg);
        }
    }
    return segments;
}

SubtitleSegment DatabaseManager::getSegment(int segmentId)
{
    SubtitleSegment seg;
    QSqlQuery query(m_db);
    query.prepare("SELECT segment_id, project_id, segment_index, start_ms, end_ms, original_text, "
                  "final_text, final_start_ms, final_end_ms, is_resolved "
                  "FROM subtitle_segments WHERE segment_id = ?");
    query.addBindValue(segmentId);
    if (query.exec() && query.next()) {
        seg.segmentId = query.value(0).toInt();
        seg.projectId = query.value(1).toInt();
        seg.segmentIndex = query.value(2).toInt();
        seg.startMs = query.value(3).toLongLong();
        seg.endMs = query.value(4).toLongLong();
        seg.originalText = query.value(5).toString();
        seg.finalText = query.value(6).toString();
        seg.finalStartMs = query.value(7).toLongLong();
        seg.finalEndMs = query.value(8).toLongLong();
        seg.isResolved = query.value(9).toBool();
    }
    return seg;
}

void DatabaseManager::updateSegmentFinal(int segmentId, const QString &text, qint64 startMs, qint64 endMs)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE subtitle_segments SET final_text = ?, final_start_ms = ?, final_end_ms = ? WHERE segment_id = ?");
    query.addBindValue(text);
    query.addBindValue(startMs);
    query.addBindValue(endMs);
    query.addBindValue(segmentId);
    query.exec();
}

void DatabaseManager::markSegmentResolved(int segmentId)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE subtitle_segments SET is_resolved = 1 WHERE segment_id = ?");
    query.addBindValue(segmentId);
    query.exec();
}

void DatabaseManager::clearSegments(int projectId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM subtitle_segments WHERE project_id = ?");
    query.addBindValue(projectId);
    query.exec();
}

void DatabaseManager::saveUserEdit(int segmentId, int userId, const QString &text, qint64 startMs, qint64 endMs)
{
    // Record per-field history by comparing with previous edit
    auto existingEdits = getEditsForSegment(segmentId);
    SubtitleSegment seg = getSegment(segmentId);

    // Determine what the previous values were for this user
    QString prevText = seg.originalText;
    qint64 prevStart = seg.startMs;
    qint64 prevEnd = seg.endMs;

    for (const auto &e : existingEdits) {
        if (e.userId == userId) {
            prevText = e.editedText;
            prevStart = e.editedStartMs;
            prevEnd = e.editedEndMs;
            break;
        }
    }

    // Record field-level changes
    if (text != prevText) {
        recordFieldEdit(segmentId, userId, "text", prevText, text);
    }
    if (startMs != prevStart) {
        recordFieldEdit(segmentId, userId, "start_ms",
                       QString::number(prevStart), QString::number(startMs));
    }
    if (endMs != prevEnd) {
        recordFieldEdit(segmentId, userId, "end_ms",
                       QString::number(prevEnd), QString::number(endMs));
    }

    // Save composite snapshot
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO user_edits (segment_id, user_id, edited_text, edited_start_ms, edited_end_ms) "
                  "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(segmentId);
    query.addBindValue(userId);
    query.addBindValue(text);
    query.addBindValue(startMs);
    query.addBindValue(endMs);
    query.exec();
}

QVector<UserEdit> DatabaseManager::getEditsForSegment(int segmentId)
{
    QVector<UserEdit> edits;
    QSqlQuery query(m_db);
    query.prepare("SELECT edit_id, segment_id, user_id, edited_text, edited_start_ms, edited_end_ms, created_at "
                  "FROM user_edits WHERE segment_id = ? ORDER BY created_at");
    query.addBindValue(segmentId);
    if (query.exec()) {
        while (query.next()) {
            UserEdit e;
            e.editId = query.value(0).toInt();
            e.segmentId = query.value(1).toInt();
            e.userId = query.value(2).toInt();
            e.editedText = query.value(3).toString();
            e.editedStartMs = query.value(4).toLongLong();
            e.editedEndMs = query.value(5).toLongLong();
            e.createdAt = query.value(6).toString();
            edits.append(e);
        }
    }
    return edits;
}

int DatabaseManager::getEditCount(int segmentId)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM user_edits WHERE segment_id = ?");
    query.addBindValue(segmentId);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

void DatabaseManager::recordFieldEdit(int segmentId, int userId, const QString &fieldType,
                                      const QString &oldValue, const QString &newValue)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO edit_history (segment_id, user_id, field_type, old_value, new_value) "
                  "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(segmentId);
    query.addBindValue(userId);
    query.addBindValue(fieldType);
    query.addBindValue(oldValue);
    query.addBindValue(newValue);
    query.exec();
}

QVector<EditHistoryEntry> DatabaseManager::getFieldHistory(int segmentId, const QString &fieldType)
{
    QVector<EditHistoryEntry> entries;
    QSqlQuery query(m_db);
    query.prepare("SELECT history_id, segment_id, user_id, field_type, old_value, new_value, created_at "
                  "FROM edit_history WHERE segment_id = ? AND field_type = ? ORDER BY created_at");
    query.addBindValue(segmentId);
    query.addBindValue(fieldType);
    if (query.exec()) {
        while (query.next()) {
            EditHistoryEntry e;
            e.historyId = query.value(0).toInt();
            e.segmentId = query.value(1).toInt();
            e.userId = query.value(2).toInt();
            e.fieldType = query.value(3).toString();
            e.oldValue = query.value(4).toString();
            e.newValue = query.value(5).toString();
            e.createdAt = query.value(6).toString();
            entries.append(e);
        }
    }
    return entries;
}

QVector<EditHistoryEntry> DatabaseManager::getAllHistory(int segmentId)
{
    QVector<EditHistoryEntry> entries;
    QSqlQuery query(m_db);
    query.prepare("SELECT history_id, segment_id, user_id, field_type, old_value, new_value, created_at "
                  "FROM edit_history WHERE segment_id = ? ORDER BY created_at");
    query.addBindValue(segmentId);
    if (query.exec()) {
        while (query.next()) {
            EditHistoryEntry e;
            e.historyId = query.value(0).toInt();
            e.segmentId = query.value(1).toInt();
            e.userId = query.value(2).toInt();
            e.fieldType = query.value(3).toString();
            e.oldValue = query.value(4).toString();
            e.newValue = query.value(5).toString();
            e.createdAt = query.value(6).toString();
            entries.append(e);
        }
    }
    return entries;
}

void DatabaseManager::saveFieldConflictRecord(int segmentId, const QString &fieldType, int numVariants,
                                              const QString &winningValue, int voteCount, int totalVotes)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO conflict_records (segment_id, field_type, num_variants, "
                  "winning_value, vote_count, total_votes) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(segmentId);
    query.addBindValue(fieldType);
    query.addBindValue(numVariants);
    query.addBindValue(winningValue);
    query.addBindValue(voteCount);
    query.addBindValue(totalVotes);
    query.exec();
}
