#include "app/Application.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    app.setApplicationName("SubtitleCrowdCorrector");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SubtitleCrowd");

    if (!app.initDatabase()) {
        return -1;
    }

    MainWindow window;
    window.show();

    return app.exec();
}
