// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "SpectrometerWorker.hpp"

class BlackBodyWorker : public SpectrometerWorker
{
    Q_OBJECT

public:
    BlackBodyWorker(QObject *parent = nullptr);
    ~BlackBodyWorker();

    bool setParam(const QString &key, const QVariant &value) override;

    QString staticDataName() const override;

private:
    void run() override;

private:
    double m_temperature = 0.0;
    double m_normalize = 0.0;
};
