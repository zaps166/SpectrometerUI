// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

namespace BlackBody {

double get(double wavelength, double temperatureK);

static auto getView(int minWavelength, int maxWavelength, double temperatureK)
{
    return views::iota(minWavelength, maxWavelength + 1) | views::transform([temperatureK](double wavelength) {
        return BlackBody::get(wavelength, temperatureK);
    });
}
static auto getViewLimited(double temperatureK)
{
    return getView(380, 380 + g_limitedVisSize - 1, temperatureK);
}

}
