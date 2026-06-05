#include "SubtitleEditorPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QTextEdit>
#include <QTimeEdit>
#include <QPushButton>
#include <QGroupBox>

SubtitleEditorPanel::SubtitleEditorPanel(QWidget *parent)
    : QWidget(parent)
{
    m_segmentInfoLabel = new QLabel(tr("No segment selected"), this);
    m_segmentInfoLabel->setStyleSheet("font-weight: bold; font-size: 12px;");

    m_startTimeEdit = new QTimeEdit(this);
    m_startTimeEdit->setDisplayFormat("HH:mm:ss.zzz");
    m_startTimeEdit->setEnabled(false);

    m_endTimeEdit = new QTimeEdit(this);
    m_endTimeEdit->setDisplayFormat("HH:mm:ss.zzz");
    m_endTimeEdit->setEnabled(false);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setPlaceholderText(tr("Select a subtitle segment to edit..."));
    m_textEdit->setEnabled(false);

    m_saveButton = new QPushButton(tr("Save Edit"), this);
    m_saveButton->setEnabled(false);
    m_revertButton = new QPushButton(tr("Revert"), this);
    m_revertButton->setEnabled(false);

    auto *timeLayout = new QFormLayout;
    timeLayout->addRow(tr("Start:"), m_startTimeEdit);
    timeLayout->addRow(tr("End:"), m_endTimeEdit);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_revertButton);
    buttonLayout->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_segmentInfoLabel);
    layout->addLayout(timeLayout);
    layout->addWidget(new QLabel(tr("Text:"), this));
    layout->addWidget(m_textEdit, 1);
    layout->addLayout(buttonLayout);

    connect(m_saveButton, &QPushButton::clicked, this, &SubtitleEditorPanel::onSaveClicked);
    connect(m_revertButton, &QPushButton::clicked, this, &SubtitleEditorPanel::onRevertClicked);
}

void SubtitleEditorPanel::setSegment(const SubtitleSegment &segment)
{
    m_currentSegment = segment;

    m_segmentInfoLabel->setText(tr("Segment #%1").arg(segment.segmentIndex + 1));
    m_startTimeEdit->setTime(msToQTime(segment.effectiveStartMs()));
    m_endTimeEdit->setTime(msToQTime(segment.effectiveEndMs()));
    m_textEdit->setPlainText(segment.effectiveText());

    m_startTimeEdit->setEnabled(true);
    m_endTimeEdit->setEnabled(true);
    m_textEdit->setEnabled(true);
    m_saveButton->setEnabled(true);
    m_revertButton->setEnabled(true);
}

void SubtitleEditorPanel::clear()
{
    m_currentSegment = SubtitleSegment();
    m_segmentInfoLabel->setText(tr("No segment selected"));
    m_startTimeEdit->setTime(QTime(0, 0));
    m_endTimeEdit->setTime(QTime(0, 0));
    m_textEdit->clear();
    m_startTimeEdit->setEnabled(false);
    m_endTimeEdit->setEnabled(false);
    m_textEdit->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_revertButton->setEnabled(false);
}

void SubtitleEditorPanel::onSaveClicked()
{
    if (m_currentSegment.segmentId < 0) return;

    QString text = m_textEdit->toPlainText();
    qint64 startMs = qTimeToMs(m_startTimeEdit->time());
    qint64 endMs = qTimeToMs(m_endTimeEdit->time());

    emit editSaved(m_currentSegment.segmentId, text, startMs, endMs);
}

void SubtitleEditorPanel::onRevertClicked()
{
    if (m_currentSegment.segmentId < 0) return;
    setSegment(m_currentSegment);
}

QTime SubtitleEditorPanel::msToQTime(qint64 ms) const
{
    int h = static_cast<int>(ms / 3600000);
    int m = static_cast<int>((ms % 3600000) / 60000);
    int s = static_cast<int>((ms % 60000) / 1000);
    int millis = static_cast<int>(ms % 1000);
    return QTime(h, m, s, millis);
}

qint64 SubtitleEditorPanel::qTimeToMs(const QTime &t) const
{
    return t.hour() * 3600000LL + t.minute() * 60000LL + t.second() * 1000LL + t.msec();
}
