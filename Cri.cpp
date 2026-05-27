// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "Cri.hpp"
#include "CriData.hpp"
#include "SpdData.hpp"
#include "CieXyz1931TwoDegData.hpp"
#include "CieDIlluminantData.hpp"
#include "BlackBody.hpp"
#include "DataSeqIterp.hpp"
#include "Helpers.hpp"

namespace Cri {

using namespace Helpers;

struct UV
{
    double u, v;
};

// Robertson 1968 CCT interpolation (CIE 1960 UCS, isotherm table)
// Table format: {mired, u, v, slope} (Wyszecki & Stiles). 325 mired u corrected (Lindbloom).
static constexpr struct { double mired; double u; double v; double slope; } robertson_isotherms[] = {
    {  0, 0.18006, 0.26352, -0.24341},
    { 10, 0.18066, 0.26589, -0.25479},
    { 20, 0.18133, 0.26846, -0.26876},
    { 30, 0.18208, 0.27119, -0.28539},
    { 40, 0.18293, 0.27407, -0.30470},
    { 50, 0.18388, 0.27709, -0.32675},
    { 60, 0.18494, 0.28021, -0.35156},
    { 70, 0.18611, 0.28342, -0.37915},
    { 80, 0.18740, 0.28668, -0.40955},
    { 90, 0.18880, 0.28997, -0.44278},
    {100, 0.19032, 0.29326, -0.47888},
    {125, 0.19462, 0.30141, -0.58204},
    {150, 0.19962, 0.30921, -0.70471},
    {175, 0.20525, 0.31647, -0.84901},
    {200, 0.21142, 0.32312, -1.0182},
    {225, 0.21807, 0.32909, -1.2168},
    {250, 0.22511, 0.33439, -1.4512},
    {275, 0.23247, 0.33904, -1.7298},
    {300, 0.24010, 0.34308, -2.0637},
    {325, 0.24792, 0.34655, -2.4681},
    {350, 0.25591, 0.34951, -2.9641},
    {375, 0.26400, 0.35200, -3.5814},
    {400, 0.27218, 0.35407, -4.3633},
    {425, 0.28039, 0.35577, -5.3762},
    {450, 0.28863, 0.35714, -6.7262},
    {475, 0.29685, 0.35823, -8.5955},
    {500, 0.30505, 0.35907, -11.324},
    {525, 0.31320, 0.35968, -15.628},
    {550, 0.32129, 0.36011, -23.325},
    {575, 0.32931, 0.36038, -40.770},
    {600, 0.33724, 0.36051, -116.45}
};
static constexpr int robertson_table_size = sizeof(robertson_isotherms) / sizeof(robertson_isotherms[0]);

// Convert mired (micro reciprocal degree) to CCT
static double miredToCCT(double mired)
{
    return 1.0e6 / mired;
}

// Convert CCT to mired
static double cctToMired(double cct)
{
    return 1.0e6 / cct;
}

// Robertson 1968 CCT calculation - used internally for CRI
static double computeCCT_Robertson(double u, double v)
{
    // Precompute normalized direction vectors for all isotherms
    array<double, robertson_table_size> du_itl, dv_itl;
    for (int i = 0; i < robertson_table_size; ++i)
    {
        const double t = robertson_isotherms[i].slope;
        const double length = hypot(1.0, t);
        du_itl[i] = 1.0 / length;
        dv_itl[i] = t / length;
    }

    // Find the crossing point (where distance changes sign)
    int idx = robertson_table_size - 1;
    for (int i = 1; i < robertson_table_size; ++i)
    {
        const double uu = u - robertson_isotherms[i].u;
        const double vv = v - robertson_isotherms[i].v;
        const double dt = -uu * dv_itl[i] + vv * du_itl[i];
        if (dt <= 0)
        {
            idx = i;
            break;
        }
    }

    // Compute distances at adjacent isotherms
    const double uu_curr = u - robertson_isotherms[idx].u;
    const double vv_curr = v - robertson_isotherms[idx].v;
    const double dt_curr = -uu_curr * dv_itl[idx] + vv_curr * du_itl[idx];

    const double uu_prev = u - robertson_isotherms[idx - 1].u;
    const double vv_prev = v - robertson_isotherms[idx - 1].v;
    const double dt_prev = -uu_prev * dv_itl[idx - 1] + vv_prev * du_itl[idx - 1];

    // Interpolation factor
    const double dt_current = -min(dt_curr, 0.0);
    const double f = (idx == 1) ? 0.0 : dt_current / (dt_prev + dt_current);

    // Interpolate temperature
    const double mired = robertson_isotherms[idx - 1].mired * f +
                         robertson_isotherms[idx].mired * (1.0 - f);
    return miredToCCT(mired);
}

// Interpolate TCS reflectance from 5 nm samples to 1 nm and apply the illuminant
static VisData tcsSPD(const VisData &illuminant, int tcsIndex)
{
#if 0 // Unavailable in libc++
    const auto rfl = views::zip(views::iota(380) | views::take(g_limitedVisSize) | views::stride(5), TCS[tcsIndex]);
#else
    constexpr int step = 5;
    const auto rfl = views::zip(views::iota(0, (g_limitedVisSize + step - 1) / step) | views::transform([](auto i){return 380 + (i * step);}), CriData::TCS[tcsIndex]);
#endif
    DataSeqIterp dsiRfl(rfl);
    return views::zip(views::iota(380), illuminant) | views::transform([&dsiRfl](auto &&zipped) {
        auto &&[wavelength, ill] = zipped;
        return dsiRfl.get(wavelength) * ill;
    }) | ranges::to<VisData>();
}

// Von Kries chromatic adaptation (CIE 13.3-1995): adapt sample chromaticity to the reference white
static UV vonKriesAdapt(UV tcs, UV test, UV ref)
{
    // Intermediate (c, d) coordinates per CIE 13.3-1995
    auto cd = [](UV uv) -> pair<double, double> {
        const double c = (4.0 - uv.u - 10.0 * uv.v) / uv.v;
        const double d = (1.708 * uv.v + 0.404 - 1.481 * uv.u) / uv.v;
        return {c, d};
    };

    const auto [cT, dT] = cd(test);
    const auto [cR, dR] = cd(ref);
    const auto [ci, di] = cd(tcs);

    const double ratio_c = cR / cT;
    const double ratio_d = dR / dT;

    // Adapted chromaticity in CIE 1960 UCS
    const double denom = 16.518 + 1.481 * ratio_c * ci - ratio_d * di;
    const double u_a   = (10.872 + 0.404 * ratio_c * ci - 4.0 * ratio_d * di) / denom;
    const double v_a   = 5.520 / denom;
    return {u_a, v_a};
}

// Convert values to CIE 1964 W*U*V* (legacy CRI colour space)

struct WUV { double W, U, V; };

static WUV toWUV(double Y, UV uv, UV white)
{
    const double W = 25.0 * cbrt(Y) - 17.0;
    const double U = 13.0 * W * (uv.u - white.u);
    const double V = 13.0 * W * (uv.v - white.v);
    return {W, U, V};
}

// CRI (CIE 1995): compute Ri/Ra using Robertson CCT, von Kries adaptation and W*U*V* ΔE
void compute(SpdData &spdData, const VisData &testSpd)
{
    // XYZ of test source and get CCT
    const XYZ xyzTest = CieXyz1931TwoDegData::getXyzFromSpdLimited(testSpd);
    if (xyzTest.Y <= 0.0)
    {
        return;
    }

    // Get (u, v) in CIE 1960 UCS
    const auto [u_test, v_test] = xyzToUV1960(xyzTest);

    // Compute CCT using Robertson 1968 method (for reference illuminant selection)
    // This matches the colour-science CRI implementation which uses Robertson 1968
    const auto CCT = computeCCT_Robertson(u_test, v_test);
    if (CCT < 1000.0 || CCT > 100000.0)
    {
        return;
    }

    // Reference illuminant
    const auto refSPD = (CCT < 5000.0) ? BlackBody::getViewLimited(CCT) | ranges::to<VisData>() : CieDIlluminantData::getSpd(CCT);

    // White-point (u, v) for chromatic adaptation
    const XYZ xyzRef = CieXyz1931TwoDegData::getXyzFromSpdLimited(refSPD);
    const auto [u_ref, v_ref] = xyzToUV1960(xyzRef);

    const UV uvTest = {u_test, v_test};
    const UV uvRef = {u_ref, v_ref};

    // For each TCS sample compute ΔE using W*U*V* and Ri
    // CIE 1995 method using von Kries chromatic adaptation

    for (int s = 0; s < CriData::TCS_COUNT; ++s)
    {
        // XYZ of TCS under test illuminant
        const auto tcsTestSPD = tcsSPD(testSpd, s);
        const XYZ xyzTi_raw = CieXyz1931TwoDegData::getXyzFromSpdLimited(tcsTestSPD);

        // Get (u, v) and Y for TCS under test illuminant
        const auto [u_tcs_test, v_tcs_test] = xyzToUV1960(xyzTi_raw);
        const UV uvTcsTest = {u_tcs_test, v_tcs_test};

        // Y relative to illuminant (luminance factor * 100)
        const double Y_test = xyzTi_raw.Y / xyzTest.Y * 100.0;

        // Von Kries chromatic adaptation
        const UV uvTcsAdapted = vonKriesAdapt(uvTcsTest, uvTest, uvRef);

        // Convert to W*U*V* (test sample, adapted)
        const WUV wuvTest = toWUV(Y_test, uvTcsAdapted, uvRef);

        // XYZ of TCS under reference illuminant
        const auto tcsRefSPD = tcsSPD(refSPD, s);
        const XYZ xyzRi_raw = CieXyz1931TwoDegData::getXyzFromSpdLimited(tcsRefSPD);

        // Get (u, v) and Y for TCS under reference illuminant
        const auto [u_tcs_ref, v_tcs_ref] = xyzToUV1960(xyzRi_raw);
        const UV uvTcsRef = {u_tcs_ref, v_tcs_ref};

        // Y relative to illuminant
        const double Y_ref = xyzRi_raw.Y / xyzRef.Y * 100.0;

        // Convert to W*U*V* (reference sample, no adaptation needed)
        const WUV wuvRef = toWUV(Y_ref, uvTcsRef, uvRef);

        // Euclidean distance in W*U*V* space (ΔE)
        const double dU = wuvTest.U - wuvRef.U;
        const double dV = wuvTest.V - wuvRef.V;
        const double dW = wuvTest.W - wuvRef.W;
        const double dE = sqrt(dU * dU + dV * dV + dW * dW);

        // Ri = 100 - 4.6 * dE (CIE 13.3-1995)
        spdData.R.push_back(100.0 - 4.6 * dE);
    }

    // Ra = mean of R1–R8
    spdData.Ra = ranges::fold_left(span(spdData.R.cbegin(), 8), 0.0, plus<double>()) / 8.0;
}

}
