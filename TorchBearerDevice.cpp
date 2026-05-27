// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "TorchBearerDevice.hpp"
#include "CH341Uart.hpp"

#ifdef Q_OS_ANDROID
constexpr const auto className = "org/zaps166/UsbFD/UsbFD";
#endif

constexpr uint16_t VID = 0x1a86;
constexpr uint16_t PID = 0x7523;

TorchBearerDevice::TorchBearerDevice()
    : m_driver(new CH341Uart)
{
}
TorchBearerDevice::~TorchBearerDevice()
{
#ifdef Q_OS_ANDROID
    if (m_openInJava)
    {
        m_driver->close();
        QJniObject::callStaticMethod(className, "close");
    }
#endif
    delete m_driver;
}

bool TorchBearerDevice::open(OpenMode mode)
{
    if (mode != ReadWrite || isOpen())
    {
        return false;
    }

    try
    {
#ifdef Q_OS_ANDROID
        jint fd = -1;

        if (m_openInJava)
        {
            return false;
        }
        for (;;)
        {
            auto waitForPermissionsEventLoop = make_shared<QEventLoop>();

            fd = QJniObject::callStaticMethod<jint>(
                className,
                "getFD",
                "(Landroid/content/Context;JII)I",
                QNativeInterface::QAndroidApplication::context().object(),
                new weak_ptr(waitForPermissionsEventLoop),
                VID,
                PID
            );
            if (fd < 0)
            {
                if (fd == -1)
                {
                    setErrorString(tr("Can't find USB device"));
                }
                else if (waitForPermissionsEventLoop->exec() == 0)
                {
                    continue;
                }
                return false;
            }

            break;
        }
        m_openInJava = true;

        libusb_device_handle *devh = nullptr;
        if (libusb_wrap_sys_device(nullptr, fd, &devh) != 0)
        {
            setErrorString(tr("Can't wrap USB device"));
            return false;
        }

        m_driver->init(devh);
#else
        m_driver->init(VID, PID);
#endif
        m_driver->open();
    }
    catch (const exception &e)
    {
        setErrorString(QString::fromUtf8(e.what()));
        return false;
    }

    return QIODevice::open(mode);
}
void TorchBearerDevice::close()
{
    m_driver->close();
    QIODevice::close();
}

bool TorchBearerDevice::waitForReadyRead(int msecs)
{
    constexpr qint64 maxRead = 2048;

    const auto rdBuffPos = m_rdBuff.size();
    m_rdBuff.reserve(m_rdBuff.size() + maxRead);
    m_rdBuff.resize(m_rdBuff.size() + maxRead);

    const auto bread = clamp<decltype(maxRead)>(
        m_driver->read(reinterpret_cast<uint8_t *>(m_rdBuff.data() + rdBuffPos), maxRead, msecs),
        0,
        maxRead
    );
    m_rdBuff.remove(rdBuffPos + bread, maxRead - bread);

    return bread > 0;
}
bool TorchBearerDevice::waitForBytesWritten(int msecs)
{
    qint64 bwrite = m_driver->write(reinterpret_cast<const uint8_t *>(m_wrBuff.constData()), m_wrBuff.size(), msecs);
    if (bwrite > 0)
    {
        m_wrBuff.remove(0, bwrite);
        return true;
    }
    return false;
}

qint64 TorchBearerDevice::readData(char *data, qint64 maxSize)
{
    const qint64 toRead = min(maxSize, m_rdBuff.size());
    memcpy(data, m_rdBuff.constData(), toRead);
    m_rdBuff.remove(0, toRead);
    return toRead;
}
qint64 TorchBearerDevice::writeData(const char *data, qint64 maxSize)
{
    m_wrBuff.append(data, maxSize);
    return maxSize;
}

#ifdef Q_OS_ANDROID
extern "C" JNIEXPORT void JNICALL
Java_org_zaps166_UsbFD_UsbFD_permissionsGrantedCallback(JNIEnv *env, jobject obj, jlong ptr, jboolean granted)
{
    // It's called from Java thread when permission dialog popups
    auto weakPtrPtr = reinterpret_cast<weak_ptr<QEventLoop> *>(ptr);
    auto waitForPermissionsEventLoop = weakPtrPtr->lock();
    delete weakPtrPtr;
    if (waitForPermissionsEventLoop && waitForPermissionsEventLoop->isRunning())
    {
        // Exit event loop from event loop thread
        QMetaObject::invokeMethod(waitForPermissionsEventLoop.get(), [waitForPermissionsEventLoop, granted] {
            waitForPermissionsEventLoop->exit(granted ? 0 : 1);
        }, Qt::QueuedConnection);
    }
}
#endif
