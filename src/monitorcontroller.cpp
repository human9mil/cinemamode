// SPDX-License-Identifier: GPL-3.0-or-later
#include "monitorcontroller.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QDebug>

namespace {
constexpr int kProcessTimeoutMs = 3000;

// Only used to seed QSettings on first launch; the fallback logic in
// refreshOutputs() (the mainStillConnected check) immediately replaces this
// if it turns out not to be an output that's actually connected.
constexpr auto kFirstRunMainOutputHint = "DP-2";

// Reads the monitor's own "Display Product Name" out of its EDID, straight from
// sysfs — kscreen-doctor's JSON only ever gives us the connector name (DP-2 etc.),
// never the actual manufacturer/model, so this is the only way to show a human name.
QString friendlyNameFromEdid(const QString &connectorName)
{
    QDir drmDir(QStringLiteral("/sys/class/drm"));
    const QStringList matches = drmDir.entryList({QStringLiteral("card*-%1").arg(connectorName)}, QDir::Dirs);
    if (matches.isEmpty()) {
        return {};
    }

    QFile edidFile(drmDir.filePath(matches.first() + QStringLiteral("/edid")));
    if (!edidFile.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray edid = edidFile.readAll();

    if (edid.size() < 128 || edid.left(8) != QByteArray::fromHex("00ffffffffffff00")) {
        return {}; // not a valid base EDID block
    }

    // The four 18-byte descriptor blocks starting at offset 54 are either detailed
    // timings or "display descriptors". A display descriptor has bytes 0-1 zero and
    // byte 3 as a type tag; 0xFC is the display product name, stored as 13 bytes
    // of padded ASCII at bytes 5-17.
    for (const int offset : {54, 72, 90, 108}) {
        const QByteArray block = edid.mid(offset, 18);
        if (block.size() != 18 || block[0] != '\x00' || block[1] != '\x00') {
            continue;
        }
        if (static_cast<unsigned char>(block[3]) == 0xFC) {
            const QString name = QString::fromLatin1(block.mid(5, 13)).trimmed();
            if (!name.isEmpty()) {
                return name;
            }
        }
    }
    return {};
}

// Runs kscreen-doctor with the given arguments, waiting up to kProcessTimeoutMs.
// Returns false (after killing/reaping the process) if it hangs or fails to start.
bool runKscreenDoctor(const QStringList &args, QByteArray *standardOutput = nullptr)
{
    QProcess process;

    // Don't let our own QPA platform (e.g. "offscreen" in a headless/test run)
    // leak into kscreen-doctor's environment — it needs the real Wayland
    // connection to talk to KWin, regardless of how cinemamode itself is running.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove(QStringLiteral("QT_QPA_PLATFORM"));
    process.setProcessEnvironment(env);

    process.start(QStringLiteral("kscreen-doctor"), args);
    if (!process.waitForFinished(kProcessTimeoutMs)) {
        qWarning() << "kscreen-doctor timed out or failed to start:" << process.errorString();
        process.kill();
        process.waitForFinished(1000);
        return false;
    }

    if (standardOutput) {
        *standardOutput = process.readAllStandardOutput();
    }
    return true;
}

QJsonArray queryOutputs()
{
    QByteArray raw;
    if (!runKscreenDoctor({QStringLiteral("-j")}, &raw)) {
        return QJsonArray{};
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse kscreen-doctor JSON:" << parseError.errorString();
    }
    return doc.object().value(QStringLiteral("outputs")).toArray();
}
}

MonitorController::MonitorController(QObject *parent)
    : QObject(parent)
{
    QSettings settings;
    m_mainOutput = settings.value(QStringLiteral("mainOutput"), QString::fromLatin1(kFirstRunMainOutputHint)).toString();

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
    const QVariantList previousOutputs = m_outputs;
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

        const QString friendlyName = friendlyNameFromEdid(name);

        QVariantMap entry;
        entry[QStringLiteral("name")] = name;
        entry[QStringLiteral("enabled")] = enabled;
        entry[QStringLiteral("label")] = friendlyName.isEmpty()
            ? name
            : QStringLiteral("%1 (%2)").arg(friendlyName, name);
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

    if (!sawConnected) {
        qWarning() << "No connected monitor outputs detected from kscreen-doctor";
    }

    if (m_outputs != previousOutputs) {
        emit outputsChanged();
    }

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

    runKscreenDoctor(args);

    refreshOutputs();
}
