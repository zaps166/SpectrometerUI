// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

// MacAdam ellipse based SDCM calculation (ANSI C78.377).
// Computes SDCM from Duv and CCT using CCT-dependent MacAdam step sizes.

#include "Headers.hpp"

namespace MacAdam {

double calculateSdcm(double duv, double cct);

}
