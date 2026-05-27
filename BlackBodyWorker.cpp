// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "BlackBodyWorker.hpp"
#include "BlackBody.hpp"

BlackBodyWorker::BlackBodyWorker(QObject *parent)
    : SpectrometerWorker(parent)
{
}
BlackBodyWorker::~BlackBodyWorker()
{
}

bool BlackBodyWorker::setParam(const QString &key, const QVariant &value)
{
    if (!isRunning())
    {
        if (key == u"temperature"_sv)
        {
            m_temperature = value.toDouble();
            return true;
        }
        else if (key == u"min"_sv)
        {
            m_minNm = value.toInt();
            return true;
        }
        else if (key == u"max"_sv)
        {
            m_maxNm = value.toInt();
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

QString BlackBodyWorker::staticDataName() const
{
    return u"Black-body radiation "_sv + QString::number(m_temperature) + u"K"_sv;
}

void BlackBodyWorker::run()
{
    auto selfDestroy = qScopeGuard([this] {
        deleteLater();
    });

    if (m_temperature <= 0.0 || m_minNm < 1.0 || m_maxNm <= m_minNm || m_normalize <= 0.0)
    {
        return;
    }

    Data data(m_maxNm - m_minNm + 1);

    for (auto [idx, entry] : views::zip(views::iota(0), data))
    {
        auto &&[wl, irr] = entry;
        wl = idx + m_minNm;
        irr = BlackBody::get(wl, m_temperature);
    }

    const auto divider = ranges::max_element(data, {}, &Entry::second)->second / m_normalize;
    for (auto &&[wl, irr] : data)
    {
        irr /= divider;
    }

    setCurrentDate();
    processSpd(data);
}
