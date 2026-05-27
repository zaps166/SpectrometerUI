// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "TorchBearerWorker.hpp"
#ifndef USE_QT_SERIAL_PORT
# include "TorchBearerDevice.hpp"
#endif

class WriteException : public runtime_error
{
public:
    WriteException()
        : runtime_error("Unable to write data to the device")
    {}
};
class ReadException : public runtime_error
{
public:
    ReadException(const string &msg)
        : runtime_error("Unable to read from the device: "s + msg)
    {}
};

struct TorchBearerWorker::Message
{
    using DeviceID = QString;
    using Exposure = uint32_t;
    using Range = pair<uint16_t, uint16_t>;
    struct Spectrum
    {
        ExposureStatus exposureStatus;
        float exposureTime;
        Data data;
    };

    MessageType type;
    variant<DeviceID, ExposureMode, Exposure, Range, Spectrum> data;
};

QStringList TorchBearerWorker::scanDevices()
{
#ifdef USE_QT_SERIAL_PORT
    return QSerialPortInfo::availablePorts() | views::transform([](const QSerialPortInfo &info) {
        return info.portName();
    }) | ranges::to<QStringList>();
#else
    return QStringList();
#endif
}

TorchBearerWorker::TorchBearerWorker(QObject *parent)
    : SpectrometerWorker(parent)
{
}
TorchBearerWorker::~TorchBearerWorker()
{
    {
        scoped_lock locker(m_mutex);
        m_br = true;
        m_cv.notify_all();
    }
    wait();
}

bool TorchBearerWorker::setParam(const QString &key, const QVariant &value)
{
    using enum TorchBearerWorker::MessageType;

    bool ok = false;

    if (!isRunning())
    {
        if (key == u"device"_sv)
        {
            m_dev = value.toString();
            return true;
        }
        else if (key == u"openerr"_sv)
        {
            m_openError = value.toBool();
            return true;
        }
        else if (key == u"min"_sv)
        {
            m_hintMinNm = value.toDouble();
            return true;
        }
        else if (key == u"max"_sv)
        {
            m_hintMaxNm = value.toDouble();
            return true;
        }
    }

    scoped_lock locker(m_mutex);

    if (key == u"running"_sv)
    {
        const auto valueStr = value.isNull() ? QString() : value.toString();
        const bool isContinuous = (valueStr == u"continuous"_s);
        const bool isSingle = (valueStr == u"single"_s);
        if (isContinuous || isSingle)
        {
            m_commands.emplace_back(GET_DATA, QByteArray());
            m_singleMeasurement = isSingle;

            // Sometimes we obtain spectrum from previous measurement, so measure twice
            m_ignoreDataForSingleMeasurement = m_singleMeasurement;
        }
        else
        {
            m_commands.emplace_back(STOP, QByteArray());
            m_singleMeasurement = false;
        }
        ok = true;
    }

    if (key == u"exposure"_sv)
    {
        if (value.isNull())
        {
            // Prevent device hang
            if (m_exposure.time < m_minAllowedAutoExposure)
            {
                m_commands.emplace_back(SET_EXPOSURE, intToBa(static_cast<int>(m_minAllowedAutoExposure * 1e3)));
                m_commands.emplace_back(GET_EXPOSURE, QByteArray());
            }
            else if (m_exposure.time > m_maxAllowedAutoExposure)
            {
                m_commands.emplace_back(SET_EXPOSURE, intToBa(static_cast<int>(m_maxAllowedAutoExposure * 1e3)));
                m_commands.emplace_back(GET_EXPOSURE, QByteArray());
            }

            m_commands.emplace_back(SET_EXPOSURE_MODE, intToBa(ExposureMode::AUTOMATIC));
            m_commands.emplace_back(GET_EXPOSURE_MODE, QByteArray());

            ok = true;
        }
        else if (const auto exposureTime = value.toDouble(); exposureTime > 0.0)
        {
            m_commands.emplace_back(SET_EXPOSURE_MODE, intToBa(ExposureMode::MANUAL));
            m_commands.emplace_back(GET_EXPOSURE_MODE, QByteArray());
            m_commands.emplace_back(SET_EXPOSURE, intToBa(static_cast<int>(exposureTime * 1e3)));
            m_commands.emplace_back(GET_EXPOSURE, QByteArray());

            ok = true;
        }
    }

    m_mustEmitCommandsFinished = true;
    if (Q_UNLIKELY(!ok))
    {
        processCommandsFinished();
    }

    m_cv.notify_all();

    return ok;
}

QString TorchBearerWorker::staticDataName() const
{
    return QString();
}

void TorchBearerWorker::run()
{
    using enum TorchBearerWorker::MessageType;

    QString errStr;

    auto selfDestroy = qScopeGuard([this, &errStr] {
        processCommandsFinished();
        if (!errStr.isEmpty())
        {
            qCritical().noquote() << errStr;
            if (m_openError)
            {
                emit errorMesssage(errStr);
            }
        }
        deleteLater();
    });

#ifdef USE_QT_SERIAL_PORT
    if (m_dev.isEmpty())
    {
        m_dev = scanDevices().value(0);
    }
    QSerialPort device(m_dev);
    device.setBaudRate(115200);
#else
    TorchBearerDevice device;
#endif

    if (!device.open(QIODevice::ReadWrite))
    {
        errStr = tr("Unable to open device: %1").arg(device.errorString());
        return;
    }

    bool working = false;

    m_openError = true;

    try
    {
        xferMessage(device, GET_DEVICE_ID, intToBa(static_cast<uint8_t>(0x18)));
        if (m_deviceId.isEmpty())
        {
            throw runtime_error("Unable to get the device ID");
        }

        xferMessage(device, GET_RANGE);
        if (m_minNm <= 0.0 || m_maxNm < m_minNm)
        {
            throw runtime_error("Unable to get the device range");
        }

        m_exposure.min = 0.1;
        m_exposure.max = 5000.0;

        xferMessage(device, GET_EXPOSURE);
        if (m_exposure.time < 0.0)
        {
            throw runtime_error("Unable to get exposure time");
        }

        xferMessage(device, GET_EXPOSURE_MODE);
        if (!m_exposureMode.has_value())
        {
            throw runtime_error("Unable to get exposure mode");
        }

        processSpd(Data());

        bool singleMeasurement = false;

        for (;;)
        {
            Commands commands;

            {
                unique_lock locker(m_mutex);

                if (!m_br && !working && m_commands.empty())
                {
                    while (m_cv.wait_for(locker, 1.5s) == cv_status::timeout)
                    {
                        // Ping the device to check if it's still alive
                        xferMessage(device, GET_EXPOSURE_MODE);
                    }
                }

                if (m_br)
                {
                    break;
                }

                commands = std::move(m_commands);

                if (m_singleMeasurement)
                {
                    singleMeasurement = m_singleMeasurement;
                    m_singleMeasurement = false;
                }
                else if (singleMeasurement && working && (!m_exposure.aeInProgress || (m_exposure.ae && m_exposure.time == m_maxAllowedAutoExposure)))
                {
                    if (m_ignoreDataForSingleMeasurement)
                    {
                        m_ignoreDataForSingleMeasurement = false;
                    }
                    else
                    {
                        commands.emplace_back(STOP, QByteArray());
                        singleMeasurement = false;
                    }
                }
            }

            if (!commands.empty())
            {
                bool restoreWorking = working;

                if (working)
                {
                    working = false;
                    xferMessage(device, STOP);
                }

                for (auto &&[messageType, messageValue] : as_const(commands))
                {
                    if (messageType == STOP)
                    {
                        restoreWorking = false;
                        continue;
                    }

                    if (messageType == GET_DATA)
                    {
                        restoreWorking = true;
                        continue;
                    }

                    xferMessage(device, messageType, messageValue);
                }

                if (!singleMeasurement)
                {
                    processCommandsFinished();
                }

                if (restoreWorking)
                {
                    xferMessage(device, GET_DATA);
                    working = true;
                }
            }

            if (working)
            {
                readData(device);
            }
        }
    }
    catch (const ReadException &e)
    {
        errStr = QString::fromUtf8(e.what());
        // Try to stop the device if it's working
    }
    catch (const runtime_error &e)
    {
        errStr = QString::fromUtf8(e.what());
        return;
    }

    if (working)
    {
        try
        {
            xferMessage(device, STOP);
        }
        catch (...)
        {}
    }
}

void TorchBearerWorker::xferMessage(QIODevice &device, MessageType messageType, const QByteArrayView data)
{
    const auto messageData = buildMessage(messageType, data);
    if (device.write(messageData) < messageData.size() || !device.waitForBytesWritten(1000))
    {
        throw WriteException();
    }
    if (messageType != MessageType::GET_DATA)
    {
        readData(device);
    }
}

void TorchBearerWorker::processCommandsFinished()
{
    if (m_mustEmitCommandsFinished)
    {
        m_mustEmitCommandsFinished = false;
        emit commandsFinished();
    }
}

uint8_t TorchBearerWorker::calculateChecksum(const QByteArrayView message)
{
    return ranges::fold_left(message, 0, plus<uint8_t>());
}
QByteArray TorchBearerWorker::buildMessage(MessageType messageType, const QByteArrayView data)
{
    QByteArray message;
    message.reserve(2 + 3 + 1 + data.size() + 1 + 2);
    auto append_value = [&](auto value, qsizetype len) {
        message.append(reinterpret_cast<const char *>(&value), min<qsizetype>(len, sizeof(value)));
    };
    message += "\xCC\x01";
    append_value(9 + data.size(), 3);
    append_value(static_cast<int>(messageType), 1);
    message += data;
    append_value(calculateChecksum(message), 1);
    message += "\r\n";
    return message;
}
QByteArray TorchBearerWorker::intToBa(auto val)
{
    return QByteArray(reinterpret_cast<const char *>(&val), sizeof(val));
}

optional<TorchBearerWorker::Message> TorchBearerWorker::parseMessage(MessageType messageType, const QByteArrayView data) const
{
    optional<TorchBearerWorker::Message> message;
    auto getMessageData = [&]()->decltype(Message::data)& {
        message.emplace().type = messageType;
        return message->data;
    };
    switch (messageType)
    {
        using enum TorchBearerWorker::MessageType;
        case GET_DEVICE_ID:
            if (!data.empty())
            {
                getMessageData() = QString::fromLatin1(data);
            }
            break;
        case GET_EXPOSURE_MODE:
            if (data.size() == sizeof(ExposureMode))
            {
                getMessageData() = static_cast<ExposureMode>(data[0]);
            }
            break;
        case GET_EXPOSURE:
            if (data.size() == sizeof(Message::Exposure))
            {
                getMessageData() = *reinterpret_cast<const Message::Exposure *>(data.constData());
            }
            break;
        case GET_RANGE:
            if (data.size() == sizeof(uint16_t) * 2)
            {
                getMessageData() = Message::Range {
                    reinterpret_cast<const uint16_t &>(*data.mid(0).constData()),
                    reinterpret_cast<const uint16_t &>(*data.mid(2).constData()),
                };
            }
            break;
        case GET_DATA:
            if (constexpr qsizetype spectrumInfoSize = 19; data.size() > spectrumInfoSize && (data.size() - spectrumInfoSize) % sizeof(uint16_t) == 0)
            {
                auto &spectrum = getMessageData().emplace<Message::Spectrum>(Message::Spectrum {
                    .exposureStatus = reinterpret_cast<const ExposureStatus &>(*data.mid(0).constData()),
                    .exposureTime = reinterpret_cast<const uint32_t &>(*data.mid(1).constData()) / 1000.0f
                });
                const auto encodedExponent = qFromBigEndian(reinterpret_cast<const uint16_t &>(*data.mid(5).constData()));
                const auto sn = reinterpret_cast<const uint32_t &>(*data.mid(7).constData());
                const auto exInfo = reinterpret_cast<const uint64_t &>(*data.mid(11).constData());
                const auto exposureTimeData = reinterpret_cast<const uint32_t &>(spectrum.exposureTime);
                const uint32_t common = qbswap(exposureTimeData) ^ exInfo >> 16;
                const uint16_t keyA = (common ^ (exposureTimeData ^ sn) >> 16 ^ sn ^ exInfo) & 0xFFFF;
                const uint16_t keyB = ((common >> 16) ^ exposureTimeData ^ sn) & 0xFFFF;
                const uint16_t exponent = encodedExponent ^ 8848;
                const auto scale = pow(10.0f, static_cast<float>(exponent));
                const auto encodedSpectrum = span(reinterpret_cast<const uint16_t *>(data.constData() + spectrumInfoSize), (data.size() - spectrumInfoSize) / sizeof(uint16_t));
                const auto midpoint = encodedSpectrum.size() / 2;
                spectrum.data = views::zip(views::iota(0u), encodedSpectrum) | views::transform([=, minNm = m_minNm](auto &&zipped) {
                    const auto [index, encodedSpectrumValue] = zipped;
                    return make_pair<double, double>(index + minNm, (encodedSpectrumValue ^ (index < midpoint ? keyA : keyB)) / scale);
                }) | ranges::to<QList>();
            }
            break;
        case STOP:
        case SET_EXPOSURE_MODE:
        case SET_EXPOSURE:
            getMessageData();
            break;
        default:
            qDebug() << "Unknown message type:" << messageType << data.size() << "bytes";
            break;
    }
    return message;
}

void TorchBearerWorker::readData(QIODevice &device)
{
#pragma pack(1)
    struct MessageHeaderBase
    {
        uint8_t xCC;
        uint8_t x81;
        int len : 24;
    };
    struct MessageHeader : public MessageHeaderBase
    {
        MessageType messageType;
    };
    struct MessageFooter
    {
        uint8_t checksum;
        uint8_t r;
        uint8_t n;
    };
#pragma pack()

    QByteArray buffer;

    while (device.waitForReadyRead(7500))
    {
        buffer += device.readAll();

        if (buffer.size() < static_cast<qsizetype>(sizeof(MessageHeaderBase)))
        {
            qWarning() << "Message too short for header, waiting for more data";
            continue;
        }

        const auto headerBase = reinterpret_cast<const MessageHeaderBase *>(buffer.constData());
        if (headerBase->xCC != 0xCC || headerBase->x81 != 0x81)
        {
            throw ReadException("Invalid message header"s);
        }

        const qsizetype length = headerBase->len;
        if (length < static_cast<qsizetype>(sizeof(MessageHeader) + sizeof(MessageFooter)))
        {
            throw ReadException("Invalid message length"s);
        }
        if (length > buffer.size())
        {
            // Waiting for more data
            continue;
        }

        auto removeFromBufferScoped = qScopeGuard([&buffer, length] {
            buffer.remove(0, length);
        });

        const auto header = reinterpret_cast<const MessageHeader *>(headerBase);

        const auto footer = reinterpret_cast<const MessageFooter *>(buffer.constData() + length - sizeof(MessageFooter));
        if (footer->r != '\r' || footer->n != '\n')
        {
            throw ReadException("Invalid message footer"s);
        }

        if (calculateChecksum(QByteArrayView(buffer.constData(), length - sizeof(MessageFooter))) != footer->checksum)
        {
            throw ReadException("Wrong message checksum"s);
        }

        const QByteArrayView messageData(buffer.constData() + sizeof(MessageHeader), length - sizeof(MessageHeader) - sizeof(MessageFooter));
        if (const auto message = parseMessage(header->messageType, messageData); Q_LIKELY(message.has_value()))
        {
            using enum TorchBearerWorker::MessageType;
            switch (message->type)
            {
                case STOP:
                {
                    if (m_exposure.aeInProgress)
                    {
                        m_exposure.aeInProgress = false;
                        emitNewExposureData();
                    }
                    break;
                }
                case GET_DEVICE_ID:
                {
                    m_deviceId = get<Message::DeviceID>(message->data);
                    break;
                }
                case GET_RANGE:
                {
                    const auto [minNm, maxNm] = get<Message::Range>(message->data);
                    m_minNm = minNm;
                    m_maxNm = maxNm;
                    break;
                }
                case GET_EXPOSURE_MODE:
                {
                    m_exposureMode = get<ExposureMode>(message->data);
                    m_exposure.ae = (m_exposureMode.value() == ExposureMode::AUTOMATIC);
                    if (!m_exposure.ae)
                    {
                        m_exposure.aeInProgress = false;
                    }
                    emitNewExposureData();
                    break;
                }
                case GET_EXPOSURE:
                {
                    m_exposure.time = get<Message::Exposure>(message->data) / 1e3;
                    emitNewExposureData();
                    break;
                }
                case GET_DATA:
                {
                    if (!m_ignoreDataForSingleMeasurement)
                    {
                        const auto &spectrum = get<Message::Spectrum>(message->data);
                        m_exposure.aeInProgress = (spectrum.exposureStatus != ExposureStatus::NORMAL);
                        m_exposure.time = spectrum.exposureTime;
                        setCurrentDate();
                        emitNewExposureData();
                        processSpd(spectrum.data);
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
        }

        return;
    }

    throw ReadException("Timeout"s);
}
