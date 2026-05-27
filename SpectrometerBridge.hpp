// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"
#include "Exposure.hpp"
#include "SpdData.hpp"

class SpectrometerWorker;

class SpectrometerBridge : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isP3 MEMBER m_isP3 NOTIFY isP3Changed FINAL)

public:
    enum class Group : uint32_t
    {
        None = 0,
        Base = 1,
        PPFD = 2,
        BlueLight = 4,
    };
    Q_DECLARE_FLAGS(Groups, Group)
    Q_FLAG(Groups)

    struct ModelEntry
    {
        QString id;
        QString key;
        QString value;
        Groups groups = Group::None;
        qsizetype likedPos = -1;
    };
    using EntryList = vector<ModelEntry>;

public:
    SpectrometerBridge();
    ~SpectrometerBridge();

    Q_INVOKABLE void getWindowColorProfile(QQuickWindow *win);

    Q_INVOKABLE void storeSplitView(const QByteArray &data);
    Q_INVOKABLE QByteArray restoreSplitView();

    Q_INVOKABLE void create(const QUrl &url);
    Q_INVOKABLE void createDevice(bool openErr);

    Q_INVOKABLE void setDeviceParams(double minNm, double maxNm, const QString &device);
    Q_INVOKABLE QVariantMap getDeviceParams();

    Q_INVOKABLE void requestQuit();

    Q_INVOKABLE QStringList getDevices();

    Q_INVOKABLE void start(bool single);
    Q_INVOKABLE void stop();

    Q_INVOKABLE void setExposure(bool autoExposure, qreal exposureTime);

    Q_INVOKABLE int getIdRole() const;
    Q_INVOKABLE int getLikedPosRole() const;

    Q_INVOKABLE void setLiked(const QString &id, qsizetype likedPos);

    Q_INVOKABLE bool storeData(const QUrl &url);

    Q_INVOKABLE void setDataInternalCache(bool cache);

    Q_INVOKABLE bool canStoreDataInternal(const QString &name);
    Q_INVOKABLE bool storeDataInternal(const QString &name);
    Q_INVOKABLE bool renameDataInternal(const QString &oldName, const QString &newName);

    Q_INVOKABLE QVariantList getStoredMeasurements() const;

    Q_INVOKABLE bool deleteStoredMeasurement(const QString &name);

public:
    int rowCount(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role) const;

    QHash<int, QByteArray> roleNames() const;

private:
    void onNewExposureData(const Exposure &exposure);

    void onNewSpdData(const shared_ptr<SpdData> &spdData);

    void emitExposureChanged();

private:
    static QString sMeasurementFileName(const QString &name);

Q_SIGNALS:
    void quit();

    void commandsStarted();
    void commandsFinished();
    void errorMessage(const QString &errStr);

    void exposureChanged(double exposureTime, bool ae, bool aeInProgress, double min, double max);

    void staticDataNameChanged(const QString &name);

    void isP3Changed();

private:
    SpectrometerWorker *m_worker = nullptr;

    Exposure m_exposure;

    QHash<QString, qsizetype> m_likedSettings;

    shared_ptr<SpdData> m_spdData;

    EntryList m_entryListTmp;
    EntryList m_entryList;

    QStringList m_dataInternalCache;

    bool m_isP3 = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SpectrometerBridge::Groups)
