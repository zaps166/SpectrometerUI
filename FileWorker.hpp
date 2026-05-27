// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "SpectrometerWorker.hpp"

class FileWorker : public SpectrometerWorker
{
    Q_OBJECT

public:
    FileWorker(QObject *parent = nullptr);
    ~FileWorker();

    bool setParam(const QString &key, const QVariant &value) override;

    QString staticDataName() const override;

private:
    void run() override;

private:
    QString m_path;
    int m_col = 1;
    bool m_b64Name = false;
};
