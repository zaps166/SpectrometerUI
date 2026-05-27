// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "CriGraph.hpp"
#include "Application.hpp"
#include "SpdData.hpp"

Cri::Cri()
    : m_rValues(15)
{
    connect(Application::instance(), &Application::newSpdData, this, [this](const shared_ptr<SpdData> &spdData) {
        const bool hasCri = (spdData->R.size() == spdData->R.PreallocatedSize);

        if (hasCri)
        {
            for (auto [dst, src] : views::zip(m_rValues, spdData->R))
            {
                dst = src;
            }
        }
        else
        {
            m_rValues.fill(0.0);
        }

        if (m_valid || m_valid != hasCri)
        {
            emit rValuesChanged();
        }

        if (m_valid != hasCri)
        {
            m_valid = hasCri;
            emit validChanged();
        }
    });
}
Cri::~Cri()
{
}
