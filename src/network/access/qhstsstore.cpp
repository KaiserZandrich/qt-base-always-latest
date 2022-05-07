/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qhstsstore_p.h"
#include "qhstspolicy.h"

#include "qstandardpaths.h"
#include "qdatastream.h"
#include "qbytearray.h"
#include "qdatetime.h"
#include "qvariant.h"
#include "qstring.h"
#include "qdir.h"

#include <utility>

QT_BEGIN_NAMESPACE

static QString host_name_to_settings_key(const QString &hostName)
{
    const QByteArray hostNameAsHex(hostName.toUtf8().toHex());
    return QString::fromLatin1(hostNameAsHex);
}

static QString settings_key_to_host_name(const QString &key)
{
    const QByteArray hostNameAsUtf8(QByteArray::fromHex(key.toLatin1()));
    return QString::fromUtf8(hostNameAsUtf8);
}

QHstsStore::QHstsStore(const QString &dirName)
    : store(absoluteFilePath(dirName), QSettings::IniFormat)
{
    // Disable fallbacks, we do not want to use anything but our own ini file.
    store.setFallbacksEnabled(false);
}

QHstsStore::~QHstsStore()
{
    synchronize();
}

QList<QHstsPolicy> QHstsStore::readPolicies()
{
    // This function only attempts to read policies, making no decision about
    // expired policies. It's up to a user (QHstsCache) to mark these policies
    // for deletion and sync the store later. But we immediately remove keys/values
    // (if the store isWritable) for the policies that we fail to read.
    QList<QHstsPolicy> policies;

    beginHstsGroups();

    const QStringList keys = store.childKeys();
    for (const auto &key : keys) {
        QHstsPolicy restoredPolicy;
        if (deserializePolicy(key, restoredPolicy)) {
            restoredPolicy.setHost(settings_key_to_host_name(key));
            policies.push_back(std::move(restoredPolicy));
        } else if (isWritable()) {
            evictPolicy(key);
        }
    }

    endHstsGroups();

    return policies;
}

void QHstsStore::addToObserved(const QHstsPolicy &policy)
{
    observedPolicies.push_back(policy);
}

void QHstsStore::synchronize()
{
    if (!isWritable())
        return;

    if (observedPolicies.size()) {
        beginHstsGroups();
        for (const QHstsPolicy &policy : qAsConst(observedPolicies)) {
            const QString key(host_name_to_settings_key(policy.host()));
            // If we fail to write a new, updated policy, we also remove the old one.
            if (policy.isExpired() || !serializePolicy(key, policy))
                evictPolicy(key);
        }
        observedPolicies.clear();
        endHstsGroups();
    }

    store.sync();
}

bool QHstsStore::isWritable() const
{
    return store.isWritable();
}

QString QHstsStore::absoluteFilePath(const QString &dirName)
{
    const QDir dir(dirName.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                     : dirName);
    return dir.absoluteFilePath(QLatin1String("hstsstore"));
}

void QHstsStore::beginHstsGroups()
{
    store.beginGroup(QLatin1String("StrictTransportSecurity"));
    store.beginGroup(QLatin1String("Policies"));
}

void QHstsStore::endHstsGroups()
{
    store.endGroup();
    store.endGroup();
}

bool QHstsStore::deserializePolicy(const QString &key, QHstsPolicy &policy)
{
    Q_ASSERT(store.contains(key));

    const QVariant data(store.value(key));
    if (data.isNull() || !data.canConvert<QByteArray>())
        return false;

    const QByteArray serializedData(data.toByteArray());
    QDataStream streamer(serializedData);
    qint64 expiryInMS = 0;
    streamer >> expiryInMS;
    if (streamer.status() != QDataStream::Ok)
        return false;
    bool includesSubDomains = false;
    streamer >> includesSubDomains;
    if (streamer.status() != QDataStream::Ok)
        return false;

    policy.setExpiry(QDateTime::fromMSecsSinceEpoch(expiryInMS));
    policy.setIncludesSubDomains(includesSubDomains);

    return true;
}

bool QHstsStore::serializePolicy(const QString &key, const QHstsPolicy &policy)
{
    Q_ASSERT(store.isWritable());

    QByteArray serializedData;
    QDataStream streamer(&serializedData, QIODevice::WriteOnly);
    streamer << policy.expiry().toMSecsSinceEpoch();
    streamer << policy.includesSubDomains();

    if (streamer.status() != QDataStream::Ok)
        return false;

    store.setValue(key, serializedData);
    return true;
}

void QHstsStore::evictPolicy(const QString &key)
{
    Q_ASSERT(store.isWritable());
    if (store.contains(key))
        store.remove(key);
}

QT_END_NAMESPACE
