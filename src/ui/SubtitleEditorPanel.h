#pragma once

#include <QWidget>
#include "model/SubtitleSegment.h"

class QTextEdit;
class QTimeEdit;
class QPushButton;
class QLabel;

class SubtitleEditorPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SubtitleEditorPanel(QWidget *parent = nullptr);

    void setSegment(const SubtitleSegment &segment);
    void clear();

signals:
    void editSaved(int segmentId, const QString &text, qint64 startMs, qint64 endMs);

private slots:
    void onSaveClicked();
    void onRevertClicked();

private:
    QTime msToQTime(qint64 ms) const;
    qint64 qTimeToMs(const QTime &t) const;

    QLabel      *m_segmentInfoLabel;
    QTimeEdit   *m_startTimeEdit;
    QTimeEdit   *m_endTimeEdit;
    QTextEdit   *m_textEdit;
    QPushButton *m_saveButton;
    QPushButton *m_revertButton;

    SubtitleSegment m_currentSegment;
};
