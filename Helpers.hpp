// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

namespace Helpers {

struct UV1960
{
    double u, v;
};

UV1960 xyzToUV1960(const XYZ &xyz);

double dominantWavelength(const XYZ &xyz);

}
