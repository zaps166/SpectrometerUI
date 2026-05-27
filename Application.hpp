// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

class SpectrometerWorker;
class SpdData;

class Application : public QGuiApplication
{
    Q_OBJECT

public:
    static Application *instance();

public:
    Application(int &argc, char **argv);
    ~Application();

    void createSpectrometerWorker(const QUrl &url);
    void deleteSpectrometerWorkerInThread();

    SpectrometerWorker *spectrometerWorker() const;

    QSettings &settings();

    QDir measurementsDir() const;

Q_SIGNALS:
    void spectrometerWorkerDeleted();

    void newSpdData(const std::shared_ptr<SpdData> &spdData);
    void newSpectrometerWorker(const QString &staticDataName);

private:
    QSettings m_settings;
    const QDir m_measurementsDir;
    SpectrometerWorker *m_spectrometerWorker = nullptr;
    bool m_deletingInThread = false;
};
