// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

class Tm30Graph : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Tm30GraphPriv)
    Q_PROPERTY(QList<qreal> rfValues MEMBER m_rfValues NOTIFY rfValuesChanged FINAL)
    Q_PROPERTY(bool valid MEMBER m_valid NOTIFY validChanged FINAL)

public:
    Tm30Graph();
    ~Tm30Graph();

signals:
    void rfValuesChanged();
    void validChanged();

private:
    QList<qreal> m_rfValues;
    bool m_valid = false;
};
