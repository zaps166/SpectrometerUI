// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

class Cri : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CriPriv)
    Q_PROPERTY(QList<qreal> rValues MEMBER m_rValues NOTIFY rValuesChanged FINAL)
    Q_PROPERTY(bool valid MEMBER m_valid NOTIFY validChanged FINAL)

public:
    Cri();
    ~Cri();

signals:
    void rValuesChanged();
    void validChanged();

private:
    QList<qreal> m_rValues;
    bool m_valid = false;
};
