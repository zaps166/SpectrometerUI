// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

struct Exposure
{
    bool operator ==(const Exposure &) const = default;
    bool operator !=(const Exposure &) const = default;

    bool ae = false;
    bool aeInProgress = false;
    double time = 0.0;
    double min = 0.0;
    double max = 0.0;
};
