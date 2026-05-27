// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "Tm30.hpp"
#include "Tm30Data.hpp"
#include "SpdData.hpp"
#include "CieXyz1931TwoDegData.hpp"
#include "CieXyz1964TenDegData.hpp"
#include "CieDIlluminantData.hpp"
#include "BlackBody.hpp"
#include "DataSeqIterp.hpp"

namespace Tm30 {

// TM-30 (IES TM-30-18): colour fidelity (Rf) and gamut (Rg) calculations.
// Reference illuminant: Planckian <4000K, blend 4000–5000K, D >5000K.
// Uses CIE 1964 (10°), CIECAM02 and CAM02-UCS to compute Rf and Rg.

// XYZ using CIE 1964 10-degree observer (for colorimetry)
static XYZ spectrumToXYZ_10deg(const VisData &spd)
{
    XYZ xyz;
    for (auto [irradiance, cieXYZ] : views::zip(spd, CieXyz1964TenDegData::dataLimited()))
    {
        xyz.X += irradiance * cieXYZ.X;
        xyz.Y += irradiance * cieXYZ.Y;
        xyz.Z += irradiance * cieXYZ.Z;
    }
    return xyz;
}

// Reference illuminant selection per TM-30-18 (Planckian / blended / D-series)
static VisData blackBodySpectrum(double T)
{
    VisData spd(g_limitedVisSize);
    for (auto [wavelength, irradiance] : views::zip(views::iota(380), spd))
    {
        irradiance = BlackBody::get(wavelength, T);
    }
    return spd;
}

static double computeY_10deg(const VisData &spd)
{
    double Y = 0.0;
    for (auto [irradiance, cieXYZ] : views::zip(spd, CieXyz1964TenDegData::dataLimited()))
    {
        Y += irradiance * cieXYZ.Y;
    }
    return Y;
}

// Reference illuminant per CIE 2017 / TM-30-18:
// < 4000K: Planckian
// 4000-5000K: linear blend of normalized Planckian and D-illuminant
// > 5000K: D-illuminant
static VisData referenceIlluminant(double CCT)
{
    if (CCT < 4000.0)
    {
        return blackBodySpectrum(CCT);
    }
    else if (CCT <= 5000.0)
    {
        auto planckian = blackBodySpectrum(CCT);
        auto daylight = CieDIlluminantData::getSpd(CCT);

        // Normalize both to Y=1 using the 10° observer
        const double Y_p = computeY_10deg(planckian);
        const double Y_d = computeY_10deg(daylight);

        if (Y_p > 0.0)
        {
            for (auto &v : planckian)
            {
                v /= Y_p;
            }
        }
        if (Y_d > 0.0)
        {
            for (auto &v : daylight)
            {
                v /= Y_d;
            }
        }

        // Linear blend: m=0 at 4000K, m=1 at 5000K
        const double m = (CCT - 4000.0) / 1000.0;
        VisData result(g_limitedVisSize);
        for (qsizetype i = 0; i < g_limitedVisSize; ++i)
            result[i] = (1.0 - m) * planckian[i] + m * daylight[i];
        return result;
    }
    else
    {
        return CieDIlluminantData::getSpd(CCT);
    }
}

// CIECAM02 forward model and environment computation

// CIECAM02 viewing conditions: F=1.0, c=0.69, Nc=1.0 (average surround)
// L_A=100, Y_b=20, discount_illuminant=true

struct CIECAM02_Result { double J, M, h; };

// CAT02 matrix (forward)
static constexpr double M_CAT02[3][3] = {
    { 0.7328,  0.4296, -0.1624},
    {-0.7036,  1.6975,  0.0061},
    { 0.0030,  0.0136,  0.9834}
};

// Inverse CAT02
static constexpr double M_CAT02_INV[3][3] = {
    { 1.096124, -0.278869,  0.182745},
    { 0.454369,  0.473533,  0.072098},
    {-0.009628, -0.005698,  1.015326}
};

// Hunt-Pointer-Estevez matrix
static constexpr double M_HPE[3][3] = {
    { 0.38971,  0.68898, -0.07868},
    {-0.22981,  1.18340,  0.04641},
    { 0.00000,  0.00000,  1.00000}
};

static void matMul3(const double M[3][3], double x, double y, double z, double &rx, double &ry, double &rz)
{
    rx = M[0][0] * x + M[0][1] * y + M[0][2] * z;
    ry = M[1][0] * x + M[1][1] * y + M[1][2] * z;
    rz = M[2][0] * x + M[2][1] * y + M[2][2] * z;
}

struct CIECAM02_Env
{
    double Aw, Nbb, z, n, Nc, c, FL;
    double D_R, D_G, D_B; // degree of adaptation per channel
};

static CIECAM02_Env computeEnv(XYZ xyzW)
{
    constexpr double F = 1.0;
    constexpr double c = 0.69;
    constexpr double Nc = 1.0;
    constexpr double L_A = 100.0;
    constexpr double Y_b = 20.0;
    constexpr bool discountIlluminant = true;

    // Adaptation degree
    double D;
    if constexpr (discountIlluminant)
        D = 1.0;
    else
        D = F * (1.0 - (1.0 / 3.6) * exp((-L_A - 42.0) / 92.0));

    // Background and chromatic induction factors
    const double k = 1.0 / (5.0 * L_A + 1.0);
    const double k4 = k * k * k * k;
    const double FL = 0.2 * k4 * (5.0 * L_A) + 0.1 * pow(1.0 - k4, 2.0) * cbrt(5.0 * L_A);

    const double n = Y_b / xyzW.Y;
    const double Nbb = 0.725 * pow(1.0 / n, 0.2);
    const double z = 1.48 + sqrt(n);

    // CAT02 transform of white
    double Rw, Gw, Bw;
    matMul3(M_CAT02, xyzW.X, xyzW.Y, xyzW.Z, Rw, Gw, Bw);

    // Degree of adaptation per channel
    const double D_R = D * (xyzW.Y / Rw) + 1.0 - D;
    const double D_G = D * (xyzW.Y / Gw) + 1.0 - D;
    const double D_B = D * (xyzW.Y / Bw) + 1.0 - D;

    // Adapted white in CAT02
    const double Rc = D_R * Rw;
    const double Gc = D_G * Gw;
    const double Bc = D_B * Bw;

    // Convert to Hunt-Pointer-Estevez
    double Rw2, Gw2, Bw2;
    matMul3(M_CAT02_INV, Rc, Gc, Bc, Rw2, Gw2, Bw2);
    double Rwp, Gwp, Bwp;
    matMul3(M_HPE, Rw2, Gw2, Bw2, Rwp, Gwp, Bwp);

    // Non-linear adaptation
    auto adapt = [FL](double x) -> double {
        const double xAbs = abs(x);
        const double p = pow(FL * xAbs / 100.0, 0.42);
        return copysign(400.0 * p / (27.13 + p) + 0.1, x);
    };

    const double Rwpa = adapt(Rwp);
    const double Gwpa = adapt(Gwp);
    const double Bwpa = adapt(Bwp);

    // Achromatic response for white
    const double Aw = (2.0 * Rwpa + Gwpa + (1.0 / 20.0) * Bwpa - 0.305) * Nbb;

    return {Aw, Nbb, z, n, Nc, c, FL, D_R, D_G, D_B};
}

static CIECAM02_Result xyzToCIECAM02(XYZ xyz, const CIECAM02_Env &env)
{
    // CAT02 transform
    double R, G, B;
    matMul3(M_CAT02, xyz.X, xyz.Y, xyz.Z, R, G, B);

    // Chromatic adaptation
    const double Rc = env.D_R * R;
    const double Gc = env.D_G * G;
    const double Bc = env.D_B * B;

    // Convert to HPE
    double R2, G2, B2;
    matMul3(M_CAT02_INV, Rc, Gc, Bc, R2, G2, B2);
    double Rp, Gp, Bp;
    matMul3(M_HPE, R2, G2, B2, Rp, Gp, Bp);

    // Non-linear adaptation
    auto adapt = [&env](double x) -> double {
        const double xAbs = abs(x);
        const double p = pow(env.FL * xAbs / 100.0, 0.42);
        return copysign(400.0 * p / (27.13 + p) + 0.1, x);
    };

    const double Rpa = adapt(Rp);
    const double Gpa = adapt(Gp);
    const double Bpa = adapt(Bp);

    // Opponent colour dimensions
    const double a = Rpa - 12.0 * Gpa / 11.0 + Bpa / 11.0;
    const double b = (1.0 / 9.0) * (Rpa + Gpa - 2.0 * Bpa);

    // Hue angle
    double h = atan2(b, a) * 180.0 / numbers::pi;
    if (h < 0.0) h += 360.0;

    // Eccentricity factor and hue composition (not needed for J, M)
    const double hr = h * numbers::pi / 180.0;
    const double et = 0.25 * (cos(hr + 2.0) + 3.8);

    // Achromatic response
    const double A = (2.0 * Rpa + Gpa + (1.0 / 20.0) * Bpa - 0.305) * env.Nbb;

    // Lightness
    const double J = 100.0 * pow(A / env.Aw, env.c * env.z);

    // Chroma
    const double t = (50000.0 / 13.0) * env.Nc * env.Nbb * et * sqrt(a * a + b * b) /
                     (Rpa + Gpa + 21.0 * Bpa / 20.0);

    const double C = pow(t, 0.9) * sqrt(J / 100.0) *
                     pow(1.64 - pow(0.29, env.n), 0.73);

    // Colourfulness
    const double M = C * pow(env.FL, 0.25);

    return {J, M, h};
}

// CAM02-UCS (J',a',b') conversion helper

struct Jpapbp { double Jp, ap, bp; };

static Jpapbp JMh_to_CAM02UCS(double J, double M, double h)
{
    // CAM02-UCS coefficients: KL=1.0, c1=0.007, c2=0.0228
    constexpr double c1 = 0.007;
    constexpr double c2 = 0.0228;

    const double Jp = ((1.0 + 100.0 * c1) * J) / (1.0 + c1 * J);
    const double Mp = (1.0 / c2) * log1p(c2 * M);

    const double hr = h * numbers::pi / 180.0;
    const double ap = Mp * cos(hr);
    const double bp = Mp * sin(hr);

    return {Jp, ap, bp};
}

// Interpolate CES reflectances from 5nm to 1nm and apply the illuminant

static VisData cesSPD(const VisData &illuminant, int cesIndex)
{
    // CES data is at 5nm (380-780); interpolate to 1nm and multiply by illuminant
    constexpr int step = 5;
    const auto rfl = views::zip(
        views::iota(0, (g_limitedVisSize + step - 1) / step) | views::transform([](auto i){return 380 + (i * step);}),
        Tm30Data::CES[cesIndex]
    );
    DataSeqIterp dsiRfl(rfl);
    return views::zip(views::iota(380), illuminant) | views::transform([&dsiRfl](auto &&zipped) {
        auto &&[wavelength, ill] = zipped;
        return dsiRfl.get(wavelength) * ill;
    }) | ranges::to<VisData>();
}

// Convert CAM02-UCS ΔE to Rf (fidelity index)

static double deltaE_to_Rf(double deltaE)
{
    // Rf = 10 * ln(1 + exp((100 - 6.73 * deltaE) / 10))
    constexpr double cf = 6.73;
    return 10.0 * log1p(exp((100.0 - cf * deltaE) / 10.0));
}

// Compute Rg (gamut index) from 16-bin a'b' polygon area ratio

static double polygonArea(const array<pair<double, double>, 16> &pts)
{
    // Shoelace formula for polygon area
    double area = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        const int j = (i + 1) % 16;
        area += pts[i].first * pts[j].second;
        area -= pts[j].first * pts[i].second;
    }
    return abs(area) * 0.5;
}

void compute(SpdData &spdData, const VisData &testSpd)
{
    const double CCT = spdData.cct;

    auto refSpd = referenceIlluminant(CCT);

    const double Y_test = computeY_10deg(testSpd);
    const double Y_ref = computeY_10deg(refSpd);
    if (Y_test <= 0.0 || Q_UNLIKELY(Y_ref <= 0.0))
    {
        return;
    }

    const double k_test = 100.0 / Y_test;
    const double k_ref = 100.0 / Y_ref;

    VisData testNorm(g_limitedVisSize);
    for (qsizetype i = 0; i < g_limitedVisSize; ++i)
    {
        testNorm[i] = testSpd[i] * k_test;
    }

    for (auto &v : refSpd)
    {
        v *= k_ref;
    }

    // White point XYZ (10-degree) for CIECAM02
    const XYZ xyzW_test = spectrumToXYZ_10deg(testNorm);
    const XYZ xyzW_ref = spectrumToXYZ_10deg(refSpd);

    const CIECAM02_Env envTest = computeEnv(xyzW_test);
    const CIECAM02_Env envRef = computeEnv(xyzW_ref);

    // For each CES, compute XYZ under test & reference, then CIECAM02 → CAM02-UCS
    double sumDeltaE = 0.0;

    array<vector<pair<double, double>>, 16> binTest, binRef;
    array<double, 16> binSumDeltaE = {};
    array<int, 16> binCount = {};
    for (int s = 0; s < Tm30Data::CES_COUNT; ++s)
    {
        // CES under test illuminant
        const auto cesTestSpd = cesSPD(testNorm, s);
        const XYZ xyzTest = spectrumToXYZ_10deg(cesTestSpd);
        const CIECAM02_Result camTest = xyzToCIECAM02(xyzTest, envTest);
        const Jpapbp ucsTest = JMh_to_CAM02UCS(camTest.J, camTest.M, camTest.h);

        // CES under reference illuminant
        const auto cesRefSpd = cesSPD(refSpd, s);
        const XYZ xyzRef = spectrumToXYZ_10deg(cesRefSpd);
        const CIECAM02_Result camRef = xyzToCIECAM02(xyzRef, envRef);
        const Jpapbp ucsRef = JMh_to_CAM02UCS(camRef.J, camRef.M, camRef.h);

        // ΔE in CAM02-UCS
        const double dJp = ucsTest.Jp - ucsRef.Jp;
        const double dap = ucsTest.ap - ucsRef.ap;
        const double dbp = ucsTest.bp - ucsRef.bp;
        const double dE = sqrt(dJp * dJp + dap * dap + dbp * dbp);
        sumDeltaE += dE;

        const double refHue = atan2(ucsRef.bp, ucsRef.ap);
        const double hue = (refHue < 0.0) ? (refHue + 2.0 * numbers::pi) : refHue;
        const int bin = static_cast<int>(floor(hue / (numbers::pi / 8.0))) % 16;
        binSumDeltaE[bin] += dE;
        binCount[bin]++;
        binTest[bin].emplace_back(ucsTest.ap, ucsTest.bp);
        binRef[bin].emplace_back(ucsRef.ap, ucsRef.bp);
    }

    // Compute Rf (overall and per-bin)
    const double meanDeltaE = sumDeltaE / Tm30Data::CES_COUNT;
    spdData.tm30Rf = deltaE_to_Rf(meanDeltaE);

    spdData.tm30Rfi.resize(16);
    for (int bin = 0; bin < 16; ++bin)
    {
        const double binMeanDeltaE = (binCount[bin] > 0) ? binSumDeltaE[bin] / binCount[bin] : 0.0;
        spdData.tm30Rfi[bin] = deltaE_to_Rf(binMeanDeltaE);
    }

    // Compute Rg (Gamut Index)
    // Average a'b' per bin for test and reference
    array<pair<double, double>, 16> avgTest, avgRef;
    for (int bin = 0; bin < 16; ++bin)
    {
        double sumA_t = 0, sumB_t = 0, sumA_r = 0, sumB_r = 0;
        int n_t = static_cast<int>(binTest[bin].size());
        int n_r = static_cast<int>(binRef[bin].size());

        for (auto &[a, b] : binTest[bin]) { sumA_t += a; sumB_t += b; }
        for (auto &[a, b] : binRef[bin]) { sumA_r += a; sumB_r += b; }

        avgTest[bin] = (n_t > 0) ? pair{sumA_t / n_t, sumB_t / n_t} : pair{0.0, 0.0};
        avgRef[bin] = (n_r > 0) ? pair{sumA_r / n_r, sumB_r / n_r} : pair{0.0, 0.0};
    }

    const double areaTest = polygonArea(avgTest);
    const double areaRef = polygonArea(avgRef);

    spdData.tm30Rg = (areaRef > 0.0) ? 100.0 * areaTest / areaRef : 0.0;
}

}
