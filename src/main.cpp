// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <QAction>
#include <QMenu>
#include <QQuickWindow>

#include <KGlobalAccel>
#include <KStatusNotifierItem>

#include "monitorcontroller.h"

int main(int argc, char *argv[])
{
    // QApplication (not QGuiApplication) because KStatusNotifierItem's context
    // menu is a QWidget-based QMenu under the hood.
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("CinemaMode"));
    QApplication::setApplicationName(QStringLiteral("cinemamode"));
    QApplication::setDesktopFileName(QStringLiteral("io.github.human9mil.cinemamode"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("io.github.human9mil.cinemamode")));

    // Closing the window should leave cinemamode running in the tray, not quit it.
    QApplication::setQuitOnLastWindowClosed(false);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("CinemaMode", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    auto *controller = engine.singletonInstance<MonitorController *>("CinemaMode", "MonitorController");
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());

    QAction toggleAction;
    toggleAction.setObjectName(QStringLiteral("toggleCinemaMode"));
    toggleAction.setText(QObject::tr("Toggle Cinema Mode"));
    KGlobalAccel::self()->setDefaultShortcut(&toggleAction, {QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_C)});
    KGlobalAccel::self()->setShortcut(&toggleAction, {QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_C)});
    QObject::connect(&toggleAction, &QAction::triggered, controller, &MonitorController::toggleCinema);

    auto *menu = new QMenu;
    QAction *showAction = menu->addAction(QObject::tr("Show Window"));
    QObject::connect(showAction, &QAction::triggered, window, [window] {
        if (!window) {
            return;
        }
        window->show();
        window->raise();
        window->requestActivate();
    });
    QAction *quitAction = menu->addAction(QObject::tr("Quit"));
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    KStatusNotifierItem trayIcon;
    trayIcon.setIconByName(QStringLiteral("io.github.human9mil.cinemamode"));
    trayIcon.setTitle(QObject::tr("Cinema Mode"));
    trayIcon.setCategory(KStatusNotifierItem::ApplicationStatus);
    trayIcon.setStatus(KStatusNotifierItem::Active);
    trayIcon.setStandardActionsEnabled(false);
    trayIcon.setContextMenu(menu);

    // Left-click on the tray icon toggles cinema mode directly — the whole
    // point of the tray icon is to do this without opening the window.
    QObject::connect(&trayIcon, &KStatusNotifierItem::activateRequested, controller,
                      [controller](bool active, const QPoint &) {
                          if (active) {
                              controller->toggleCinema();
                          }
                      });

    auto updateTrayTooltip = [&trayIcon, controller] {
        if (!controller) {
            return;
        }
        trayIcon.setToolTip(QStringLiteral("io.github.human9mil.cinemamode"), QObject::tr("Cinema Mode"),
                             controller->cinemaActive() ? QObject::tr("Now Showing") : QObject::tr("Standing By"));
    };
    QObject::connect(controller, &MonitorController::cinemaActiveChanged, &app, updateTrayTooltip);
    updateTrayTooltip();

    return app.exec();
}
