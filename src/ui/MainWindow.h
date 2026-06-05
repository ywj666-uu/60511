#pragma once

#include <QMainWindow>

class VideoPlayerWidget;
class TimelineWidget;
class SubtitleEditorPanel;
class DatabaseManager;
class Project;
struct SubtitleSegment;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onOpenVideo();
    void onImportSubtitles();
    void onExportSubtitles();
    void onDetectConflicts();
    void onSegmentSelected(int segmentIndex);
    void onSegmentTimingChanged(int segmentIndex, qint64 newStartMs, qint64 newEndMs);
    void onVideoPositionChanged(qint64 positionMs);
    void onSwitchUser();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupStatusBar();
    void loadSegmentsToTimeline();
    DatabaseManager* dbManager();

    VideoPlayerWidget   *m_videoPlayer = nullptr;
    TimelineWidget      *m_timeline = nullptr;
    SubtitleEditorPanel *m_subtitleEditor = nullptr;

    int m_currentProjectId = -1;
    QVector<SubtitleSegment> m_segments;
};
