// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "Application.hpp"
#include "TorchBearerWorker.hpp"
#include "FileWorker.hpp"
#include "BlackBodyWorker.hpp"
#include "DaylightWorker.hpp"

Application *Application::instance()
{
    return static_cast<Application *>(QCoreApplication::instance());
}

Application::Application(int &argc, char **argv)
    : QGuiApplication(argc, argv)
    , m_settings(QSettings::Format::IniFormat, QSettings::Scope::UserScope, u"SpectrometerUI"_s, u"Settings"_s)
    , m_measurementsDir(QFileInfo(m_settings.fileName()).path() + u"/Measurements"_s)
{
    auto format = QSurfaceFormat::defaultFormat();
#ifdef Q_OS_ANDROID
    format.setVersion(3, 0);
#endif
    format.setRedBufferSize(10);
    format.setGreenBufferSize(10);
    format.setBlueBufferSize(10);
    format.setAlphaBufferSize(2);
    if (platformName().startsWith(u"wayland"_s) || platformName() == u"cocoa"_s)
    {
        format.setColorSpace(QColorSpace(QColorSpace::Primaries::DciP3D65, QColorSpace::TransferFunction::SRgb));
    }
    else
    {
        format.setColorSpace(QColorSpace(QColorSpace::Primaries::SRgb, QColorSpace::TransferFunction::SRgb));
    }
    QSurfaceFormat::setDefaultFormat(format);

    setApplicationName(m_settings.organizationName());
    setApplicationDisplayName(u"Spectrometer UI"_s);
    setApplicationVersion(QLatin1StringView(PROJECT_VERSION));
#ifndef Q_OS_ANDROID
    setWindowIcon(QIcon(u":/icons/icon.png"_s));
#endif

    styleHints()->setMousePressAndHoldInterval(333);

    QQuickStyle::setStyle(u"Material"_s);
    QQuickWindow::setTextRenderType(QQuickWindow::CurveTextRendering);
}
Application::~Application()
{
    Q_ASSERT(!m_deletingInThread);
    delete m_spectrometerWorker; // Just in case
}

void Application::createSpectrometerWorker(const QUrl &url)
{
    if (m_deletingInThread)
    {
        return;
    }

    delete m_spectrometerWorker;
    Q_ASSERT(!m_spectrometerWorker);

    const auto scheme = url.scheme();
    if (scheme == u"device"_sv)
    {
        m_spectrometerWorker = new TorchBearerWorker;
    }
    else if (scheme == u"bb"_sv)
    {
        m_spectrometerWorker = new BlackBodyWorker;
    }
    else if (scheme == u"daylight"_sv)
    {
        m_spectrometerWorker = new DaylightWorker;
    }
    else
    {
        m_spectrometerWorker = new FileWorker;
        m_spectrometerWorker->setParam(u"path"_s, url.isLocalFile() ? url.toLocalFile() : url.toString());
    }

    const auto query = QUrlQuery(url).queryItems();
    for (auto &&[key, val] : query)
    {
        m_spectrometerWorker->setParam(key, val);
    }

    connect(m_spectrometerWorker, &QObject::destroyed, this, [this] {
        m_spectrometerWorker = nullptr;
    });
    connect(m_spectrometerWorker, &SpectrometerWorker::newSpdData, this, &Application::newSpdData);

    emit newSpectrometerWorker(m_spectrometerWorker->staticDataName());

    m_spectrometerWorker->begin();
}
void Application::deleteSpectrometerWorkerInThread()
{
    if (m_deletingInThread)
    {
        return;
    }

    auto thr = QThread::create([this] {
        delete m_spectrometerWorker;
    });
    connect(thr, &QThread::finished, thr, &QThread::deleteLater);
    connect(thr, &QThread::finished, this, [this] {
        m_deletingInThread = false;
        emit spectrometerWorkerDeleted();
    });

    if (m_spectrometerWorker)
    {
        Q_ASSERT(m_spectrometerWorker->parent() == nullptr);
        m_spectrometerWorker->moveToThread(thr);
    }
    m_deletingInThread = true;

    thr->start();
}

SpectrometerWorker *Application::spectrometerWorker() const
{
    return m_deletingInThread ? nullptr : m_spectrometerWorker;
}

QSettings &Application::settings()
{
    return m_settings;
}

QDir Application::measurementsDir() const
{
    return m_measurementsDir;
}

int main(int argc, char *argv[])
{
#ifndef USE_QT_SERIAL_PORT
# ifdef Q_OS_ANDROID
    libusb_set_option(nullptr, LIBUSB_OPTION_NO_DEVICE_DISCOVERY, nullptr);
# endif
    if (int r = libusb_init(nullptr); r < 0)
    {
        qCritical().nospace() << "libusb_init failed: LIBUSB_ERROR_" << libusb_error_name(r);
        return -1;
    }
    auto libusbExit = qScopeGuard([] {
        libusb_exit(nullptr);
    });
#endif

    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");

    Application app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] {
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.loadFromModule(
        "SpectrometerUI_files",
        "MainWindow"
    );

    return app.exec();
}
