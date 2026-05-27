// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "SpectrometerBridge.hpp"
#include "SpectrometerWorker.hpp"
#include "TorchBearerWorker.hpp"
#include "Application.hpp"

enum class DataRole : int
{
    Id = Qt::UserRole,
    Key,
    Value,
    LikedPos,
    Base,
    PPFD,
    BlueLight,
};

namespace {

constexpr qsizetype g_maxLikedPos = 5;

const auto g_noData = u"-"_s;

const auto g_date = u"date"_s;
const auto g_dev = u"dev"_s;
const auto g_time = u"time"_s;
const auto g_ee = u"ee"_s;
const auto g_lux = u"lux"_s;
const auto g_cct = u"cct"_s;
const auto g_duv = u"duv"_s;
const auto g_sdcm = u"sdcm"_s;
const auto g_ra = u"ra"_s;
const auto g_rf = u"rf"_s;
const auto g_rg = u"rg"_s;
const auto g_rx = u"r%1"_s;
const auto g_rfhx = u"rfh%1"_s;
const auto g_lp = u"lp"_s;
const auto g_ld = u"ld"_s;
const auto g_X = u"X"_s;
const auto g_Y = u"Y"_s;
const auto g_Z = u"Z"_s;
const auto g_x = u"x"_s;
const auto g_y = u"y"_s;
const auto g_ppfd = u"ppfd"_s;
const auto g_par = u"par"_s;
const auto g_eb = u"eb"_s;
const auto g_blr = u"blr"_s;
const auto g_mlux = u"mlux"_s;
const auto g_mr = u"mr"_s;
const auto g_mder = u"mder"_s;
const auto g_uvi = u"uvi"_s;
const auto g_vitamind = u"vitamind"_s;

}

SpectrometerBridge::SpectrometerBridge()
    : m_spdData(make_shared<SpdData>())
{
    auto &settings = Application::instance()->settings();
    settings.beginGroup(u"Liked"_s);
    bool restoredAny = false;
    for (const auto ids = settings.childKeys(); auto &&id : ids)
    {
        bool ok = false;
        const qsizetype likedPos = settings.value(id).toLongLong(&ok);
        if (ok && likedPos >= 0 && likedPos <= g_maxLikedPos)
        {
            m_likedSettings[id] = likedPos;
            restoredAny = true;
        }
    }
    if (!restoredAny)
    {
        // Initialize
        m_likedSettings[g_cct] = 0;
        m_likedSettings[g_duv] = 1;
        m_likedSettings[g_ee] = 2;
        m_likedSettings[g_lp] = 3;
        m_likedSettings[g_lux] = 4;
        m_likedSettings[g_ra] = 5;
        for (auto &&[key, val] : as_const(m_likedSettings).asKeyValueRange())
        {
            settings.setValue(key, val);
        }
    }
    settings.endGroup();

    connect(Application::instance(), &Application::spectrometerWorkerDeleted, this, &SpectrometerBridge::quit);

    connect(Application::instance(), &Application::newSpectrometerWorker, this, [this](const QString &staticDataName) {
        m_worker = Application::instance()->spectrometerWorker();
        connect(m_worker, &QObject::destroyed, this, [this] {
            m_worker = nullptr;
            onNewExposureData(Exposure());
        });
        connect(m_worker, &SpectrometerWorker::newExposureData, this, &SpectrometerBridge::onNewExposureData);
        connect(m_worker, &SpectrometerWorker::newSpdData, this, &SpectrometerBridge::onNewSpdData);
        connect(m_worker, &SpectrometerWorker::commandsFinished, this, &SpectrometerBridge::commandsFinished);
        connect(m_worker, &SpectrometerWorker::errorMesssage, this, &SpectrometerBridge::errorMessage);
        emit staticDataNameChanged(staticDataName);
    });
}
SpectrometerBridge::~SpectrometerBridge()
{
}

void SpectrometerBridge::getWindowColorProfile(QQuickWindow *win)
{
    Q_ASSERT(win);
    auto conn = make_shared<QMetaObject::Connection>();
    *conn = connect(win, &QQuickWindow::frameSwapped, this, [conn, win, this] {
        if (win->format().colorSpace().primaries() == QColorSpace::Primaries::DciP3D65)
        {
            m_isP3 = true;
            emit isP3Changed();
        }
        disconnect(*conn);
    });
}

void SpectrometerBridge::storeSplitView(const QByteArray &data)
{
    Application::instance()->settings().setValue("UI/SplitView", data);
}
QByteArray SpectrometerBridge::restoreSplitView()
{
    return Application::instance()->settings().value("UI/SplitView").toByteArray();
}

void SpectrometerBridge::create(const QUrl &url)
{
    Application::instance()->createSpectrometerWorker(url);
}
void SpectrometerBridge::createDevice(bool openErr)
{
    const auto &sets = Application::instance()->settings();
    const auto device = sets.value("Device/Value").toString();
    const auto minNm = QString::number(sets.value("Device/MinNm").toDouble());
    const auto maxNm = QString::number(sets.value("Device/MaxNm").toDouble());
    Application::instance()->createSpectrometerWorker(QUrl(
        u"device:/?openerr=" + QString::number(static_cast<int>(openErr)) + u"&min="_s + minNm + u"&max="_s + maxNm + u"&device="_s + device
    ));
}

void SpectrometerBridge::setDeviceParams(double minNm, double maxNm, const QString &device)
{
    auto &sets = Application::instance()->settings();
    sets.setValue("Device/Value", device);
    sets.setValue("Device/MinNm", minNm);
    sets.setValue("Device/MaxNm", maxNm);
}
QVariantMap SpectrometerBridge::getDeviceParams()
{
    QVariantMap ret;
    const auto &sets = Application::instance()->settings();
    ret[u"dev"_s] = sets.value("Device/Value").toString();
    ret[u"min"_s] = sets.value("Device/MinNm").toDouble();
    ret[u"max"_s] = sets.value("Device/MaxNm").toDouble();
    return ret;
}

void SpectrometerBridge::requestQuit()
{
    m_worker = nullptr; // We can't use it anymore
    Application::instance()->deleteSpectrometerWorkerInThread();
}

QStringList SpectrometerBridge::getDevices()
{
    return TorchBearerWorker::scanDevices();
}

void SpectrometerBridge::start(bool single)
{
    if (!m_worker)
        return;

    emit commandsStarted();
    m_worker->setParam(u"running"_s, single ? u"single"_s : u"continuous"_s);
}
void SpectrometerBridge::stop()
{
    if (!m_worker)
        return;

    emit commandsStarted();
    m_worker->setParam(u"running"_s, QVariant());
}

void SpectrometerBridge::setExposure(bool autoExposure, qreal exposureTime)
{
    if (!m_worker)
        return;

    emit commandsStarted();
    m_worker->setParam(u"exposure"_s, autoExposure ? QVariant() : exposureTime);
}

int SpectrometerBridge::getIdRole() const
{
    return to_underlying(DataRole::Id);
}
int SpectrometerBridge::getLikedPosRole() const
{
    return to_underlying(DataRole::LikedPos);
}

void SpectrometerBridge::setLiked(const QString &id, qsizetype likedPos)
{
    if (const auto it = ranges::find(m_entryList, id, &ModelEntry::id); Q_LIKELY(it != m_entryList.end()))
    {
        if (const auto index = this->index(distance(m_entryList.begin(), it)); Q_LIKELY(index.isValid()))
        {
            if (likedPos >= 0 && likedPos <= g_maxLikedPos)
            {
                it->likedPos = likedPos;
                m_likedSettings[id] = likedPos;
                Application::instance()->settings().setValue(u"Liked/"_s + id, likedPos);
            }
            else
            {
                it->likedPos = -1;
                m_likedSettings.remove(id);
                Application::instance()->settings().remove(u"Liked/"_s + id);
            }
            emit dataChanged(index, index, {to_underlying(DataRole::LikedPos)});
        }
    }
}

bool SpectrometerBridge::storeData(const QUrl &url)
{
    if (url.isEmpty())
    {
        return false;
    }

    unique_ptr<QIODevice> f;
    if (url.isLocalFile())
    {
        f = make_unique<QSaveFile>(url.toLocalFile());
    }
    else
    {
        f = make_unique<QFile>(url.toString());
    }
    if (!f->open(QFile::WriteOnly | QFile::Text))
    {
        return false;
    }

    f->write(QGuiApplication::applicationDisplayName().toLatin1() + ": " + QCoreApplication::applicationVersion().toLatin1() + "\n");
    for (const ModelEntry &entry : as_const(m_entryList))
    {
        if (entry.key == SpectrometerWorker::sDateKey)
        {
            if (m_spdData->date.isValid())
            {
                f->write(SpectrometerWorker::sDateKey.toLatin1() + ": " + m_spdData->date.toString(Qt::ISODate).toLatin1() + "\n");
            }
        }
        else if (entry.value != g_noData)
        {
            f->write(entry.key.toUtf8() + ": " + entry.value.toUtf8() + "\n");
        }
    }
    f->write("\n");
    for (auto &&[wavelength, irradiance] : as_const(m_spdData->spd))
    {
        f->write(QByteArray::number(wavelength) + "," + QByteArray::number(irradiance) + "\n");
    }

    if (url.isLocalFile())
    {
        return static_cast<QSaveFile *>(f.get())->commit();
    }
    return true;
}

void SpectrometerBridge::setDataInternalCache(bool cache)
{
    if (cache)
    {
        m_dataInternalCache = Application::instance()->measurementsDir().entryList({u"*.txt"_s}, QDir::Files);
    }
    else
    {
        m_dataInternalCache.clear();
        m_dataInternalCache.shrink_to_fit();
    }
}

bool SpectrometerBridge::canStoreDataInternal(const QString &name)
{
    const auto nameTrimmed = name.trimmed();
    if (nameTrimmed.isEmpty())
    {
        return false;
    }
    return !m_dataInternalCache.contains(sMeasurementFileName(nameTrimmed));
}
bool SpectrometerBridge::storeDataInternal(const QString &name)
{
    const auto nameTrimmed = name.trimmed();
    if (Q_UNLIKELY(nameTrimmed.isEmpty()))
    {
        return false;
    }
    if (!Application::instance()->measurementsDir().mkpath(u"."_s))
    {
        return false;
    }
    return storeData(QUrl::fromLocalFile(Application::instance()->measurementsDir().filePath(sMeasurementFileName(nameTrimmed))));
}
bool SpectrometerBridge::renameDataInternal(const QString &oldName, const QString &newName)
{
    const auto oldNameTrimmed = oldName.trimmed();
    const auto newNameTrimmed = newName.trimmed();
    if (Q_UNLIKELY(oldNameTrimmed.isEmpty() || newNameTrimmed.isEmpty()))
    {
        return false;
    }

    return QFile::rename(
        Application::instance()->measurementsDir().filePath(sMeasurementFileName(oldNameTrimmed)),
        Application::instance()->measurementsDir().filePath(sMeasurementFileName(newNameTrimmed))
    );
}

QVariantList SpectrometerBridge::getStoredMeasurements() const
{
    QVariantList ret;

    const auto list = Application::instance()->measurementsDir().entryInfoList({u"*.txt"_s}, QDir::Files);
    ret.reserve(list.size());

    for (auto &&entry : list)
    {
        const auto name = QByteArray::fromBase64(entry.baseName().toLatin1(), QByteArray::Base64UrlEncoding);
        if (!name.isEmpty())
        {
            const auto date = entry.fileTime(QFile::FileModificationTime);

            QVariantMap map;
            map[u"name"_s] = name;
            map[u"date"_s] = date.toString();
            map[u"path"_s] = entry.filePath();
            ret.emplace_back(std::move(map));
        }
    }

    ranges::stable_sort(ret, [](const QVariant &v1, const QVariant &v2) {
        return QDateTime::fromString(v1.value<QVariantMap>().value(u"date"_s).toString()) > QDateTime::fromString(v2.value<QVariantMap>().value(u"date"_s).toString());
    });

    return ret;
}

bool SpectrometerBridge::deleteStoredMeasurement(const QString &name)
{
    if (Q_UNLIKELY(name.isEmpty()))
    {
        return false;
    }
    return QFile::remove(Application::instance()->measurementsDir().filePath(sMeasurementFileName(name)));
}

int SpectrometerBridge::rowCount(const QModelIndex &parent) const
{
    return m_entryList.size();
}

QVariant SpectrometerBridge::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < static_cast<int>(m_entryList.size()))
    {
        const auto &entry = m_entryList[index.row()];
        switch (role)
        {
            case Qt::DisplayRole:
            {
                return QString(entry.key + u"\n\n"_s + entry.value);
            }
            case to_underlying(DataRole::Id):
            {
                return entry.id;
            }
            case to_underlying(DataRole::Key):
            {
                return entry.key;
            }
            case to_underlying(DataRole::Value):
            {
                return entry.value;
            }
            case to_underlying(DataRole::LikedPos):
            {
                return entry.likedPos;
            }
            case to_underlying(DataRole::Base):
            {
                return static_cast<bool>(entry.groups & Group::Base);
            }
            case to_underlying(DataRole::PPFD):
            {
                return static_cast<bool>(entry.groups & Group::PPFD);
            }
            case to_underlying(DataRole::BlueLight):
            {
                return static_cast<bool>(entry.groups & Group::BlueLight);
            }
        }
    }
    return QVariant();
}

QHash<int, QByteArray> SpectrometerBridge::roleNames() const
{
    return {
        {to_underlying(DataRole::Id), "id"_ba},
        {to_underlying(DataRole::Key), "key"_ba},
        {to_underlying(DataRole::Value), "value"_ba},
        {to_underlying(DataRole::Base), "base"_ba},
        {to_underlying(DataRole::PPFD), "ppfd"_ba},
        {to_underlying(DataRole::BlueLight), "bl"_ba},
    };
}

void SpectrometerBridge::onNewExposureData(const Exposure &exposure)
{
    if (m_exposure != exposure)
    {
        m_exposure = exposure;
        emitExposureChanged();
    }
}

void SpectrometerBridge::onNewSpdData(const shared_ptr<SpdData> &spdData)
{
    Q_ASSERT(m_entryListTmp.empty());

    auto emplace = [this](const QString &id, const QString &key, const QString &value, Groups groups) {
        m_entryListTmp.emplace_back(
            id,
            key,
            value,
            groups,
            m_likedSettings.value(id, -1)
        );
    };

    m_spdData = spdData;

    const auto uvCoarseDataSuffix = (spdData->minNm > 280.0) ? u"*"_s : QString();

    emplace(
        g_dev,
        SpectrometerWorker::sDeviceKey,
        m_spdData->deviceId.isEmpty() ? g_noData : m_spdData->deviceId,
        Group::None
    );
    emplace(
        g_date,
        SpectrometerWorker::sDateKey,
        m_spdData->date.isValid() ? m_spdData->date.toLocalTime().toString() : g_noData,
        Group::None
    );
    emplace(
        g_time,
        SpectrometerWorker::sExposureKey,
        m_exposure.time <= 0.0 ? g_noData : QString::number(m_exposure.time) + u" ms"_s,
        Group::None
    );

    emplace(
        g_ee,
        u"Irradiance (Ee)"_s,
        QString::number(spdData->ee) + u" W/m²"_s,
        Group::Base
    );
    emplace(
        g_lux,
        u"Illuminance"_s,
        QString::number(spdData->lux, 'f', 2) + u" lx"_s,
        Group::Base
    );

    emplace(
        g_cct,
        u"CCT"_s,
        QString(QString::number(spdData->cct, 'f', 0) + u" K"_s),
        Group::Base
    );
    emplace(
        g_duv,
        u"Duv"_s,
        QString::number(spdData->duv, 'f', 4),
        Group::Base
    );
    emplace(
        g_sdcm,
        u"SDCM"_s,
        QString::number(spdData->sdcm, 'f', 1),
        Group::Base
    );

    emplace(
        u"sp"_s,
        u"S/P ratio"_s,
        QString::number(spdData->spRatio, 'f', 3),
        Group::Base
    );

    emplace(
        g_ra,
        u"CRI Ra"_s,
        QString::number(spdData->Ra, 'f', 1),
        Group::Base
    );

    emplace(
        g_rf,
        u"TM-30 Rf"_s,
        QString::number(spdData->tm30Rf, 'f', 1),
        Group::Base
    );
    emplace(
        g_rg,
        u"TM-30 Rg"_s,
        QString::number(spdData->tm30Rg, 'f', 1),
        Group::Base
    );

    emplace(
        g_lp,
        u"Peak wavelength"_s,
        QString::number(spdData->peakWavelength) + u" nm"_s,
        Group::Base
    );
    emplace(
        g_ld,
        u"Dominant wavelength"_s,
        QString::number(spdData->dominantWavelength) + u" nm"_s,
        Group::Base
    );

    emplace(
        g_X,
        u"CIE1931 X"_s,
        QString::number(spdData->X, 'f', 5),
        Group::None
    );
    emplace(
        g_Y,
        u"CIE1931 Y"_s,
        QString::number(spdData->Y, 'f', 5),
        Group::None
    );
    emplace(
        g_Z,
        u"CIE1931 Z"_s,
        QString::number(spdData->Z, 'f', 5),
        Group::None
    );
    emplace(
        g_x,
        u"CIE1931 x"_s,
        QString::number(spdData->x, 'f', 5),
        Group::Base
    );
    emplace(
        g_y,
        u"CIE1931 y"_s,
        QString::number(spdData->y, 'f', 5),
        Group::Base
    );

    for (qsizetype i = 0; i < m_spdData->R.PreallocatedSize; ++i)
    {
        emplace(
            g_rx.arg(i),
            u"CRI R%1"_s.arg(i + 1),
            QString::number(spdData->R.value(i), 'f', 1),
            Group::None
        );
    }

    for (qsizetype i = 0; i < m_spdData->tm30Rfi.PreallocatedSize; ++i)
    {
        emplace(
            g_rfhx.arg(i),
            u"TM-30 Rf,h%1"_s.arg(i + 1),
            QString::number(spdData->tm30Rfi.value(i), 'f', 1),
            Group::None
        );
    }

    emplace(
        g_par,
        u"PAR"_s,
        QString::number(spdData->par) + u" W/m²"_s,
        Group::PPFD
    );
    emplace(
        g_ppfd,
        u"PPFD"_s,
        QString::number(spdData->ppfd) + u" μmol/(m²·s)"_s,
        Group::PPFD
    );

    emplace(
        g_eb,
        u"Irradiance (Eb)"_s,
        QString::number(spdData->eb) + u" W/m²"_s,
        Group::BlueLight
    );
    emplace(
        g_blr,
        u"Blue light ratio"_s,
        QString::number(spdData->blueRatio, 'f', 3),
        Group::BlueLight
    );

    emplace(
        g_mlux,
        u"Melanopic illuminance"_s,
        QString::number(spdData->melanopicLux, 'f', 2) + u" lx"_s,
        Group::BlueLight
    );
    emplace(
        g_mr,
        u"Melanopic ratio"_s,
        QString::number(spdData->melanopicRatio, 'f', 3),
        Group::BlueLight
    );
    emplace(
        g_mder,
        u"Melanopic daylight equivalent ratio"_s,
        QString::number(spdData->mder, 'f', 3),
        Group::BlueLight
    );

    emplace(
        g_uvi,
        u"UV Index"_s,
        QString::number(spdData->uvi, 'f', 1) + uvCoarseDataSuffix,
        Group::None
    );
    emplace(
        g_vitamind,
        u"Vitamin D"_s,
        QString::number(spdData->vitaminD * 1000.0, 'f', 0) + u" mW/m²"_s + uvCoarseDataSuffix,
        Group::None
    );

    if (m_entryListTmp.size() != m_entryList.size())
    {
        beginResetModel();
        swap(m_entryList, m_entryListTmp);
        endResetModel();
    }
    else if (!m_entryListTmp.empty())
    {
        swap(m_entryList, m_entryListTmp);
        emit dataChanged(index(0), index(m_entryList.size() - 1));
    }
    m_entryListTmp.clear();
}

void SpectrometerBridge::emitExposureChanged()
{
    emit exposureChanged(m_exposure.time, m_exposure.ae, m_exposure.aeInProgress, m_exposure.min, m_exposure.max);
}

QString SpectrometerBridge::sMeasurementFileName(const QString &name)
{
    return QString::fromLatin1(name.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals) + ".txt");
}
