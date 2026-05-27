// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "SpectrometerWorker.hpp"

class TorchBearerWorker : public SpectrometerWorker
{
    Q_OBJECT

    enum class MessageType : uint8_t
    {
        STOP = 0x04,
        SET_DEVICE_ID = 0x07, // Don't use, it'll change the device ID permanently!
        GET_DEVICE_ID = 0x08,
        SET_EXPOSURE_MODE = 0x0A,
        GET_EXPOSURE_MODE = 0x0B,
        SET_EXPOSURE = 0x0C,
        GET_EXPOSURE = 0x0D,
        GET_RANGE = 0x0F,
        GET_DATA = 0x33,
    };
    Q_ENUM(MessageType)

    enum class ExposureMode : uint8_t
    {
        MANUAL = 0x00,
        AUTOMATIC = 0x01,
    };
    Q_ENUM(ExposureMode)

    enum class ExposureStatus : uint8_t
    {
        NORMAL = 0x00,
        OVER = 0x01,
        UNDER = 0x02,
    };
    Q_ENUM(ExposureStatus)

    using Commands = QList<pair<MessageType, QByteArray>>;

    struct Message;

public:
    static QStringList scanDevices();

public:
    TorchBearerWorker(QObject *parent = nullptr);
    ~TorchBearerWorker();

    bool setParam(const QString &key, const QVariant &value) override;

    QString staticDataName() const override;

private:
    void run() override;

    void xferMessage(QIODevice &device, MessageType messageType, const QByteArrayView data = QByteArrayView());

    void processCommandsFinished();

private:
    static uint8_t calculateChecksum(const QByteArrayView message);
    static QByteArray buildMessage(MessageType messageType, const QByteArrayView data);
    static QByteArray intToBa(auto val);

private:
    optional<Message> parseMessage(MessageType message_type, const QByteArrayView data) const;

private:
    void readData(QIODevice &device);

private:
    bool m_br = false;

    const double m_minAllowedAutoExposure = 1.0;
    const double m_maxAllowedAutoExposure = 2000.0;

    QString m_dev;

    bool m_openError = true;

    optional<ExposureMode> m_exposureMode;

    mutex m_mutex;
    condition_variable m_cv;

    Commands m_commands;
    bool m_ignoreDataForSingleMeasurement = false;
    bool m_singleMeasurement = false;
    bool m_mustEmitCommandsFinished = false;
};
