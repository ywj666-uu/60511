#include "MainWindow.h"
#include "ui/VideoPlayerWidget.h"
#include "ui/TimelineWidget.h"
#include "ui/SubtitleEditorPanel.h"
#include "ui/ConflictResolutionDialog.h"
#include "ui/ExportDialog.h"
#include "app/Application.h"
#include "core/SubtitleImporter.h"
#include "core/SubtitleExporter.h"
#include "core/ConflictDetector.h"
#include "core/MajorityVoteMerger.h"
#include "db/DatabaseManager.h"
#include "model/SubtitleSegment.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Subtitle Crowd Corrector"));
    resize(1280, 800);

    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("Open Video..."), this, &MainWindow::onOpenVideo, QKeySequence::Open);
    fileMenu->addAction(tr("Import Subtitles..."), this, &MainWindow::onImportSubtitles);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Export Subtitles..."), this, &MainWindow::onExportSubtitles, QKeySequence("Ctrl+E"));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), this, &QWidget::close, QKeySequence::Quit);

    auto *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("Detect Conflicts"), this, &MainWindow::onDetectConflicts);
    toolsMenu->addAction(tr("Switch User..."), this, &MainWindow::onSwitchUser);
}

void MainWindow::setupToolBar()
{
    auto *toolbar = addToolBar(tr("Main"));
    toolbar->addAction(tr("Open Video"), this, &MainWindow::onOpenVideo);
    toolbar->addAction(tr("Import Subs"), this, &MainWindow::onImportSubtitles);
    toolbar->addAction(tr("Export"), this, &MainWindow::onExportSubtitles);
    toolbar->addSeparator();
    toolbar->addAction(tr("Detect Conflicts"), this, &MainWindow::onDetectConflicts);
}

void MainWindow::setupCentralWidget()
{
    m_videoPlayer = new VideoPlayerWidget(this);
    m_subtitleEditor = new SubtitleEditorPanel(this);
    m_timeline = new TimelineWidget(this);

    auto *topSplitter = new QSplitter(Qt::Horizontal);
    topSplitter->addWidget(m_videoPlayer);
    topSplitter->addWidget(m_subtitleEditor);
    topSplitter->setStretchFactor(0, 3);
    topSplitter->setStretchFactor(1, 2);

    auto *mainSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(m_timeline);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 1);

    setCentralWidget(mainSplitter);

    connect(m_videoPlayer, &VideoPlayerWidget::positionChanged,
            m_timeline, &TimelineWidget::setPlayheadPosition);

    connect(m_timeline, &TimelineWidget::segmentSelected,
            this, &MainWindow::onSegmentSelected);

    connect(m_timeline, &TimelineWidget::segmentTimingChanged,
            this, &MainWindow::onSegmentTimingChanged);

    connect(m_timeline, &TimelineWidget::playheadMoved,
            m_videoPlayer, &VideoPlayerWidget::seekTo);

    connect(m_subtitleEditor, &SubtitleEditorPanel::editSaved,
            this, [this](int segmentId, const QString &text, qint64 startMs, qint64 endMs) {
                auto *app = qobject_cast<Application*>(qApp);
                dbManager()->saveUserEdit(segmentId, app->currentUserId(), text, startMs, endMs);
                loadSegmentsToTimeline();
                statusBar()->showMessage(tr("Edit saved"), 3000);
            });
}

void MainWindow::setupStatusBar()
{
    auto *app = qobject_cast<Application*>(qApp);
    statusBar()->showMessage(tr("User: %1 | No project loaded").arg(app->currentUserName()));
}

void MainWindow::onOpenVideo()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Video"),
        QString(), tr("Video Files (*.mp4 *.mkv *.avi *.mov *.wmv *.flv);;All Files (*)"));
    if (filePath.isEmpty()) return;

    m_videoPlayer->loadVideo(filePath);

    QString name = QFileInfo(filePath).baseName();
    m_currentProjectId = dbManager()->createProject(name, filePath, 0);

    connect(m_videoPlayer, &VideoPlayerWidget::durationChanged,
            this, [this](qint64 dur) {
                dbManager()->updateProjectDuration(m_currentProjectId, dur);
                m_timeline->setDuration(dur);
            }, Qt::UniqueConnection);

    statusBar()->showMessage(tr("Video loaded: %1").arg(name));
}

void MainWindow::onImportSubtitles()
{
    if (m_currentProjectId < 0) {
        QMessageBox::warning(this, tr("No Project"), tr("Please open a video file first."));
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Subtitles"),
        QString(), tr("Subtitle Files (*.srt *.ass *.ssa);;All Files (*)"));
    if (filePath.isEmpty()) return;

    auto segments = SubtitleImporter::importFile(filePath);
    if (segments.isEmpty()) {
        QMessageBox::warning(this, tr("Import Failed"), tr("No subtitles found in the file."));
        return;
    }

    dbManager()->clearSegments(m_currentProjectId);
    for (const auto &seg : segments) {
        dbManager()->addSegment(m_currentProjectId, seg.segmentIndex,
                               seg.startMs, seg.endMs, seg.originalText);
    }

    loadSegmentsToTimeline();
    statusBar()->showMessage(tr("Imported %1 subtitle segments").arg(segments.size()), 5000);
}

void MainWindow::onExportSubtitles()
{
    if (m_currentProjectId < 0 || m_segments.isEmpty()) {
        QMessageBox::warning(this, tr("Nothing to Export"), tr("No subtitles loaded."));
        return;
    }

    ExportDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    if (dlg.resolveConflictsBeforeExport()) {
        MajorityVoteMerger merger(dbManager());
        merger.resolveAll(m_currentProjectId);
        loadSegmentsToTimeline();
    }

    m_segments = dbManager()->getSegments(m_currentProjectId);

    SubtitleExporter::Format fmt = (dlg.selectedFormat() == ExportDialog::SRT)
        ? SubtitleExporter::SRT : SubtitleExporter::ASS;

    if (SubtitleExporter::exportToFile(m_segments, dlg.outputPath(), fmt)) {
        QMessageBox::information(this, tr("Export Complete"),
            tr("Subtitles exported to:\n%1").arg(dlg.outputPath()));
    } else {
        QMessageBox::critical(this, tr("Export Failed"), tr("Could not write to file."));
    }
}

void MainWindow::onDetectConflicts()
{
    if (m_currentProjectId < 0) return;

    ConflictDetector detector(dbManager());
    auto conflicts = detector.detectConflicts(m_currentProjectId);

    if (conflicts.isEmpty()) {
        QMessageBox::information(this, tr("No Conflicts"), tr("All segments are consistent."));
        return;
    }

    ConflictResolutionDialog dlg(conflicts, dbManager(), this);
    if (dlg.exec() == QDialog::Accepted) {
        MajorityVoteMerger merger(dbManager());
        merger.resolveAll(m_currentProjectId);
        loadSegmentsToTimeline();
        statusBar()->showMessage(tr("Conflicts resolved by majority vote"), 5000);
    }
}

void MainWindow::onSegmentSelected(int segmentIndex)
{
    if (segmentIndex >= 0 && segmentIndex < m_segments.size()) {
        m_subtitleEditor->setSegment(m_segments[segmentIndex]);
        qint64 seekPos = m_segments[segmentIndex].effectiveStartMs();
        m_videoPlayer->seekTo(seekPos);
    }
}

void MainWindow::onSegmentTimingChanged(int segmentIndex, qint64 newStartMs, qint64 newEndMs)
{
    if (segmentIndex < 0 || segmentIndex >= m_segments.size()) return;

    auto &seg = m_segments[segmentIndex];
    auto *app = qobject_cast<Application*>(qApp);
    dbManager()->saveUserEdit(seg.segmentId, app->currentUserId(),
                             seg.effectiveText(), newStartMs, newEndMs);

    seg.finalStartMs = newStartMs;
    seg.finalEndMs = newEndMs;
    m_subtitleEditor->setSegment(seg);
}

void MainWindow::onVideoPositionChanged(qint64 positionMs)
{
    m_timeline->setPlayheadPosition(positionMs);
}

void MainWindow::onSwitchUser()
{
    bool ok;
    QString username = QInputDialog::getText(this, tr("Switch User"),
        tr("Enter username:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !username.isEmpty()) {
        auto *app = qobject_cast<Application*>(qApp);
        app->setCurrentUser(username, username);
        statusBar()->showMessage(tr("Switched to user: %1").arg(username));
    }
}

void MainWindow::loadSegmentsToTimeline()
{
    m_segments = dbManager()->getSegments(m_currentProjectId);
    m_timeline->setSegments(m_segments);
}

DatabaseManager* MainWindow::dbManager()
{
    auto *app = qobject_cast<Application*>(qApp);
    return app->databaseManager();
}
