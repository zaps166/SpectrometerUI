// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

struct SpdData
{
    Data spd;

    QString deviceId;
    QDateTime date;

    double minNm = 0.0;
    double maxNm = 0.0;

    double ee = 0.0;
    double peakWavelength = 0.0;
    double dominantWavelength = 0.0;

    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;

    double sumXYZ = 0.0;

    double x = 0.0;
    double y = 0.0;

    double cct = 0.0;
    double duv = 0.0;
    double sdcm = 0.0;

    QVarLengthArray<double, 15> R;
    double Ra = 0.0;

    double tm30Rf = 0.0;
    double tm30Rg = 0.0;
    QVarLengthArray<double, 16> tm30Rfi;

    double luminousEfficiency = 0.0;
    double lux = 0.0;

    double scotopic = 0.0;
    double spRatio = 0.0;

    double melanopic = 0.0;
    double melanopicLux = 0.0;
    double melanopicRatio = 0.0;
    double mder = 0.0;

    double par = 0.0;
    double ppfd = 0.0;

    double eb = 0.0;
    double blueRatio = 0.0;

    double uvi = 0.0;
    double vitaminD = 0.0;
};
