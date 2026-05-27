// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"
#include "Exposure.hpp"

struct SpdData;

class SpectrometerWorker : public QThread
{
    Q_OBJECT

public:
    static const QString sDateKey;
    static const QString sDeviceKey;
    static const QString sExposureKey;

public:
    SpectrometerWorker(QObject *parent);
    ~SpectrometerWorker();

    void begin();

    virtual bool setParam(const QString &key, const QVariant &value) = 0;

    virtual QString staticDataName() const = 0;

protected:
    void setCurrentDate();
    void emitNewExposureData();

    void processSpd(const Data &spd);

Q_SIGNALS:
    void newExposureData(const Exposure &exposure);
    void newSpdData(const std::shared_ptr<SpdData> &spdData, QPrivateSignal);
    void commandsFinished();
    void errorMesssage(const QString &errStr);

protected:
    QString m_deviceId;
    QDateTime m_date;

    double m_hintMinNm = 0.0;
    double m_hintMaxNm = 0.0;

    double m_minNm = 0.0;
    double m_maxNm = 0.0;

    Exposure m_exposure;
};
