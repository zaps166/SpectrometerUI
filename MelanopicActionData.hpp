// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

namespace MelanopicAction {

using Data = array<Entry, g_limitedVisSize>;

constexpr double step = 1.0;

extern const Data data;

}
