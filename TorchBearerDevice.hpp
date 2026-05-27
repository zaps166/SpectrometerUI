// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

class CH341Uart;

class TorchBearerDevice : public QIODevice
{
public:
    TorchBearerDevice();
    ~TorchBearerDevice();

    bool open(OpenMode mode) override;
    void close() override;

    bool waitForReadyRead(int msecs) override;
    bool waitForBytesWritten(int msecs) override;

private:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

private:
    CH341Uart *m_driver;
#ifdef Q_OS_ANDROID
    bool m_openInJava = false;
#endif
    QByteArray m_rdBuff;
    QByteArray m_wrBuff;
};
