// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

namespace CieXyz1964TenDegData {

using Data = array<XYZ, 471>;

extern const Data data;

static inline auto dataLimited()
{
    return data | views::drop(20) | views::take(g_limitedVisSize);
}

}
