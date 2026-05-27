// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "Ohno2013.hpp"
#include "BlackBody.hpp"
#include "CieXyz1931TwoDegData.hpp"
#include "Helpers.hpp"

namespace Ohno2013 {

using namespace Helpers;

// Ohno (2013) correlated colour temperature and Duv calculation

constexpr double T_start = 500.0;
constexpr double T_end = 100000.0;
constexpr double spacing = 1.001;

// Cached Planckian locus table (temperature -> UV in CIE 1960 UCS)
struct LocusEntry
{
    double T;
    UV1960 uv;
};

static vector<double> buildTemperatures()
{
    vector<double> temperatures;
    temperatures.push_back(T_start);
    temperatures.push_back(T_start + 1.0);

    double nextTi = T_start + 1.0;
    double nextSpacing = spacing;
    while ((nextTi = nextTi * nextSpacing) < T_end)
    {
        temperatures.push_back(nextTi);

        const double D = clamp((nextTi - T_start) / (T_end - T_start), 0.0, 1.0);
        nextSpacing = spacing * (1.0 - D) + (1.0 + (spacing - 1.0) / 10.0) * D;
    }

    temperatures.push_back(T_end - 1.0);
    temperatures.push_back(T_end);
    return temperatures;
}
static vector<LocusEntry> buildLocusTable()
{
    vector<LocusEntry> table;
    const auto temperatures = buildTemperatures();
    table.reserve(temperatures.size());

    for (const double T : temperatures)
    {
        table.emplace_back(T, xyzToUV1960(CieXyz1931TwoDegData::getXyzFromSpd(BlackBody::getView(360, 830, T))));
    }

    return table;
}

static vector<LocusEntry> g_locusTable;

static thread g_locusTableInitThr = thread([] {
    g_locusTable = buildLocusTable();
});
static optional g_locusTableInitThrJoin = qScopeGuard([] {
    if (g_locusTableInitThr.joinable())
    {
        g_locusTableInitThr.join();
    }
});

pair<double, double> computeCCT(const VisData &spd)
{
    g_locusTableInitThrJoin.reset();

    const auto &table = g_locusTable;
    if (table.size() < 3)
    {
        return {0.0, 0.0};
    }

    const XYZ xyz = CieXyz1931TwoDegData::getXyzFromSpdLimited(spd);
    if (qFuzzyIsNull(xyz.X) && qFuzzyIsNull(xyz.Y) && qFuzzyIsNull(xyz.Z))
    {
        return {0.0, 0.0};
    }

    const UV1960 uv = xyzToUV1960(xyz);
    const qsizetype N = static_cast<qsizetype>(table.size());

    // Find the nearest locus sample.
    double minDist = numeric_limits<double>::max();
    qsizetype minIdx = 0;

    for (qsizetype i = 0; i < N; ++i)
    {
        const double du = uv.u - table[i].uv.u;
        const double dv = uv.v - table[i].uv.v;
        const double dist = hypot(du, dv);
        if (dist < minDist)
        {
            minDist = dist;
            minIdx = i;
        }
    }

    // Keep neighbors available for interpolation.
    minIdx = clamp<qsizetype>(minIdx, 1, N - 2);

    const auto &prev = table[minIdx - 1];
    const auto &curr = table[minIdx];
    const auto &next = table[minIdx + 1];

    const double dPrev = hypot(uv.u - prev.uv.u, uv.v - prev.uv.v);
    const double dCurr = hypot(uv.u - curr.uv.u, uv.v - curr.uv.v);
    const double dNext = hypot(uv.u - next.uv.u, uv.v - next.uv.v);

    const double l = hypot(next.uv.u - prev.uv.u, next.uv.v - prev.uv.v);
    if (qFuzzyIsNull(l))
    {
        return {0.0, 0.0};
    }

    const double x = (dPrev * dPrev - dNext * dNext + l * l) / (2.0 * l);
    const double Tt = prev.T + (next.T - prev.T) * (x / l);

    const double vtx = prev.uv.v + (next.uv.v - prev.uv.v) * (x / l);
    const double sign = (uv.v > vtx) - (uv.v < vtx);
    const double DuvT = sqrt(max(0.0, dPrev * dPrev - x * x)) * sign;

    const double X = (next.T - curr.T) * (prev.T - next.T) * (curr.T - prev.T);
    if (qFuzzyIsNull(X))
    {
        return {Tt, DuvT};
    }

    const double a = (prev.T * (dNext - dCurr) + curr.T * (dPrev - dNext) + next.T * (dCurr - dPrev)) / X;
    if (qFuzzyIsNull(a))
    {
        return {Tt, DuvT};
    }

    const double b = -(prev.T * prev.T * (dNext - dCurr) + curr.T * curr.T * (dPrev - dNext) + next.T * next.T * (dCurr - dPrev)) / X;
    const double c = -(
        dPrev * (next.T - curr.T) * curr.T * next.T
        + dCurr * (prev.T - next.T) * prev.T * next.T
        + dNext * (curr.T - prev.T) * prev.T * curr.T
    ) / X;

    const double Tp = -b / (2.0 * a);
    const double DuvP = (a * Tp * Tp + b * Tp + c) * sign;

    if (abs(DuvT) >= 0.002)
    {
        return {Tp, DuvP};
    }

    return {Tt, DuvT};
}

}
