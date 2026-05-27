// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

namespace CieXyz1931TwoDegData {

using Data = array<XYZ, 471>;

extern const Data data;

static inline auto dataLimited()
{
    return data | views::drop(20) | views::take(g_limitedVisSize);
}

static inline auto dataWithWavelength()
{
    return views::zip(views::iota(360), data);
}
static inline auto dataLimitedWithWavelength()
{
    return views::zip(views::iota(380), dataLimited());
}

static XYZ getXyzFromSpd(auto &&spd)
{
    XYZ xyz;
    Q_ASSERT(spd.size() == data.size());
    for (auto [irradiance, cieXYZ] : views::zip(as_const(spd), data))
    {
        xyz.X += irradiance * cieXYZ.X;
        xyz.Y += irradiance * cieXYZ.Y;
        xyz.Z += irradiance * cieXYZ.Z;
    }
    return xyz;
}
static XYZ getXyzFromSpdLimited(auto &&spd)
{
    XYZ xyz;
    Q_ASSERT(spd.size() == g_limitedVisSize);
    for (auto [irradiance, cieXYZ] : views::zip(as_const(spd), dataLimited()))
    {
        xyz.X += irradiance * cieXYZ.X;
        xyz.Y += irradiance * cieXYZ.Y;
        xyz.Z += irradiance * cieXYZ.Z;
    }
    return xyz;
}

}
