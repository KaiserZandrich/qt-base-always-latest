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

#ifndef QJNI_ENVIRONMENT_H
#define QJNI_ENVIRONMENT_H

#include <QtCore/QScopedPointer>

#if defined(Q_QDOC) || (defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED))
#include <jni.h>

QT_BEGIN_NAMESPACE

class QJniEnvironmentPrivate;

class Q_CORE_EXPORT QJniEnvironment
{
public:
    QJniEnvironment();
    ~QJniEnvironment();
    bool isValid() const;
    JNIEnv *operator->() const;
    JNIEnv &operator*() const;
    JNIEnv *jniEnv() const;
    jclass findClass(const char *className);
    jmethodID findMethod(jclass clazz, const char *methodName, const char *signature);
    jmethodID findStaticMethod(jclass clazz, const char *methodName, const char *signature);
    jfieldID findField(jclass clazz, const char *fieldName, const char *signature);
    jfieldID findStaticField(jclass clazz, const char *fieldName, const char *signature);
    static JavaVM *javaVM();
    bool registerNativeMethods(const char *className, const JNINativeMethod methods[], int size);
    bool registerNativeMethods(jclass clazz, const JNINativeMethod methods[], int size);

#if QT_DEPRECATED_SINCE(6, 2)
    // ### Qt 7: remove
    QT_DEPRECATED_VERSION_X_6_2("Use the overload with a const JNINativeMethod[] instead.")
    bool registerNativeMethods(const char *className, JNINativeMethod methods[], int size);
#endif

    enum class OutputMode {
        Silent,
        Verbose
    };

    bool checkAndClearExceptions(OutputMode outputMode = OutputMode::Verbose);
    static bool checkAndClearExceptions(JNIEnv *env, OutputMode outputMode = OutputMode::Verbose);

private:
    Q_DISABLE_COPY_MOVE(QJniEnvironment)
    QScopedPointer<QJniEnvironmentPrivate> d;
};

QT_END_NAMESPACE

#endif

#endif // QJNI_ENVIRONMENT_H
