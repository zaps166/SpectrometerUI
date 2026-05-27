// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#ifndef Q_MOC_RUN

#include <QAbstractListModel>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QSaveFile>
#include <QFileInfo>
#include <QUrlQuery>
#include <QSettings>
#include <QtEndian>
#include <QThread>
#include <QFile>
#include <QDir>

#ifndef ALGORITHM_TESTS

# include <QQmlApplicationEngine>

# include <QColorSpace>
# include <QStyleHints>

# include <QQuickWindow>
# include <QQuickStyle>

# ifdef USE_QT_SERIAL_PORT
#  include <QSerialPortInfo>
#  include <QSerialPort>
# else
#  include <libusb.h>
# endif

# ifdef Q_OS_ANDROID
#  include <QJniObject>
# endif

#else

# include <QtTest/QtTest>
# include <QProcess>

#endif

#include <algorithm>
#include <numbers>
#include <ranges>
#include <thread>
#include <chrono>
#include <mutex>
#include <cmath>

using namespace Qt::StringLiterals;
using namespace std;

constexpr qsizetype g_limitedVisSize = 401; // [380 - 780], 1 nm step

using Entry = pair<double, double>;
using Data = QList<Entry>;
using VisData = QVarLengthArray<double, g_limitedVisSize>;

struct XYZ
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

#else

#include <QtQml/qqmlregistration.h>

#endif
