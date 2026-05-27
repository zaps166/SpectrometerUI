// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "DaylightWorker.hpp"
#include "CieDIlluminantData.hpp"

DaylightWorker::DaylightWorker(QObject *parent)
    : SpectrometerWorker(parent)
{
    m_minNm = 380.0;
    m_maxNm = 780.0;
}
DaylightWorker::~DaylightWorker()
{
}

bool DaylightWorker::setParam(const QString &key, const QVariant &value)
{
    if (!isRunning())
    {
        if (key == u"cct"_sv)
        {
            m_cct = value.toDouble();
            return true;
        }
        else if (key == u"normalize"_sv)
        {
            m_normalize = value.toDouble();
            return true;
        }
    }
    return false;
}

QString DaylightWorker::staticDataName() const
{
    return u"Daylight "_sv + QString::number(m_cct) + u"K"_sv;
}

void DaylightWorker::run()
{
    auto selfDestroy = qScopeGuard([this] {
        deleteLater();
    });

    if (m_cct <= 0.0 || m_normalize <= 0.0)
    {
        return;
    }

    const auto spd = CieDIlluminantData::getSpd(m_cct);
    if (Q_UNLIKELY(spd.size() != g_limitedVisSize))
    {
        return;
    }

    Data data(g_limitedVisSize);

    for (auto [idx, entry] : views::zip(views::iota(0), data))
    {
        auto &&[wl, irr] = entry;
        wl = idx + m_minNm;
        irr = spd[idx];
    }

    const auto divider = ranges::max_element(data, {}, &Entry::second)->second / m_normalize;
    for (auto &&[wl, irr] : data)
    {
        irr /= divider;
    }

    setCurrentDate();
    processSpd(data);
}
