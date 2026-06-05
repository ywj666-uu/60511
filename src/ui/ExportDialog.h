#pragma once

#include <QDialog>

class QComboBox;
class QLineEdit;
class QCheckBox;

class ExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExportDialog(QWidget *parent = nullptr);

    enum Format { SRT, ASS };
    Format selectedFormat() const;
    QString outputPath() const;
    bool resolveConflictsBeforeExport() const;

private slots:
    void onBrowse();

private:
    QComboBox   *m_formatCombo;
    QLineEdit   *m_pathEdit;
    QCheckBox   *m_resolveCheckbox;
};
