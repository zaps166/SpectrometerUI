// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

namespace Tm30Data {

constexpr int CES_N = (780 - 380) / 5 + 1;
constexpr int CES_COUNT = 99;

extern const array<array<double, CES_N>, CES_COUNT> CES;

}
