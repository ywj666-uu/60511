#pragma once

#include <QString>

namespace Schema {

const char* const CREATE_USERS_TABLE = R"SQL(
CREATE TABLE IF NOT EXISTS users (
    user_id       INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE,
    display_name  TEXT NOT NULL,
    created_at    TEXT NOT NULL DEFAULT (datetime('now'))
)
)SQL";

const char* const CREATE_PROJECTS_TABLE = R"SQL(
CREATE TABLE IF NOT EXISTS projects (
    project_id    INTEGER PRIMARY KEY AUTOINCREMENT,
    name          TEXT NOT NULL,
    video_path    TEXT NOT NULL,
    duration_ms   INTEGER NOT NULL DEFAULT 0,
    created_at    TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at    TEXT NOT NULL DEFAULT (datetime('now'))
)
)SQL";

const char* const CREATE_SEGMENTS_TABLE = R"SQL(
CREATE TABLE IF NOT EXISTS subtitle_segments (
    segment_id    INTEGER PRIMARY KEY AUTOINCREMENT,
    project_id    INTEGER NOT NULL REFERENCES projects(project_id) ON DELETE CASCADE,
    segment_index INTEGER NOT NULL,
    start_ms      INTEGER NOT NULL,
    end_ms        INTEGER NOT NULL,
    original_text TEXT NOT NULL,
    final_text    TEXT,
    final_start_ms INTEGER,
    final_end_ms   INTEGER,
    is_resolved   INTEGER NOT NULL DEFAULT 0,
    UNIQUE(project_id, segment_index)
)
)SQL";

// Per-field edit history: each row records one user's change to one field of a segment
const char* const CREATE_EDIT_HISTORY_TABLE = R"SQL(
CREATE TABLE IF NOT EXISTS edit_history (
    history_id    INTEGER PRIMARY KEY AUTOINCREMENT,
    segment_id    INTEGER NOT NULL REFERENCES subtitle_segments(segment_id) ON DELETE CASCADE,
    user_id       INTEGER NOT NULL REFERENCES users(user_id),
    field_type    TEXT NOT NULL CHECK(field_type IN ('text', 'start_ms', 'end_ms')),
    old_value     TEXT,
    new_value     TEXT NOT NULL,
    created_at    TEXT NOT NULL DEFAULT (datetime('now'))
)
)SQL";

// Composite user edits (the latest snapshot per user per segment)
const char* const CREATE_EDITS_TABLE = R"SQL(
CREATE TABLE IF NOT EXISTS user_edits (
    edit_id       INTEGER PRIMARY KEY AUTOINCREMENT,
    segment_id    INTEGER NOT NULL REFERENCES subtitle_segments(segment_id) ON DELETE CASCADE,
    user_id       INTEGER NOT NULL REFERENCES users(user_id),
    edited_text   TEXT NOT NULL,
    edited_start_ms INTEGER NOT NULL,
    edited_end_ms   INTEGER NOT NULL,
    created_at    TEXT NOT NULL DEFAULT (datetime('now')),
    UNIQUE(segment_id, user_id)
)
)SQL";

// Per-field conflict resolution records
const char* const CREATE_CONFLICTS_TABLE = R"SQL(
CREATE TABLE IF NOT EXISTS conflict_records (
    conflict_id   INTEGER PRIMARY KEY AUTOINCREMENT,
    segment_id    INTEGER NOT NULL REFERENCES subtitle_segments(segment_id) ON DELETE CASCADE,
    field_type    TEXT NOT NULL CHECK(field_type IN ('text', 'start_ms', 'end_ms')),
    num_variants  INTEGER NOT NULL,
    winning_value TEXT NOT NULL,
    vote_count    INTEGER NOT NULL DEFAULT 0,
    total_votes   INTEGER NOT NULL DEFAULT 0,
    resolved_at   TEXT NOT NULL DEFAULT (datetime('now'))
)
)SQL";

const char* const CREATE_INDEXES = R"SQL(
CREATE INDEX IF NOT EXISTS idx_segments_project ON subtitle_segments(project_id, segment_index);
CREATE INDEX IF NOT EXISTS idx_edits_segment ON user_edits(segment_id);
CREATE INDEX IF NOT EXISTS idx_edits_user ON user_edits(user_id);
CREATE INDEX IF NOT EXISTS idx_history_segment ON edit_history(segment_id);
CREATE INDEX IF NOT EXISTS idx_history_field ON edit_history(segment_id, field_type)
)SQL";

} // namespace Schema
