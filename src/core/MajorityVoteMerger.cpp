#include "MajorityVoteMerger.h"
#include "db/DatabaseManager.h"

#include <QMap>
#include <QtMath>
#include <algorithm>

MajorityVoteMerger::MajorityVoteMerger(DatabaseManager *db)
    : m_db(db)
{
}

MajorityVoteMerger::MergeResult MajorityVoteMerger::resolveSegment(int segmentId)
{
    MergeResult result;
    result.segmentId = segmentId;

    auto edits = m_db->getEditsForSegment(segmentId);
    if (edits.isEmpty()) {
        auto seg = m_db->getSegment(segmentId);
        result.winningText = seg.originalText;
        result.winningStartMs = seg.startMs;
        result.winningEndMs = seg.endMs;
        return result;
    }

    if (edits.size() == 1) {
        result.winningText = edits[0].editedText;
        result.winningStartMs = edits[0].editedStartMs;
        result.winningEndMs = edits[0].editedEndMs;
        FieldResult tr;
        tr.fieldType = "text"; tr.winningValue = edits[0].editedText;
        tr.voteCount = 1; tr.totalVotes = 1;
        result.fieldResults.append(tr);
    } else {
        // Sort edits by the segment's timeline position (start_ms) for consistent ordering
        std::sort(edits.begin(), edits.end(), [](const UserEdit &a, const UserEdit &b) {
            return a.editedStartMs < b.editedStartMs;
        });

        // Vote on each field independently
        FieldResult textResult = voteOnTextField(edits);
        FieldResult startResult = voteOnTimingField(edits, "start_ms");
        FieldResult endResult = voteOnTimingField(edits, "end_ms");

        result.winningText = textResult.winningValue;
        result.winningStartMs = startResult.winningValue.toLongLong();
        result.winningEndMs = endResult.winningValue.toLongLong();
        result.fieldResults = {textResult, startResult, endResult};

        // Record per-field conflict resolution
        if (textResult.voteCount < textResult.totalVotes) {
            m_db->saveFieldConflictRecord(segmentId, "text", textResult.totalVotes,
                                         textResult.winningValue, textResult.voteCount, textResult.totalVotes);
        }
        if (startResult.voteCount < startResult.totalVotes) {
            m_db->saveFieldConflictRecord(segmentId, "start_ms", startResult.totalVotes,
                                         startResult.winningValue, startResult.voteCount, startResult.totalVotes);
        }
        if (endResult.voteCount < endResult.totalVotes) {
            m_db->saveFieldConflictRecord(segmentId, "end_ms", endResult.totalVotes,
                                         endResult.winningValue, endResult.voteCount, endResult.totalVotes);
        }
    }

    m_db->updateSegmentFinal(segmentId, result.winningText, result.winningStartMs, result.winningEndMs);
    m_db->markSegmentResolved(segmentId);

    return result;
}

QVector<MajorityVoteMerger::MergeResult> MajorityVoteMerger::resolveAll(int projectId)
{
    QVector<MergeResult> results;
    auto segments = m_db->getSegments(projectId);

    // Sort by timeline position before resolving
    std::sort(segments.begin(), segments.end(), [](const SubtitleSegment &a, const SubtitleSegment &b) {
        return a.effectiveStartMs() < b.effectiveStartMs();
    });

    for (const auto &seg : segments) {
        auto edits = m_db->getEditsForSegment(seg.segmentId);
        if (edits.size() >= 2) {
            results.append(resolveSegment(seg.segmentId));
        } else if (edits.size() == 1) {
            m_db->updateSegmentFinal(seg.segmentId, edits[0].editedText,
                                    edits[0].editedStartMs, edits[0].editedEndMs);
            m_db->markSegmentResolved(seg.segmentId);
        }
    }

    return results;
}

MajorityVoteMerger::FieldResult MajorityVoteMerger::voteOnTextField(const QVector<UserEdit> &edits)
{
    FieldResult result;
    result.fieldType = "text";
    result.totalVotes = edits.size();

    // Group by normalized text, track original case and earliest timestamp
    QMap<QString, int> counts;
    QMap<QString, QString> originalCase;
    QMap<QString, QString> earliestTime;

    for (const auto &e : edits) {
        QString normalized = e.editedText.trimmed().toLower();
        counts[normalized]++;
        if (!originalCase.contains(normalized)) {
            originalCase[normalized] = e.editedText;
            earliestTime[normalized] = e.createdAt;
        } else if (e.createdAt < earliestTime[normalized]) {
            earliestTime[normalized] = e.createdAt;
            originalCase[normalized] = e.editedText;
        }
    }

    QString winner = pickByMajority(counts, earliestTime);
    result.winningValue = originalCase.value(winner, winner);
    result.voteCount = counts.value(winner, 0);

    return result;
}

MajorityVoteMerger::FieldResult MajorityVoteMerger::voteOnTimingField(const QVector<UserEdit> &edits, const QString &fieldType)
{
    FieldResult result;
    result.fieldType = fieldType;
    result.totalVotes = edits.size();

    constexpr qint64 TOLERANCE_MS = 500;

    // Extract values
    QVector<QPair<qint64, QString>> values;  // value, timestamp
    for (const auto &e : edits) {
        qint64 val = (fieldType == "start_ms") ? e.editedStartMs : e.editedEndMs;
        values.append({val, e.createdAt});
    }

    // Sort by value for clustering
    std::sort(values.begin(), values.end(), [](const auto &a, const auto &b) {
        return a.first < b.first;
    });

    // Cluster within tolerance
    struct Cluster {
        qint64 centroid;
        int count;
        QString earliestTime;
    };
    QVector<Cluster> clusters;

    for (const auto &v : values) {
        bool placed = false;
        for (auto &c : clusters) {
            if (qAbs(v.first - c.centroid) <= TOLERANCE_MS) {
                // Update centroid as weighted average
                c.centroid = (c.centroid * c.count + v.first) / (c.count + 1);
                c.count++;
                if (v.second < c.earliestTime) c.earliestTime = v.second;
                placed = true;
                break;
            }
        }
        if (!placed) {
            clusters.append({v.first, 1, v.second});
        }
    }

    // Find largest cluster; tie-break by earliest timestamp
    std::sort(clusters.begin(), clusters.end(), [](const Cluster &a, const Cluster &b) {
        if (a.count != b.count) return a.count > b.count;
        return a.earliestTime < b.earliestTime;
    });

    if (!clusters.isEmpty()) {
        result.winningValue = QString::number(clusters[0].centroid);
        result.voteCount = clusters[0].count;
    }

    return result;
}

QString MajorityVoteMerger::pickByMajority(const QMap<QString, int> &counts, const QMap<QString, QString> &earliestTimestamps)
{
    int maxCount = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        maxCount = qMax(maxCount, it.value());
    }

    QVector<QString> winners;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        if (it.value() == maxCount) {
            winners.append(it.key());
        }
    }

    if (winners.size() == 1) return winners[0];

    // Tie-break: earliest submission
    QString best = winners[0];
    for (const auto &w : winners) {
        if (earliestTimestamps.value(w) < earliestTimestamps.value(best)) {
            best = w;
        }
    }
    return best;
}
