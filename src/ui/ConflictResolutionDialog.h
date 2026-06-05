#pragma once

#include <QDialog>
#include <QVector>
#include "core/ConflictDetector.h"

class QTableWidget;
class DatabaseManager;

class ConflictResolutionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConflictResolutionDialog(const QVector<ConflictDetector::Conflict> &conflicts,
                                     DatabaseManager *db,
                                     QWidget *parent = nullptr);

private:
    void populateTable();

    QTableWidget *m_table;
    QVector<ConflictDetector::Conflict> m_conflicts;
    DatabaseManager *m_db;
};
