// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "Spectrum.hpp"
#include "Application.hpp"
#include "SpdData.hpp"

static const auto g_drawOutlineSettings = "Spectrum/DrawOutline";

Spectrum::Spectrum()
    : m_drawOutline(Application::instance()->settings().value(g_drawOutlineSettings, true).toBool())
{
    connect(Application::instance(), &Application::newSpdData, this, [this](const shared_ptr<SpdData> &spdData) {
        setData(spdData->minNm, spdData->maxNm, spdData->spd);
    });

    connect(this, &Spectrum::drawOutlineChanged, this, [this] {
        Application::instance()->settings().setValue(g_drawOutlineSettings, m_drawOutline);
        setDataInternal();
    });
}
Spectrum::~Spectrum()
{
}

void Spectrum::setPathObj(QObject *polyline, QObject *outlinePolyline)
{
    m_polyline = polyline;
    m_outlinePolyline = outlinePolyline;

    connect(m_polyline, &QObject::destroyed, this, [this] {
        m_polyline = nullptr;
    });
    connect(m_outlinePolyline, &QObject::destroyed, this, [this] {
        m_outlinePolyline = nullptr;
    });
}

QVariantMap Spectrum::getIrradianceAt(qreal nm) const
{
    qreal wl = 0.0;
    qreal irr = 0.0;
    for (auto [p1, p2] : views::zip(m_outlineData, m_outlineData | views::drop(1)))
    {
        const auto wlP1 = p1.x() + m_minNm;
        const auto wlP2 = p2.x() + m_minNm;
        const auto d1 = abs(wlP1 - nm);
        const auto d2 = abs(wlP2 - nm);
        if (d1 < d2)
        {
            wl = wlP1;
            irr = getIrradianceFromPoint(p1);
            break;
        }
        else if (wlP2 >= nm)
        {
            wl = wlP2;
            irr = getIrradianceFromPoint(p2);
            break;
        }
    }
    return QVariantMap{
        {u"wavelength"_s, wl},
        {u"irradiance"_s, irr},
    };
}

qreal Spectrum::getPeakIrradiance() const
{
    return m_maxIrr;
}

void Spectrum::setData(double minNm, double maxNm, const Data &data)
{
    if (m_minNm != minNm || m_maxNm != maxNm)
    {
        m_minNm = minNm;
        m_maxNm = maxNm;
        emit rangeChanged();
    }

    const qreal spanNm = m_maxNm - m_minNm;

    if (data.empty())
    {
        m_maxIrr = 0.0;
    }
    else
    {
        m_maxIrr = ranges::max_element(data, {}, &Entry::second)->second;
    }

    Q_ASSERT(m_data.isEmpty() || m_data.isDetached());
    Q_ASSERT(m_outlineData.isEmpty() || m_outlineData.isDetached());

    if (const qsizetype size = data.size() + 2; m_data.size() != size)
    {
        m_data.clear();
        m_data.resize(size);
    }
    if (const qsizetype size = data.size(); m_outlineData.size() != size)
    {
        m_outlineData.clear();
        m_outlineData.resize(size);
    }

    // Use value greater than 1.0 to close the path, because "CurveRenderer"
    // can't handle it otherwise and graph (fill) disappears at random.
    m_data.first() = QPointF(0.0, 1.0001);
    for (auto [dst, dstOutline, src] : views::zip(m_data | views::drop(1), m_outlineData, data))
    {
        dst = dstOutline = QPointF(src.first - m_minNm, (m_maxIrr > 0.0) ? 1.0 - src.second / m_maxIrr : 1.0);
    }
    m_data.last() = QPointF(spanNm, 1.0001);

    setDataInternal();
}

void Spectrum::setDataInternal()
{
    if (m_polyline && m_outlinePolyline)
    {
        m_polyline->setProperty("path", QVariant::fromValue(m_data));
        m_outlinePolyline->setProperty("path", QVariant::fromValue(m_drawOutline ? m_outlineData : QList<QPointF>()));
        emit dataUpdated();
    }
}

qreal Spectrum::getIrradianceFromPoint(const QPointF &p) const
{
    return (1.0 - p.y()) * m_maxIrr;
}
