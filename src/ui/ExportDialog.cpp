#include "ExportDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Export Subtitles"));
    resize(500, 200);

    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("SRT (.srt)", static_cast<int>(SRT));
    m_formatCombo->addItem("ASS (.ass)", static_cast<int>(ASS));

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText(tr("Select output file..."));
    auto *browseBtn = new QPushButton(tr("Browse..."), this);

    m_resolveCheckbox = new QCheckBox(tr("Auto-resolve conflicts before export (majority vote)"), this);
    m_resolveCheckbox->setChecked(true);

    auto *pathLayout = new QHBoxLayout;
    pathLayout->addWidget(m_pathEdit, 1);
    pathLayout->addWidget(browseBtn);

    auto *formLayout = new QFormLayout;
    formLayout->addRow(tr("Format:"), m_formatCombo);
    formLayout->addRow(tr("Output:"), pathLayout);

    auto *okBtn = new QPushButton(tr("Export"), this);
    auto *cancelBtn = new QPushButton(tr("Cancel"), this);
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(m_resolveCheckbox);
    layout->addStretch();
    layout->addLayout(btnLayout);

    connect(browseBtn, &QPushButton::clicked, this, &ExportDialog::onBrowse);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

ExportDialog::Format ExportDialog::selectedFormat() const
{
    return static_cast<Format>(m_formatCombo->currentData().toInt());
}

QString ExportDialog::outputPath() const
{
    return m_pathEdit->text();
}

bool ExportDialog::resolveConflictsBeforeExport() const
{
    return m_resolveCheckbox->isChecked();
}

void ExportDialog::onBrowse()
{
    QString filter;
    if (selectedFormat() == SRT) {
        filter = tr("SRT Files (*.srt)");
    } else {
        filter = tr("ASS Files (*.ass)");
    }

    QString path = QFileDialog::getSaveFileName(this, tr("Export Subtitles"), QString(), filter);
    if (!path.isEmpty()) {
        m_pathEdit->setText(path);
    }
}
