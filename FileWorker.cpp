// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "FileWorker.hpp"

FileWorker::FileWorker(QObject *parent)
    : SpectrometerWorker(parent)
{
}
FileWorker::~FileWorker()
{
}

bool FileWorker::setParam(const QString &key, const QVariant &value)
{
    if (!isRunning())
    {
        if (key == u"path"_sv)
        {
            m_path = value.toString();
            return true;
        }
        else if (key == u"col"_sv)
        {
            m_col = value.toInt();
            return true;
        }
        else if (key == u"min"_sv)
        {
            m_hintMinNm = value.toDouble();
            return true;
        }
        else if (key == u"max"_sv)
        {
            m_hintMaxNm = value.toDouble();
            return true;
        }
        else if (key == u"b64"_sv)
        {
            m_b64Name = value.toBool();
            return true;
        }
    }
    return false;
}

QString FileWorker::staticDataName() const
{
    const QFileInfo fi(m_path);
    if (m_b64Name)
    {
        return QString::fromUtf8(QByteArray::fromBase64(fi.baseName().toLatin1(), QByteArray::Base64UrlEncoding));
    }
    return fi.fileName();
}

void FileWorker::run()
{
    auto selfDestroy = qScopeGuard([this] {
        deleteLater();
    });

    if (m_path.isEmpty())
    {
        return;
    }

    QFile f(m_path);

    if (f.size() > pow(2.0, 24.0))
    {
        emit errorMesssage(tr("File too large"));
        return;
    }

    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        emit errorMesssage(tr("Unable to open the file"));
        return;
    }

    Data data;

    const QRegularExpression splitRx(uR"(\,|\ |\;)"_s);
    while (!f.atEnd())
    {
        const auto line = QString::fromLatin1(f.readLine().trimmed());

        if (line.startsWith(sDateKey))
        {
            m_date = QDateTime::fromString(line.mid(sDateKey.size() + 2), Qt::ISODate);
            continue;
        }
        else if (line.startsWith(sDeviceKey))
        {
            m_deviceId = line.mid(sDeviceKey.size() + 2);
            continue;
        }
        else if (line.startsWith(sExposureKey))
        {
            const auto exposureSplitted = line.mid(sExposureKey.size() + 2).split(u' ', Qt::SkipEmptyParts);
            if (exposureSplitted.value(1) == u"ms"_sv)
            {
                m_exposure.time = exposureSplitted.value(0).toDouble();
                emitNewExposureData();
            }
            continue;
        }

        const auto lineValues = line.split(splitRx, Qt::SkipEmptyParts);
        if (lineValues.size() >= 2)
        {
            bool ok1 = false;
            bool ok2 = false;
            Entry entry {
                lineValues[0].toDouble(&ok1),
                lineValues.value(m_col).toDouble(&ok2),
            };
            if (ok1 && ok2)
            {
                data.emplace_back(entry);
            }
        }
    }

    if (data.size() < 2)
    {
        emit errorMesssage(tr("No data in the file"));
        return;
    }

    if (!ranges::is_sorted(data, {}, &Entry::first))
    {
        emit errorMesssage(tr("Data is not sorted"));
        return;
    }

    m_minNm = data.constFirst().first;
    m_maxNm = data.constLast().first;
    if (m_minNm <= 0 || m_maxNm <= m_minNm)
    {
        emit errorMesssage(tr("Wrong data"));
        return;
    }

    processSpd(data);
}
