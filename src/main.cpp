#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("CinemaMode"));
    QGuiApplication::setApplicationName(QStringLiteral("cinemamode"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.cinemamode.app"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.cinemamode.app")));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("CinemaMode", "Main");

    return app.exec();
}
