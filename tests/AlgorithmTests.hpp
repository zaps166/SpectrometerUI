// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <Headers.hpp>

class AlgorithmTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void cctDuv_data();
    void cctDuv();

    void blackBodyCctDuv_data();
    void blackBodyCctDuv();

    void daylightCctDuv_data();
    void daylightCctDuv();

    void zeroSpectrum();

    void dominantWavelength_data();
    void dominantWavelength();

    void cieDSeries_data();
    void cieDSeries();

    void cri_data();
    void cri();

    void criGenerated_data();
    void criGenerated();

    void tm30_data();
    void tm30();

    void tm30Generated_data();
    void tm30Generated();

private:
    VisData m_led1;
    VisData m_led9;
    VisData m_sun;
    VisData m_zero;
};
