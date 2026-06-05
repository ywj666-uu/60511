#include "ConflictResolutionDialog.h"
#include "db/DatabaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>

ConflictResolutionDialog::ConflictResolutionDialog(
    const QVector<ConflictDetector::Conflict> &conflicts,
    DatabaseManager *db,
    QWidget *parent)
    : QDialog(parent), m_conflicts(conflicts), m_db(db)
{
    setWindowTitle(tr("Conflict Resolution"));
    resize(800, 550);

    auto *infoLabel = new QLabel(
        tr("Found %1 segments with conflicting edits.\n"
           "Text and timing conflicts are voted independently.\n"
           "Click 'Resolve All' to apply per-field majority vote merging.").arg(conflicts.size()), this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        tr("Segment"), tr("Field"), tr("Type"), tr("Variants"), tr("Editors"), tr("Details")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    populateTable();

    auto *resolveBtn = new QPushButton(tr("Resolve All (Per-Field Majority Vote)"), this);
    auto *cancelBtn = new QPushButton(tr("Cancel"), this);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(resolveBtn);
    btnLayout->addWidget(cancelBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(infoLabel);
    layout->addWidget(m_table, 1);
    layout->addLayout(btnLayout);

    connect(resolveBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ConflictResolutionDialog::populateTable()
{
    // Count total rows (one per field conflict)
    int totalRows = 0;
    for (const auto &c : m_conflicts) {
        totalRows += qMax(1, c.fieldConflicts.size());
    }
    m_table->setRowCount(totalRows);

    int row = 0;
    for (const auto &c : m_conflicts) {
        if (c.fieldConflicts.isEmpty()) {
            m_table->setItem(row, 0, new QTableWidgetItem(QString("#%1").arg(c.segmentId)));
            m_table->setItem(row, 1, new QTableWidgetItem("-"));
            m_table->setItem(row, 2, new QTableWidgetItem(c.conflictType));
            m_table->setItem(row, 3, new QTableWidgetItem(QString::number(c.numVariants)));
            m_table->setItem(row, 4, new QTableWidgetItem(QString::number(c.edits.size())));
            m_table->setItem(row, 5, new QTableWidgetItem(""));
            row++;
        } else {
            for (const auto &fc : c.fieldConflicts) {
                m_table->setItem(row, 0, new QTableWidgetItem(QString("#%1").arg(c.segmentId)));

                QString fieldLabel;
                if (fc.fieldType == "text") fieldLabel = tr("Text");
                else if (fc.fieldType == "start_ms") fieldLabel = tr("Start Time");
                else if (fc.fieldType == "end_ms") fieldLabel = tr("End Time");
                m_table->setItem(row, 1, new QTableWidgetItem(fieldLabel));
                m_table->setItem(row, 2, new QTableWidgetItem(fc.fieldType));
                m_table->setItem(row, 3, new QTableWidgetItem(QString::number(fc.numVariants)));

                int totalUsers = 0;
                for (auto it = fc.valueToUsers.begin(); it != fc.valueToUsers.end(); ++it) {
                    totalUsers += it.value().size();
                }
                m_table->setItem(row, 4, new QTableWidgetItem(QString::number(totalUsers)));

                // Show vote breakdown
                QString details;
                for (auto it = fc.valueToUsers.begin(); it != fc.valueToUsers.end(); ++it) {
                    QString val = it.key().left(25);
                    details += QString("\"%1\" x%2; ").arg(val).arg(it.value().size());
                }
                m_table->setItem(row, 5, new QTableWidgetItem(details));
                row++;
            }
        }
    }
}
