// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "DuvGraph.hpp"
#include "Application.hpp"
#include "SpdData.hpp"
#include "CieXyz1931TwoDegData.hpp"
#include "BlackBody.hpp"

DuvGraph::DuvGraph()
{
    // Range: [380 - 700)
    for (auto cieXYZ : CieXyz1931TwoDegData::dataLimited() | views::take(320))
    {
        double X = cieXYZ.X;
        double Y = cieXYZ.Y;
        double Z = cieXYZ.Z;
        double sum = X + Y + Z;
        m_spectralLocus.append(QPointF(X / sum, 1.0 - (Y / sum)));
    }
    m_spectralLocus.append(m_spectralLocus.constFirst());

    for (int mired = 10; mired <= 1000; mired += 10)
    {
        double X = 0.0, Y = 0.0, Z = 0.0;

        const double t = 1e6 / mired;
        for (auto [wavelength, cieXYZ] : CieXyz1931TwoDegData::dataLimitedWithWavelength())
        {
            const double irradiance = BlackBody::get(wavelength, t);
            X += irradiance * cieXYZ.X;
            Y += irradiance * cieXYZ.Y;
            Z += irradiance * cieXYZ.Z;
        }

        const double sum = X + Y + Z;
        m_planckianLocus.append(QPointF(X / sum, 1.0 - (Y / sum)));
    }

    connect(Application::instance(), &Application::newSpdData, this, [this](const shared_ptr<SpdData> &spdData) {
        const bool hasData = (spdData->sumXYZ > 0.0);

        if (hasData)
        {
            m_currentPoint = QPointF(spdData->x, 1.0 - spdData->y);
        }
        else
        {
            m_currentPoint = QPointF();
        }

        if (m_valid || m_valid != hasData)
        {
            emit currentPointChanged();
        }

        if (m_valid != hasData)
        {
            m_valid = hasData;
            emit validChanged();
        }
    });
}
DuvGraph::~DuvGraph()
{
}

void DuvGraph::setSpectralLocus(QObject *pathPolyLine)
{
    pathPolyLine->setProperty("path", QVariant::fromValue(m_spectralLocus));
}
void DuvGraph::setPlanckianLocus(QObject *pathPolyLine)
{
    pathPolyLine->setProperty("path", QVariant::fromValue(m_planckianLocus));
}
