// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "SpectrometerWorker.hpp"
#include "SpdData.hpp"
#include "Helpers.hpp"
#include "Cri.hpp"
#include "Tm30.hpp"
#include "Ohno2013.hpp"
#include "DataSeqIterp.hpp"
#include "BlueLightHazardData.hpp"
#include "LuminousEfficiencyData.hpp"
#include "MelanopicActionData.hpp"
#include "ErythemalActionData.hpp"
#include "VitaminDActionData.hpp"
#include "CieXyz1931TwoDegData.hpp"
#include "ScotopicEfficiencyData.hpp"
#include "MacAdamEllipse.hpp"

const QString SpectrometerWorker::sDeviceKey = u"Device"_s;
const QString SpectrometerWorker::sDateKey = u"Date"_s;
const QString SpectrometerWorker::sExposureKey = u"Exposure"_s;

SpectrometerWorker::SpectrometerWorker(QObject *parent)
    : QThread(parent)
{
}
SpectrometerWorker::~SpectrometerWorker()
{
}

void SpectrometerWorker::begin()
{
    emit newSpdData(make_shared<SpdData>(), QPrivateSignal());
    start();
}

void SpectrometerWorker::setCurrentDate()
{
    m_date = QDateTime::currentDateTimeUtc();
}
void SpectrometerWorker::emitNewExposureData()
{
    emit newExposureData(m_exposure);
}

void SpectrometerWorker::processSpd(const Data &spd)
{
    constexpr double luxMultiplier = 683.002;
    constexpr double scotopicMultiplier = 1700.06;
    constexpr double melanopicLuxMultiplier = 1.1038848;
    constexpr double uviMultiplier = 40.0;

    auto minNm = m_hintMinNm > 0.0 ? max(m_minNm, m_hintMinNm) : m_minNm;
    auto maxNm = m_hintMaxNm > 0.0 ? min(m_maxNm, m_hintMaxNm) : m_maxNm;
    if (Q_UNLIKELY(maxNm - minNm < 1))
    {
        minNm = m_minNm;
        maxNm = m_maxNm;
    }

    auto spdData = make_shared<SpdData>();

    if (minNm == m_minNm && maxNm == m_maxNm)
    {
        spdData->spd = spd;
    }
    else
    {
        const auto beginIt = ranges::find_if(spd, [minNm](const Entry &entry) {
            return entry.first >= minNm;
        });
        const auto endIt = ranges::find_if(spd | views::reverse, [maxNm](const Entry &entry) {
            return entry.first <= maxNm;
        });

        if (beginIt == ranges::end(spd) || endIt == ranges::rend(spd))
        {
            spdData->spd.clear();
        }
        else
        {
            const auto dropCount = static_cast<qsizetype>(ranges::distance(ranges::begin(spd), beginIt));
            const auto takeCount = static_cast<qsizetype>(ranges::distance(beginIt, endIt.base()));
            spdData->spd = spd | views::drop(dropCount) | views::take(takeCount) | ranges::to<Data>();
            minNm = spdData->spd.constFirst().first;
            maxNm = spdData->spd.constLast().first;
        }
    }

    spdData->date = m_date;
    spdData->deviceId = m_deviceId;

    spdData->minNm = minNm;
    spdData->maxNm = maxNm;

    if (spdData->spd.empty())
    {
        emit newSpdData(spdData, QPrivateSignal());
        return;
    }

    spdData->ee = ranges::fold_left(spdData->spd | views::values, 0.0, plus<double>());
    spdData->peakWavelength = ranges::max_element(spdData->spd, {}, &Entry::second)->first;

    {
        DataSeqIterp dsi(spdData->spd);

        auto getNormalizedIrradiance = [&dsi](const double wavelength, const double step = 1.0) {
            return dsi.get(wavelength) * step;
        };

        for (auto [wavelength, cieXYZ] : CieXyz1931TwoDegData::dataWithWavelength())
        {
            const auto normalizedIrradiance = getNormalizedIrradiance(wavelength);
            const double irr = dsi.get(wavelength);
            spdData->X += normalizedIrradiance * cieXYZ.X;
            spdData->Y += normalizedIrradiance * cieXYZ.Y;
            spdData->Z += normalizedIrradiance * cieXYZ.Z;
        }
        spdData->sumXYZ = spdData->X + spdData->Y + spdData->Z;
        if (!qFuzzyIsNull(spdData->sumXYZ))
        {
            spdData->x = spdData->X / spdData->sumXYZ;
            spdData->y = spdData->Y / spdData->sumXYZ;

            spdData->dominantWavelength = Helpers::dominantWavelength(XYZ{spdData->X, spdData->Y, spdData->Z});
        }
        dsi.reset();

        for (auto &&[wavelength, multiplier] : LuminousEfficiency::data)
        {
            spdData->luminousEfficiency += getNormalizedIrradiance(wavelength, LuminousEfficiency::step) * multiplier;
        }
        spdData->lux = spdData->luminousEfficiency * luxMultiplier;
        dsi.reset();

        for (auto &&[wavelength, multiplier] : ScotopicEfficiency::data)
        {
            spdData->scotopic += getNormalizedIrradiance(wavelength, ScotopicEfficiency::step) * multiplier;
        }
        spdData->scotopic *= scotopicMultiplier;
        dsi.reset();

        for (double wavelength : views::iota(400, 700 + 1))
        {
            constexpr double hcNa = 119.626565582;
            const double irradiance = getNormalizedIrradiance(wavelength);
            spdData->par += irradiance;
            spdData->ppfd += (irradiance * wavelength) / hcNa;
        }
        dsi.reset();

        for (auto &&[wavelength, multiplier] : BlueLightHazard::data)
        {
            spdData->eb += getNormalizedIrradiance(wavelength, BlueLightHazard::step) * multiplier;
        }
        dsi.reset();

        for (auto &&[wavelength, multiplier] : MelanopicAction::data)
        {
            spdData->melanopic += getNormalizedIrradiance(wavelength, MelanopicAction::step) * multiplier;
        }
        spdData->melanopicLux = spdData->melanopic * luxMultiplier * melanopicLuxMultiplier;
        dsi.reset();

        for (auto &&[wavelength, multiplier] : ErythemalAction::data)
        {
            spdData->uvi += getNormalizedIrradiance(wavelength, ErythemalAction::step) * multiplier;
        }
        spdData->uvi *= uviMultiplier;
        dsi.reset();

        for (auto &&[wavelength, multiplier] : VitaminDAction::data)
        {
            spdData->vitaminD += getNormalizedIrradiance(wavelength, VitaminDAction::step) * multiplier;
        }
        dsi.reset();

        const auto visData = views::iota(380) | views::take(g_limitedVisSize) | views::transform([&dsi](double wavelength) {
            return dsi.get(wavelength);
        }) | ranges::to<VisData>();
        dsi.reset();

        tie(spdData->cct, spdData->duv) = Ohno2013::computeCCT(visData);

        spdData->sdcm = MacAdam::calculateSdcm(spdData->duv, spdData->cct);

        if (spdData->cct >= 1000.0)
        {
            Cri::compute(*spdData, visData);
            Tm30::compute(*spdData, visData);
        }
    }

    if (!qFuzzyIsNull(spdData->luminousEfficiency))
    {
        spdData->melanopicRatio = spdData->melanopic / spdData->luminousEfficiency;
        spdData->blueRatio = spdData->eb / spdData->luminousEfficiency;
    }
    if (!qFuzzyIsNull(spdData->lux))
    {
        spdData->spRatio = spdData->scotopic / spdData->lux;
        spdData->mder = spdData->melanopicLux / spdData->lux;
    }

    emit newSpdData(spdData, QPrivateSignal());
}
