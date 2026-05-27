// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "AlgorithmTests.hpp"

#include <CieDIlluminantData.hpp>
#include <CieXyz1931TwoDegData.hpp>
#include <BlackBody.hpp>
#include <Helpers.hpp>
#include <Cri.hpp>
#include <DataSeqIterp.hpp>
#include <Ohno2013.hpp>
#include <SpdData.hpp>
#include <Tm30.hpp>

static bool loadSpectrumCsv(const QString &path, int valueColumn, VisData &out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    Data samples;
    samples.reserve(512);

    const QRegularExpression delimiter(u"[,\\s]+"_s);
    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        const QStringList parts = line.split(delimiter, Qt::SkipEmptyParts);
        if (parts.size() <= valueColumn)
            return false;

        bool ok = false;
        const double wavelength = parts[0].toDouble(&ok);
        if (!ok)
            return false;

        const double value = parts[valueColumn].toDouble(&ok);
        if (!ok)
            return false;

        samples.append(Entry{wavelength, value});
    }

    if (samples.size() < 2)
        return false;

    if (samples.first().first > 380.0)
    {
        const auto [wl0, val0] = samples[0];
        const auto [wl1, val1] = samples[1];
        const double slope = (val1 - val0) / (wl1 - wl0);
        samples.prepend(Entry{380.0, val0 + slope * (380.0 - wl0)});
    }
    if (samples.last().first < 780.0)
    {
        const auto [wl0, val0] = samples[samples.size() - 2];
        const auto [wl1, val1] = samples.last();
        const double slope = (val1 - val0) / (wl1 - wl0);
        samples.append(Entry{780.0, val1 + slope * (780.0 - wl1)});
    }

    out.resize(g_limitedVisSize);
    DataSeqIterp dsi(samples);
    for (int nm = 380; nm < 380 + g_limitedVisSize; ++nm)
    {
        out[nm - 380] = dsi.get(nm);
    }

    return true;
}

static const VisData *sampleForName(const QString &sample, const VisData &led1, const VisData &led9, const VisData &sun)
{
    if (sample == u"LED1"_s)
        return &led1;
    if (sample == u"LED9"_s)
        return &led9;
    if (sample == u"Sun"_s)
        return &sun;
    return nullptr;
}

static void compareNear(double actual, double expected, double tolerance, const char *actualExpr, const char *expectedExpr)
{
    QVERIFY2(abs(actual - expected) <= tolerance,
             qPrintable(u"%1=%2 differs from %3=%4 by more than %5"_s
                            .arg(QString::fromLatin1(actualExpr))
                            .arg(actual, 0, 'g', 15)
                            .arg(QString::fromLatin1(expectedExpr))
                            .arg(expected, 0, 'g', 15)
                            .arg(tolerance, 0, 'g', 15)));
}

#define COMPARE_NEAR(actual, expected, tol) compareNear((actual), (expected), (tol), #actual, #expected)

static bool runPythonReference(const QString &script, const QString &stdinText, QString &stdoutText)
{
    QProcess process;
    process.start(u"python3"_s, {u"-c"_s, script});
    if (!process.waitForStarted())
        return false;
    if (!stdinText.isEmpty())
        process.write(stdinText.toUtf8());
    process.closeWriteChannel();
    if (!process.waitForFinished())
        return false;
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return false;

    stdoutText = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return true;
}

static bool referenceCctDuvFromSpectrum(const VisData &spd, double &cct, double &duv)
{
    QString csv;
    csv.reserve(spd.size() * 16);
    for (qsizetype i = 0; i < spd.size(); ++i)
    {
        csv += QString::number(380 + i);
        csv += u","_s;
        csv += QString::number(spd[i], 'g', 17);
        csv += u"\n"_s;
    }

    QString out;
    const QString script =
        u"import sys, io, numpy as np, colour\n"_s
        u"csv = sys.stdin.read()\n"_s
        u"data = np.loadtxt(io.StringIO(csv), delimiter=',')\n"_s
        u"sd = colour.SpectralDistribution(data[:, 1], data[:, 0])\n"_s
        u"cmfs = colour.MSDS_CMFS['CIE 1931 2 Degree Standard Observer'].copy().align(colour.SpectralShape(360, 830, 1))\n"_s
        u"XYZ = colour.colorimetry.msds_to_XYZ_integration(sd, cmfs=cmfs, shape=colour.SpectralShape(380, 780, 1))\n"_s
        u"res = colour.temperature.XYZ_to_CCT_Ohno2013(XYZ, start=1000, end=100000, spacing=1.001)\n"_s
        u"print(f'{res[0]:.12f} {res[1]:.12f}')\n"_s;

    if (!runPythonReference(script, csv, out))
        return false;

    const auto parts = out.split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts);
    if (parts.size() != 2)
        return false;

    bool ok = false;
    cct = parts[0].toDouble(&ok);
    if (!ok)
        return false;
    duv = parts[1].toDouble(&ok);
    return ok;
}

static bool referenceDominantWavelength(const XYZ &xyz, double &wavelength)
{
    const double sum = xyz.X + xyz.Y + xyz.Z;
    const double x = qFuzzyIsNull(sum) ? 0.0 : xyz.X / sum;
    const double y = qFuzzyIsNull(sum) ? 0.0 : xyz.Y / sum;

    const QString script =
        u"import sys\n"_s
        u"import numpy as np\n"_s
        u"import colour\n"_s
        u"xy = np.array([float(sys.argv[1]), float(sys.argv[2])])\n"_s
        u"xy_n = np.array([1/3, 1/3])\n"_s
        u"wl, xy_wl, xy_cw = colour.dominant_wavelength(xy, xy_n)\n"_s
        u"print(f'{wl:.12f}')\n"_s;

    QString out;
    QProcess process;
    process.start(u"python3"_s, {u"-c"_s, script, QString::number(x, 'g', 17), QString::number(y, 'g', 17)});
    if (!process.waitForFinished())
        return false;
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return false;

    bool ok = false;
    wavelength = QString::fromUtf8(process.readAllStandardOutput()).trimmed().toDouble(&ok);
    return ok;
}

static bool referenceCri(const VisData &spd, double &ra, QVarLengthArray<double, 15> &rValues)
{
    QString csv;
    csv.reserve(spd.size() * 16);
    for (qsizetype i = 0; i < spd.size(); ++i)
    {
        csv += QString::number(380 + i);
        csv += u","_s;
        csv += QString::number(spd[i], 'g', 17);
        csv += u"\n"_s;
    }

    QString out;
    const QString script =
        u"import sys, numpy as np, colour, tempfile\n"_s
        u"from colour import SpectralDistribution\n"_s
        u"csv = sys.stdin.read()\n"_s
        u"tmp = tempfile.NamedTemporaryFile(delete=False, suffix='.csv')\n"_s
        u"tmp.write(csv.encode('utf-8'))\n"_s
        u"tmp.close()\n"_s
        u"data = np.loadtxt(tmp.name, delimiter=',')\n"_s
        u"sd = SpectralDistribution(data[:, 1], data[:, 0])\n"_s
        u"res = colour.quality.colour_rendering_index(sd, additional_data=True)\n"_s
        u"vals = [res.Q_a] + [res.Q_as[i].Q_a for i in range(1, 15)]\n"_s
        u"print(' '.join(f'{v:.12f}' for v in vals))\n"_s;

    if (!runPythonReference(script, csv, out))
        return false;
    const auto parts = out.split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts);
    if (parts.size() != 15)
        return false;

    bool ok = false;
    ra = parts[0].toDouble(&ok);
    if (!ok)
        return false;
    rValues.resize(14);
    for (int i = 0; i < 14; ++i)
    {
        rValues[i] = parts[i + 1].toDouble(&ok);
        if (!ok)
            return false;
    }
    return true;
}

static bool referenceTm30(const VisData &spd, double &rf, double &rg, QVarLengthArray<double, 16> &bins)
{
    QString csv;
    csv.reserve(spd.size() * 16);
    for (qsizetype i = 0; i < spd.size(); ++i)
    {
        csv += QString::number(380 + i);
        csv += u","_s;
        csv += QString::number(spd[i], 'g', 17);
        csv += u"\n"_s;
    }

    QString out;
    const QString script =
        u"import sys, numpy as np, colour, tempfile\n"_s
        u"from colour import SpectralDistribution\n"_s
        u"csv = sys.stdin.read()\n"_s
        u"tmp = tempfile.NamedTemporaryFile(delete=False, suffix='.csv')\n"_s
        u"tmp.write(csv.encode('utf-8'))\n"_s
        u"tmp.close()\n"_s
        u"data = np.loadtxt(tmp.name, delimiter=',')\n"_s
        u"sd = SpectralDistribution(data[:, 1], data[:, 0])\n"_s
        u"res = colour.quality.colour_fidelity_index(sd, additional_data=True, method='ANSI/IES TM-30-18')\n"_s
        u"vals = [res.R_f, res.R_g] + list(res.R_fs)\n"_s
        u"print(' '.join(f'{v:.12f}' for v in vals))\n"_s;

    if (!runPythonReference(script, csv, out))
        return false;
    const auto parts = out.split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts);
    if (parts.size() != 18)
        return false;

    bool ok = false;
    rf = parts[0].toDouble(&ok);
    if (!ok)
        return false;
    rg = parts[1].toDouble(&ok);
    if (!ok)
        return false;
    bins.resize(16);
    for (int i = 0; i < 16; ++i)
    {
        bins[i] = parts[i + 2].toDouble(&ok);
        if (!ok)
            return false;
    }
    return true;
}

static void seedCctDuv(SpdData &spdData, const VisData &spd)
{
    const auto [cct, duv] = Ohno2013::computeCCT(spd);
    spdData.cct = cct;
    spdData.duv = duv;
}

enum class GeneratedSpectrumKind
{
    BlackBody,
    Daylight,
};

static VisData generatedSpectrum(GeneratedSpectrumKind kind, double cct)
{
    switch (kind)
    {
        case GeneratedSpectrumKind::BlackBody:
            return BlackBody::getViewLimited(cct) | ranges::to<VisData>();
        case GeneratedSpectrumKind::Daylight:
            return CieDIlluminantData::getSpd(cct);
    }
    return VisData();
}

void AlgorithmTests::initTestCase()
{
    QVERIFY2(loadSpectrumCsv(u":/CIE_illum_LEDs_1nm.csv"_s, 1, m_led1), qPrintable(u"failed to load LED1 CSV resource"_s));
    QVERIFY2(loadSpectrumCsv(u":/CIE_illum_LEDs_1nm.csv"_s, 9, m_led9), qPrintable(u"failed to load LED9 CSV resource"_s));
    QVERIFY2(loadSpectrumCsv(u":/Sun.csv"_s, 1, m_sun), qPrintable(u"failed to load Sun CSV resource"_s));
    m_zero.resize(g_limitedVisSize, 0.0);
}

void AlgorithmTests::cctDuv_data()
{
    QTest::addColumn<QString>("sample");

    QTest::newRow("LED1") << u"LED1"_s;
    QTest::newRow("LED9") << u"LED9"_s;
    QTest::newRow("Sun") << u"Sun"_s;
}

void AlgorithmTests::cctDuv()
{
    QFETCH(QString, sample);

    const VisData *spd = sampleForName(sample, m_led1, m_led9, m_sun);
    QVERIFY2(spd != nullptr, qPrintable(u"unknown sample"_s));
    const auto [cct, duv] = Ohno2013::computeCCT(*spd);
    double cctBack = 0.0;
    double duvBack = 0.0;
    QVERIFY2(referenceCctDuvFromSpectrum(*spd, cctBack, duvBack), qPrintable(u"python reference failed"_s));
    COMPARE_NEAR(cct, cctBack, 0.05);
    COMPARE_NEAR(duv, duvBack, 2e-5);
}

void AlgorithmTests::blackBodyCctDuv_data()
{
    QTest::addColumn<double>("temperature");

    QTest::newRow("2500K") << 2500.0;
    QTest::newRow("3000K") << 3000.0;
    QTest::newRow("4000K") << 4000.0;
    QTest::newRow("20000K") << 20000.0;
    QTest::newRow("50000K") << 50000.0;
}

void AlgorithmTests::blackBodyCctDuv()
{
    QFETCH(double, temperature);

    const VisData spd = BlackBody::getViewLimited(temperature) | ranges::to<VisData>();
    const auto [cct, duv] = Ohno2013::computeCCT(spd);
    double cctBack = 0.0;
    double duvBack = 0.0;
    QVERIFY2(referenceCctDuvFromSpectrum(spd, cctBack, duvBack), qPrintable(u"python black body reference failed"_s));

    COMPARE_NEAR(cct, cctBack, 1.0);
    COMPARE_NEAR(duv, duvBack, 1e-5);
}

void AlgorithmTests::daylightCctDuv_data()
{
    QTest::addColumn<double>("temperature");

    QTest::newRow("4000K") << 4000.0;
    QTest::newRow("5000K") << 5000.0;
    QTest::newRow("6500K") << 6500.0;
    QTest::newRow("20000K") << 20000.0;
    QTest::newRow("50000K") << 50000.0;
}

void AlgorithmTests::daylightCctDuv()
{
    QFETCH(double, temperature);

    const VisData spd = CieDIlluminantData::getSpd(temperature);
    const auto [cct, duv] = Ohno2013::computeCCT(spd);
    double cctBack = 0.0;
    double duvBack = 0.0;
    QVERIFY2(referenceCctDuvFromSpectrum(spd, cctBack, duvBack), qPrintable(u"python daylight reference failed"_s));

    COMPARE_NEAR(cct, cctBack, 1.0);
    COMPARE_NEAR(duv, duvBack, 1e-5);
    QVERIFY2(duvBack > 0.0, qPrintable(u"daylight DUV reference should be positive"_s));
}

void AlgorithmTests::zeroSpectrum()
{
    const auto [cct, duv] = Ohno2013::computeCCT(m_zero);
    QVERIFY2(qFuzzyIsNull(cct), qPrintable(u"zero spectrum CCT should be disabled"_s));
    QCOMPARE(duv, 0.0);

    SpdData spdData;
    seedCctDuv(spdData, m_zero);
    Cri::compute(spdData, m_zero);
    Tm30::compute(spdData, m_zero);
    QVERIFY(spdData.R.empty());
    QVERIFY(spdData.tm30Rfi.empty());
}

void AlgorithmTests::dominantWavelength_data()
{
    QTest::addColumn<QString>("sample");

    QTest::newRow("LED1") << u"LED1"_s;
    QTest::newRow("LED9") << u"LED9"_s;
    QTest::newRow("Sun") << u"Sun"_s;
}

void AlgorithmTests::dominantWavelength()
{
    QFETCH(QString, sample);
    const VisData *spd = sampleForName(sample, m_led1, m_led9, m_sun);
    QVERIFY2(spd != nullptr, qPrintable(u"unknown sample"_s));

    const XYZ xyz = CieXyz1931TwoDegData::getXyzFromSpdLimited(*spd);
    double expected = 0.0;
    QVERIFY2(referenceDominantWavelength(xyz, expected), qPrintable(u"python dominant wavelength reference failed"_s));

    const double actual = Helpers::dominantWavelength(xyz);
    COMPARE_NEAR(actual, expected, 1e-6);
}

void AlgorithmTests::cieDSeries_data()
{
    QTest::addColumn<double>("cct");

    QTest::newRow("4000K") << 4000.0;
    QTest::newRow("5000K") << 5000.0;
    QTest::newRow("6500K") << 6500.0;
}

void AlgorithmTests::cieDSeries()
{
    QFETCH(double, cct);

    const QString script =
        u"import sys\n"_s
        u"import colour\n"_s
        u"cct = float(sys.argv[1])\n"_s
        u"xy = colour.temperature.CCT_to_xy_CIE_D(cct)\n"_s
        u"sd = colour.sd_CIE_illuminant_D_series(xy, M1_M2_rounding=False, shape=colour.SpectralShape(380, 780, 1))\n"_s
        u"values = [sd[w] for w in range(380, 781)]\n"_s
        u"print(' '.join(f'{v:.12f}' for v in values))\n"_s;

    QProcess process;
    process.start(u"python3"_s, {u"-c"_s, script, QString::number(cct, 'g', 17)});
    QVERIFY2(process.waitForFinished(), qPrintable(u"python CIE D-series reference failed"_s));
    QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, qPrintable(u"python CIE D-series reference failed"_s));

    const auto parts = QString::fromUtf8(process.readAllStandardOutput()).trimmed().split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts);
    QCOMPARE(parts.size(), g_limitedVisSize);

    QVector<double> expected;
    expected.reserve(g_limitedVisSize);
    bool ok = false;
    for (const auto &part : parts)
    {
        expected.push_back(part.toDouble(&ok));
        QVERIFY(ok);
    }

    const auto sd = CieDIlluminantData::getSpd(cct);
    QCOMPARE(sd.size(), g_limitedVisSize);
    for (qsizetype i = 0; i < sd.size(); ++i)
        COMPARE_NEAR(sd[i], expected[i], 1e-6);
}

void AlgorithmTests::cri_data()
{
    QTest::addColumn<QString>("sample");

    QTest::newRow("LED1") << u"LED1"_s;
    QTest::newRow("LED9") << u"LED9"_s;
    QTest::newRow("Sun") << u"Sun"_s;
}

void AlgorithmTests::criGenerated_data()
{
    QTest::addColumn<GeneratedSpectrumKind>("kind");
    QTest::addColumn<double>("cct");

    QTest::newRow("blackBody 2700K") << GeneratedSpectrumKind::BlackBody << 2700.0;
    QTest::newRow("blackBody 4500K") << GeneratedSpectrumKind::BlackBody << 4500.0;
    QTest::newRow("blackBody 6500K") << GeneratedSpectrumKind::BlackBody << 6500.0;
    QTest::newRow("daylight 2700K") << GeneratedSpectrumKind::Daylight << 2700.0;
    QTest::newRow("daylight 4500K") << GeneratedSpectrumKind::Daylight << 4500.0;
    QTest::newRow("daylight 6500K") << GeneratedSpectrumKind::Daylight << 6500.0;
}

void AlgorithmTests::criGenerated()
{
    QFETCH(GeneratedSpectrumKind, kind);
    QFETCH(double, cct);

    const auto spectrumKind = static_cast<GeneratedSpectrumKind>(kind);
    const VisData spd = generatedSpectrum(spectrumKind, cct);

    double expectedRa = 0.0;
    QVarLengthArray<double, 15> expectedRi;
    QVERIFY2(referenceCri(spd, expectedRa, expectedRi), qPrintable(u"python CRI reference failed"_s));

    SpdData spdData;
    seedCctDuv(spdData, spd);
    Cri::compute(spdData, spd);

    COMPARE_NEAR(spdData.Ra, expectedRa, 0.5);
    QCOMPARE(spdData.R.size(), 15);
    for (qsizetype i = 0; i < expectedRi.size(); ++i)
        COMPARE_NEAR(spdData.R[i], expectedRi[i], 0.1);
}

void AlgorithmTests::cri()
{
    QFETCH(QString, sample);
    const VisData *spd = sampleForName(sample, m_led1, m_led9, m_sun);
    QVERIFY2(spd != nullptr, qPrintable(u"unknown sample"_s));

    double expectedRa = 0.0;
    QVarLengthArray<double, 15> expectedRi;
    QVERIFY2(referenceCri(*spd, expectedRa, expectedRi), qPrintable(u"python CRI reference failed"_s));

    SpdData spdData;
    seedCctDuv(spdData, *spd);
    Cri::compute(spdData, *spd);

    COMPARE_NEAR(spdData.Ra, expectedRa, 0.5);
    QCOMPARE(spdData.R.size(), 15);
    for (qsizetype i = 0; i < expectedRi.size(); ++i)
        COMPARE_NEAR(spdData.R[i], expectedRi[i], 0.1);
}

void AlgorithmTests::tm30_data()
{
    QTest::addColumn<QString>("sample");

    QTest::newRow("LED1") << u"LED1"_s;
    QTest::newRow("LED9") << u"LED9"_s;
    QTest::newRow("Sun") << u"Sun"_s;
}

void AlgorithmTests::tm30Generated_data()
{
    QTest::addColumn<GeneratedSpectrumKind>("kind");
    QTest::addColumn<double>("cct");

    QTest::newRow("blackBody 2700K") << GeneratedSpectrumKind::BlackBody << 2700.0;
    QTest::newRow("blackBody 4500K") << GeneratedSpectrumKind::BlackBody << 4500.0;
    QTest::newRow("blackBody 6500K") << GeneratedSpectrumKind::BlackBody << 6500.0;
    QTest::newRow("daylight 2700K") << GeneratedSpectrumKind::Daylight << 2700.0;
    QTest::newRow("daylight 4500K") << GeneratedSpectrumKind::Daylight << 4500.0;
    QTest::newRow("daylight 6500K") << GeneratedSpectrumKind::Daylight << 6500.0;
}

void AlgorithmTests::tm30Generated()
{
    QFETCH(GeneratedSpectrumKind, kind);
    QFETCH(double, cct);

    const auto spectrumKind = static_cast<GeneratedSpectrumKind>(kind);
    const VisData spd = generatedSpectrum(spectrumKind, cct);

    double expectedRf = 0.0;
    double expectedRg = 0.0;
    QVarLengthArray<double, 16> expectedBins;
    QVERIFY2(referenceTm30(spd, expectedRf, expectedRg, expectedBins), qPrintable(u"python TM-30 reference failed"_s));

    SpdData spdData;
    seedCctDuv(spdData, spd);
    Tm30::compute(spdData, spd);

    COMPARE_NEAR(spdData.tm30Rf, expectedRf, 0.05);
    COMPARE_NEAR(spdData.tm30Rg, expectedRg, 0.05);
    QCOMPARE(spdData.tm30Rfi.size(), 16);
    for (qsizetype i = 0; i < expectedBins.size(); ++i)
        COMPARE_NEAR(spdData.tm30Rfi[i], expectedBins[i], 0.05);
}

void AlgorithmTests::tm30()
{
    QFETCH(QString, sample);
    const VisData *spd = sampleForName(sample, m_led1, m_led9, m_sun);
    QVERIFY2(spd != nullptr, qPrintable(u"unknown sample"_s));

    double expectedRf = 0.0;
    double expectedRg = 0.0;
    QVarLengthArray<double, 16> expectedBins;
    QVERIFY2(referenceTm30(*spd, expectedRf, expectedRg, expectedBins), qPrintable(u"python TM-30 reference failed"_s));

    SpdData spdData;
    seedCctDuv(spdData, *spd);
    Tm30::compute(spdData, *spd);

    COMPARE_NEAR(spdData.tm30Rf, expectedRf, 0.05);
    COMPARE_NEAR(spdData.tm30Rg, expectedRg, 0.05);
    QCOMPARE(spdData.tm30Rfi.size(), 16);
    for (qsizetype i = 0; i < expectedBins.size(); ++i)
        COMPARE_NEAR(spdData.tm30Rfi[i], expectedBins[i], 0.05);
}

QTEST_APPLESS_MAIN(AlgorithmTests)
