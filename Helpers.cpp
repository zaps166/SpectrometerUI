// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "Helpers.hpp"
#include "CieXyz1931TwoDegData.hpp"

namespace Helpers {

UV1960 xyzToUV1960(const XYZ &xyz)
{
    const double denom = xyz.X + 15.0 * xyz.Y + 3.0 * xyz.Z;
    if (qFuzzyIsNull(denom))
    {
        return {0.0, 0.0};
    }
    return {4.0 * xyz.X / denom, 6.0 * xyz.Y / denom};
}

double dominantWavelength(const XYZ &xyz)
{
    constexpr double xw = 1.0 / 3.0;
    constexpr double yw = 1.0 / 3.0;

    const double sum = xyz.X + xyz.Y + xyz.Z;
    if (qFuzzyIsNull(sum))
    {
        return 0.0;
    }

    const double x = xyz.X / sum;
    const double y = xyz.Y / sum;
    const double targetAngle = atan2(y - yw, x - xw);

    double minDiff = numeric_limits<double>::max();
    double dominant = 0.0;

    for (auto [wavelength, locus] : CieXyz1931TwoDegData::dataWithWavelength())
    {
        const double locusSum = locus.X + locus.Y + locus.Z;
        if (qFuzzyIsNull(locusSum))
        {
            continue;
        }

        const double locusX = locus.X / locusSum;
        const double locusY = locus.Y / locusSum;

        const double locusAngle = atan2(locusY - yw, locusX - xw);
        double diff = abs(targetAngle - locusAngle);
        if (diff > numbers::pi)
            diff = 2.0 * numbers::pi - diff;

        if (diff < minDiff)
        {
            minDiff = diff;
            dominant = wavelength;
        }
    }

    return dominant;
}

}
