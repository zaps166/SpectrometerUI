// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "BlackBody.hpp"

namespace BlackBody {

double get(double wavelength, double temperatureK)
{
    const double wlInMeters = wavelength * 1e-9;
    return 0.1 * (3.74183e-16 * pow(wlInMeters, -5.0)) / expm1(1.4388e-2 / (wlInMeters * temperatureK));
}

}
