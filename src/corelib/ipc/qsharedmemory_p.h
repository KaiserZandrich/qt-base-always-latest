// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHAREDMEMORY_P_H
#define QSHAREDMEMORY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsharedmemory.h"

#include <QtCore/qstring.h>

#if QT_CONFIG(sharedmemory)
#include "qsystemsemaphore.h"
#include "private/qobject_p.h"

#if QT_CONFIG(posix_shm)
#  include <sys/mman.h>
#endif
#if QT_CONFIG(sysv_shm)
#  include <sys/shm.h>
#endif

QT_BEGIN_NAMESPACE

class QSharedMemoryPrivate;

#if QT_CONFIG(systemsemaphore)
/*!
  Helper class
  */
class QSharedMemoryLocker
{

public:
    inline QSharedMemoryLocker(QSharedMemory *sharedMemory) : q_sm(sharedMemory)
    {
        Q_ASSERT(q_sm);
    }

    inline ~QSharedMemoryLocker()
    {
        if (q_sm)
            q_sm->unlock();
    }

    inline bool lock()
    {
        if (q_sm && q_sm->lock())
            return true;
        q_sm = nullptr;
        return false;
    }

private:
    QSharedMemory *q_sm;
};
#endif // QT_CONFIG(systemsemaphore)

class QSharedMemoryPosix
{
public:
    bool handle(QSharedMemoryPrivate *self);
    bool cleanHandle(QSharedMemoryPrivate *self);
    bool create(QSharedMemoryPrivate *self, qsizetype size);
    bool attach(QSharedMemoryPrivate *self, QSharedMemory::AccessMode mode);
    bool detach(QSharedMemoryPrivate *self);

    int hand = -1;
};

class QSharedMemorySystemV
{
public:
#if QT_CONFIG(sysv_sem)
    key_t handle(QSharedMemoryPrivate *self);
    bool cleanHandle(QSharedMemoryPrivate *self);
    bool create(QSharedMemoryPrivate *self, qsizetype size);
    bool attach(QSharedMemoryPrivate *self, QSharedMemory::AccessMode mode);
    bool detach(QSharedMemoryPrivate *self);

    key_t unix_key = 0;
#endif
};

class QSharedMemoryWin32
{
public:
    Qt::HANDLE handle(QSharedMemoryPrivate *self);
    bool cleanHandle(QSharedMemoryPrivate *self);
    bool create(QSharedMemoryPrivate *self, qsizetype size);
    bool attach(QSharedMemoryPrivate *self, QSharedMemory::AccessMode mode);
    bool detach(QSharedMemoryPrivate *self);

    Qt::HANDLE hand = nullptr;
};

class Q_AUTOTEST_EXPORT QSharedMemoryPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSharedMemory)

public:
    void *memory = nullptr;
    qsizetype size = 0;
    QString key;
    QString nativeKey;
    QString errorString;
#if QT_CONFIG(systemsemaphore)
    QSystemSemaphore systemSemaphore{ QNativeIpcKey() };
    bool lockedByMe = false;
#endif
    QSharedMemory::SharedMemoryError error = QSharedMemory::NoError;

#if defined(Q_OS_WIN)
    using DefaultBackend = QSharedMemoryWin32;
#elif defined(QT_POSIX_IPC)
    using DefaultBackend = QSharedMemoryPosix;
#else
    using DefaultBackend = QSharedMemorySystemV;
#endif
    DefaultBackend backend;

    bool initKey();

    bool handle()
    {
        return backend.handle(this);
    }
    bool cleanHandle()
    {
        return backend.cleanHandle(this);
    }
    bool create(qsizetype size)
    {
        return backend.create(this, size);
    }
    bool attach(QSharedMemory::AccessMode mode)
    {
        return backend.attach(this, mode);
    }
    bool detach()
    {
        return backend.detach(this);
    }

    inline void setError(QSharedMemory::SharedMemoryError e, const QString &message)
    { error = e; errorString = message; }
    void setUnixErrorString(QLatin1StringView function);
    void setWindowsErrorString(QLatin1StringView function);

#if QT_CONFIG(systemsemaphore)
    bool tryLocker(QSharedMemoryLocker *locker, const QString &function) {
        if (!locker->lock()) {
            errorString = QSharedMemory::tr("%1: unable to lock").arg(function);
            error = QSharedMemory::LockError;
            return false;
        }
        return true;
    }
    QNativeIpcKey semaphoreNativeKey() const;
#endif // QT_CONFIG(systemsemaphore)
};

QT_END_NAMESPACE

#endif // QT_CONFIG(sharedmemory)

#endif // QSHAREDMEMORY_P_H

