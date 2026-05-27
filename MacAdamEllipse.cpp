// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "MacAdamEllipse.hpp"

namespace MacAdam {

struct MacAdamEllipseData
{
    double cct_min;      // Minimum CCT for this ellipse model
    double cct_max;      // Maximum CCT for this ellipse model
    double duv_scale;    // SDCM = |Duv| / duv_scale (one step size in CIE 1960 Duv units)
    double orientation;  // Ellipse orientation in degrees (for reference)
};

// MacAdam step-size table by CCT band (used to convert Duv -> SDCM)
static constexpr array<MacAdamEllipseData, 8> g_macadamSteps = {{
    {1500, 2500, 0.00110, 35.0},     // Warm white (warm incandescent)
    {2500, 3500, 0.00111, 32.0},     // Warm white (3000K, incandescent-like)
    {3500, 4500, 0.00112, 28.0},     // Neutral white (3500-4000K)
    {4500, 5500, 0.00113, 23.0},     // Cool white (5000K)
    {5500, 6500, 0.00115, 18.0},     // Daylight (6500K)
    {6500, 7500, 0.00117, 14.0},     // Cool daylight
    {7500, 8500, 0.00118, 10.0},     // Very cool
    {8500, 100000, 0.00120, 5.0}     // Ultra-cool (for completeness)
}};

static double getMacadamStepSizeForCCT(double cct)
{
    if (cct < g_macadamSteps[0].cct_min)
    {
        return g_macadamSteps[0].duv_scale;
    }
    if (cct > g_macadamSteps.back().cct_max)
    {
        return g_macadamSteps.back().duv_scale;
    }

    // Find the matching CCT band and interpolate linearly within it.
    for (size_t i = 0; i < g_macadamSteps.size() - 1; ++i)
    {
        const auto &band = g_macadamSteps[i];
        if (cct >= band.cct_min && cct <= band.cct_max)
        {
            const auto &next = g_macadamSteps[i + 1];
            const double t = (cct - band.cct_min) / (band.cct_max - band.cct_min);
            return band.duv_scale + t * (next.duv_scale - band.duv_scale);
        }
    }

    return g_macadamSteps.back().duv_scale;
}

// Enhanced SDCM calculation using:
// 1. CCT-dependent MacAdam step size from ANSI C78.377 data
// 2. Perpendicular distance (Duv) from Planckian locus (Ohno 2013 method)
double calculateSdcm(double duv, double cct)
{
    if (cct < 1500.0 || cct > 100000.0)
        return 0.0;  // Invalid CCT range

    // Get the MacAdam step size for this color temperature
    const double step_size = getMacadamStepSizeForCCT(cct);

    // SDCM = |Duv| / step_size
    // Represents the number of MacAdam steps away from the Planckian locus
    const double sdcm = abs(duv) / step_size;

    return sdcm;
}

}
