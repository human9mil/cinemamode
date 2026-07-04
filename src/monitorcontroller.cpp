#include "monitorcontroller.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QStringList>

namespace {
constexpr auto kDefaultMainOutput = "DP-2";

QJsonArray queryOutputs()
{
    QProcess process;

    // Don't let our own QPA platform (e.g. "offscreen" in a headless/test run)
    // leak into kscreen-doctor's environment — it needs the real Wayland
    // connection to talk to KWin, regardless of how cinemamode itself is running.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove(QStringLiteral("QT_QPA_PLATFORM"));
    process.setProcessEnvironment(env);

    process.start(QStringLiteral("kscreen-doctor"), {QStringLiteral("-j")});
    process.waitForFinished(3000);
    const QByteArray raw = process.readAllStandardOutput();

    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    return doc.object().value(QStringLiteral("outputs")).toArray();
}
}

MonitorController::MonitorController(QObject *parent)
    : QObject(parent)
{
    QSettings settings;
    m_mainOutput = settings.value(QStringLiteral("mainOutput"), QString::fromLatin1(kDefaultMainOutput)).toString();

    refreshOutputs();
}

QVariantList MonitorController::outputs() const
{
    return m_outputs;
}

QString MonitorController::mainOutput() const
{
    return m_mainOutput;
}

void MonitorController::setMainOutput(const QString &name)
{
    if (m_mainOutput == name) {
        return;
    }
    m_mainOutput = name;

    QSettings settings;
    settings.setValue(QStringLiteral("mainOutput"), m_mainOutput);

    emit mainOutputChanged();
}

bool MonitorController::cinemaActive() const
{
    return m_cinemaActive;
}

void MonitorController::refreshOutputs()
{
    m_outputs.clear();

    const QJsonArray outputs = queryOutputs();
    bool anyDisabled = false;
    bool sawConnected = false;

    for (const QJsonValue &value : outputs) {
        const QJsonObject obj = value.toObject();
        if (!obj.value(QStringLiteral("connected")).toBool()) {
            continue;
        }

        const QString name = obj.value(QStringLiteral("name")).toString();
        const bool enabled = obj.value(QStringLiteral("enabled")).toBool();

        QVariantMap entry;
        entry[QStringLiteral("name")] = name;
        entry[QStringLiteral("enabled")] = enabled;
        m_outputs.append(entry);

        sawConnected = true;
        if (name != m_mainOutput && !enabled) {
            anyDisabled = true;
        }
    }

    // If mainOutput no longer exists among connected outputs, fall back to the first connected one.
    bool mainStillConnected = false;
    for (const QVariant &v : m_outputs) {
        if (v.toMap().value(QStringLiteral("name")).toString() == m_mainOutput) {
            mainStillConnected = true;
            break;
        }
    }
    if (!mainStillConnected && sawConnected) {
        setMainOutput(m_outputs.first().toMap().value(QStringLiteral("name")).toString());
    }

    emit outputsChanged();

    const bool newActive = anyDisabled;
    if (newActive != m_cinemaActive) {
        m_cinemaActive = newActive;
        emit cinemaActiveChanged();
    }
}

void MonitorController::toggleCinema()
{
    QStringList args;
    for (const QVariant &v : m_outputs) {
        const QVariantMap entry = v.toMap();
        const QString name = entry.value(QStringLiteral("name")).toString();
        if (name == m_mainOutput) {
            continue; // never touch the main output
        }
        args << QStringLiteral("output.%1.%2").arg(name, m_cinemaActive ? QStringLiteral("enable") : QStringLiteral("disable"));
    }

    if (args.isEmpty()) {
        return;
    }

    QProcess::execute(QStringLiteral("kscreen-doctor"), args);

    refreshOutputs();
}
