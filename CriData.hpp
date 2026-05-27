// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

namespace CriData {

constexpr int TCS_N = (780 - 380) / 5 + 1;
constexpr int TCS_COUNT = 15;

extern const array<array<double, TCS_N>, TCS_COUNT> TCS;

}
