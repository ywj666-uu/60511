#include "Application.h"
#include "db/DatabaseManager.h"

#include <QStandardPaths>
#include <QDir>

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
}

Application::~Application() = default;

bool Application::initDatabase()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    QString dbPath = dataDir + "/subtitles.db";
    m_dbManager = std::make_unique<DatabaseManager>(dbPath);

    if (!m_dbManager->initialize()) {
        return false;
    }

    setCurrentUser("default_user", "Volunteer");
    return true;
}

DatabaseManager* Application::databaseManager() const
{
    return m_dbManager.get();
}

int Application::currentUserId() const
{
    return m_currentUserId;
}

QString Application::currentUserName() const
{
    return m_currentUserName;
}

void Application::setCurrentUser(const QString &username, const QString &displayName)
{
    m_currentUserId = m_dbManager->createUser(username, displayName);
    m_currentUserName = username;
}
