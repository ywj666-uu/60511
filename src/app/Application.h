#pragma once

#include <QApplication>
#include <memory>

class DatabaseManager;

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application();

    bool initDatabase();

    DatabaseManager* databaseManager() const;
    int currentUserId() const;
    QString currentUserName() const;
    void setCurrentUser(const QString &username, const QString &displayName);

private:
    std::unique_ptr<DatabaseManager> m_dbManager;
    int m_currentUserId = -1;
    QString m_currentUserName;
};
