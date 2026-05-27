// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "Tm30Graph.hpp"
#include "Application.hpp"
#include "SpdData.hpp"

Tm30Graph::Tm30Graph()
    : m_rfValues(16)
{
    connect(Application::instance(), &Application::newSpdData, this, [this](const shared_ptr<SpdData> &spdData) {
        const bool hasTm30 = (spdData->tm30Rfi.size() == spdData->tm30Rfi.PreallocatedSize);

        if (hasTm30)
        {
            for (auto [dst, src] : views::zip(m_rfValues, spdData->tm30Rfi))
            {
                dst = src;
            }
        }
        else
        {
            m_rfValues.fill(0.0);
        }

        if (m_valid || m_valid != hasTm30)
        {
            emit rfValuesChanged();
        }

        if (m_valid != hasTm30)
        {
            m_valid = hasTm30;
            emit validChanged();
        }
    });
}
Tm30Graph::~Tm30Graph()
{
}
