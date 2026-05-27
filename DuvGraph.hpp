// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

class DuvGraph : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DuvGraphPriv)
    Q_PROPERTY(QPointF currentPoint MEMBER m_currentPoint NOTIFY currentPointChanged FINAL)
    Q_PROPERTY(bool valid MEMBER m_valid NOTIFY validChanged FINAL)

public:
    DuvGraph();
    ~DuvGraph();

    Q_INVOKABLE void setSpectralLocus(QObject *pathPolyLine);
    Q_INVOKABLE void setPlanckianLocus(QObject *pathPolyLine);

signals:
    void currentPointChanged();
    void validChanged();

private:
    QList<QPointF> m_spectralLocus;
    QList<QPointF> m_planckianLocus;
    QPointF m_currentPoint;
    bool m_valid = false;
};
