// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QVariantList>
#include <QQmlEngine>

class MonitorController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList outputs READ outputs NOTIFY outputsChanged)
    Q_PROPERTY(QString mainOutput READ mainOutput WRITE setMainOutput NOTIFY mainOutputChanged)
    Q_PROPERTY(bool cinemaActive READ cinemaActive NOTIFY cinemaActiveChanged)

public:
    explicit MonitorController(QObject *parent = nullptr);

    QVariantList outputs() const;
    QString mainOutput() const;
    void setMainOutput(const QString &name);
    bool cinemaActive() const;

public slots:
    void refreshOutputs();
    void toggleCinema();

signals:
    void outputsChanged();
    void mainOutputChanged();
    void cinemaActiveChanged();

private:
    QVariantList m_outputs;
    QString m_mainOutput;
    bool m_cinemaActive = false;
};
