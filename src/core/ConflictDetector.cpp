#include "ConflictDetector.h"
#include "db/DatabaseManager.h"

#include <QMap>
#include <QtMath>
#include <algorithm>

ConflictDetector::ConflictDetector(DatabaseManager *db)
    : m_db(db)
{
}

QVector<ConflictDetector::Conflict> ConflictDetector::detectConflicts(int projectId)
{
    QVector<Conflict> conflicts;
    auto segments = m_db->getSegments(projectId);

    // Sort segments by timeline position for sequential processing
    std::sort(segments.begin(), segments.end(), [](const SubtitleSegment &a, const SubtitleSegment &b) {
        return a.effectiveStartMs() < b.effectiveStartMs();
    });

    for (const auto &seg : segments) {
        auto edits = m_db->getEditsForSegment(seg.segmentId);
        if (edits.size() < 2) continue;

        // Detect per-field conflicts independently
        FieldConflict textConflict = detectFieldConflict(seg.segmentId, "text", edits);
        FieldConflict startConflict = detectFieldConflict(seg.segmentId, "start_ms", edits);
        FieldConflict endConflict = detectFieldConflict(seg.segmentId, "end_ms", edits);

        bool hasText = textConflict.numVariants > 1;
        bool hasTiming = startConflict.numVariants > 1 || endConflict.numVariants > 1;

        if (hasText || hasTiming) {
            Conflict c;
            c.segmentId = seg.segmentId;
            if (hasText && hasTiming) c.conflictType = "both";
            else if (hasText) c.conflictType = "text";
            else c.conflictType = "timing";
            c.edits = edits;
            c.numVariants = qMax({textConflict.numVariants, startConflict.numVariants, endConflict.numVariants});

            if (hasText) c.fieldConflicts.append(textConflict);
            if (startConflict.numVariants > 1) c.fieldConflicts.append(startConflict);
            if (endConflict.numVariants > 1) c.fieldConflicts.append(endConflict);

            conflicts.append(c);
        }
    }

    return conflicts;
}

bool ConflictDetector::hasConflict(int segmentId)
{
    auto edits = m_db->getEditsForSegment(segmentId);
    if (edits.size() < 2) return false;

    auto textC = detectFieldConflict(segmentId, "text", edits);
    auto startC = detectFieldConflict(segmentId, "start_ms", edits);
    auto endC = detectFieldConflict(segmentId, "end_ms", edits);

    return textC.numVariants > 1 || startC.numVariants > 1 || endC.numVariants > 1;
}

ConflictDetector::FieldConflict ConflictDetector::detectFieldConflict(
    int segmentId, const QString &fieldType, const QVector<UserEdit> &edits)
{
    FieldConflict fc;
    fc.segmentId = segmentId;
    fc.fieldType = fieldType;

    QMap<QString, QVector<int>> groups;

    for (const auto &e : edits) {
        QString value;
        if (fieldType == "text") {
            value = e.editedText.trimmed();
        } else if (fieldType == "start_ms") {
            value = QString::number(e.editedStartMs);
        } else if (fieldType == "end_ms") {
            value = QString::number(e.editedEndMs);
        }
        groups[value].append(e.userId);
    }

    // For timing fields, cluster values within 500ms tolerance
    if (fieldType == "start_ms" || fieldType == "end_ms") {
        QMap<QString, QVector<int>> clustered;
        QVector<QPair<qint64, QVector<int>>> sorted;

        for (auto it = groups.begin(); it != groups.end(); ++it) {
            sorted.append({it.key().toLongLong(), it.value()});
        }
        std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
            return a.first < b.first;
        });

        constexpr qint64 TOLERANCE_MS = 500;
        QVector<QPair<qint64, QVector<int>>> clusters;

        for (const auto &item : sorted) {
            bool placed = false;
            for (auto &cluster : clusters) {
                if (qAbs(item.first - cluster.first) <= TOLERANCE_MS) {
                    // Merge into this cluster, update centroid as average
                    int totalUsers = cluster.second.size() + item.second.size();
                    cluster.first = (cluster.first * cluster.second.size() + item.first * item.second.size()) / totalUsers;
                    cluster.second.append(item.second);
                    placed = true;
                    break;
                }
            }
            if (!placed) {
                clusters.append(item);
            }
        }

        for (const auto &cluster : clusters) {
            clustered[QString::number(cluster.first)] = cluster.second;
        }
        groups = clustered;
    }

    fc.valueToUsers = groups;
    fc.numVariants = groups.size();
    return fc;
}
