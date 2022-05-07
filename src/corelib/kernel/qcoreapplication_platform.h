/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QCOREAPPLICATION_PLATFORM_H
#define QCOREAPPLICATION_PLATFORM_H

#include <QtCore/qglobal.h>
#include <QtCore/qnativeinterface.h>
#include <QtCore/qcoreapplication.h>

#if defined(Q_OS_ANDROID) || defined(Q_CLANG_QDOC)
#if QT_CONFIG(future) && !defined(QT_NO_QOBJECT)
#include <QtCore/qfuture.h>
#include <QtCore/qvariant.h>
#endif
#endif // #if defined(Q_OS_ANDROID) || defined(Q_CLANG_QDOC)

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
class _jobject;
typedef _jobject* jobject;
#endif

QT_BEGIN_NAMESPACE

namespace QNativeInterface
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED) || defined(Q_CLANG_QDOC)
struct Q_CORE_EXPORT QAndroidApplication
{
    QT_DECLARE_NATIVE_INTERFACE(QAndroidApplication, 1, QCoreApplication)
    static jobject context();
    static bool isActivityContext();
    static int sdkVersion();
    static void hideSplashScreen(int duration = 0);

#if QT_CONFIG(future) && !defined(QT_NO_QOBJECT)
    static QFuture<QVariant> runOnAndroidMainThread(const std::function<QVariant()> &runnable,
                                            const QDeadlineTimer timeout = QDeadlineTimer::Forever);

    template <class T>
    std::enable_if_t<std::is_invocable_v<T> && std::is_same_v<std::invoke_result_t<T>, void>,
    QFuture<void>> static runOnAndroidMainThread(const T &runnable,
                                            const QDeadlineTimer timeout = QDeadlineTimer::Forever)
    {
        std::function<QVariant()> func = [runnable](){ runnable(); return QVariant(); };
        return static_cast<QFuture<void>>(runOnAndroidMainThread(func, timeout));
    }
#endif
};
#endif
}

QT_END_NAMESPACE

#endif // QCOREAPPLICATION_PLATFORM_H
