// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

class Spectrum : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SpectrumPriv)
    Q_PROPERTY(qreal minNm MEMBER m_minNm NOTIFY rangeChanged FINAL)
    Q_PROPERTY(qreal maxNm MEMBER m_maxNm NOTIFY rangeChanged FINAL)
    Q_PROPERTY(bool drawOutline MEMBER m_drawOutline NOTIFY drawOutlineChanged FINAL)

public:
    Spectrum();
    ~Spectrum();

    Q_INVOKABLE void setPathObj(QObject *polyline, QObject *outlinePolyline);

    Q_INVOKABLE QVariantMap getIrradianceAt(qreal nm) const;

    Q_INVOKABLE qreal getPeakIrradiance() const;

    void setData(double minNm, double maxNm, const Data &data);

private:
    void setDataInternal();

    qreal getIrradianceFromPoint(const QPointF &p) const;

signals:
    void rangeChanged();
    void dataUpdated();
    void drawOutlineChanged();

private:
    QObject *m_polyline = nullptr;
    QObject *m_outlinePolyline = nullptr;
    QList<QPointF> m_data;
    QList<QPointF> m_outlineData;
    qreal m_maxIrr = 0.0;
    qreal m_minNm = 0.0;
    qreal m_maxNm = 0.0;
    bool m_drawOutline = false;
};
