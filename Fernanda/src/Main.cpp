#include <QApplication>
#include <QStringList>

#include "Coco/StartCop.h"

#include "BuildMessages.h"
#include "Environment.h"
#include "Version.h"

int main(int argc, char* argv[])
{
    Coco::StartCop cop("Fernanda", argc, argv);
    if (cop.isRunning()) return 0;

    QApplication app(argc, argv);
    QApplication::setOrganizationName(VERSION_AUTHOR_STRING);
    QApplication::setOrganizationDomain(VERSION_DOMAIN);
    QApplication::setApplicationName(VERSION_APP_NAME_STRING);
    QApplication::setApplicationVersion(VERSION_FULL_STRING);

    Environment environment{};

    environment.connect
    (
        &cop,
        &Coco::StartCop::appRelaunched,
        &environment,
        &Environment::onStartCopAppRelaunched
    );

    return app.exec();
}
