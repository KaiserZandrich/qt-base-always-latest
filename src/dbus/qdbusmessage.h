/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

#ifndef QDBUSMESSAGE_H
#define QDBUSMESSAGE_H

#include <QtDBus/qtdbusglobal.h>
#include <QtDBus/qdbuserror.h>
#include <QtCore/qlist.h>
#include <QtCore/qvariant.h>

#if !defined(QT_NO_DBUS) && !defined(QT_BOOTSTRAPPED)

#if defined(Q_OS_WIN) && defined(interface)
#  undef interface
#endif

QT_BEGIN_NAMESPACE

class QDBusMessagePrivate;
class Q_DBUS_EXPORT QDBusMessage
{
public:
    enum MessageType {
        InvalidMessage,
        MethodCallMessage,
        ReplyMessage,
        ErrorMessage,
        SignalMessage
    };

    QDBusMessage();
    QDBusMessage(const QDBusMessage &other);
    QDBusMessage &operator=(QDBusMessage &&other) noexcept { swap(other); return *this; }
    QDBusMessage &operator=(const QDBusMessage &other);
    ~QDBusMessage();

    void swap(QDBusMessage &other) noexcept { qt_ptr_swap(d_ptr, other.d_ptr); }

    static QDBusMessage createSignal(const QString &path, const QString &interface,
                                     const QString &name);
    static QDBusMessage createTargetedSignal(const QString &service, const QString &path,
                                             const QString &interface, const QString &name);
    static QDBusMessage createMethodCall(const QString &destination, const QString &path,
                                         const QString &interface, const QString &method);
    static QDBusMessage createError(const QString &name, const QString &msg);
    static inline QDBusMessage createError(const QDBusError &err)
    { return createError(err.name(), err.message()); }
    static inline QDBusMessage createError(QDBusError::ErrorType type, const QString &msg)
    { return createError(QDBusError::errorString(type), msg); }

    QDBusMessage createReply(const QList<QVariant> &arguments = QList<QVariant>()) const;
    inline QDBusMessage createReply(const QVariant &argument) const
    { return createReply(QList<QVariant>() << argument); }

    QDBusMessage createErrorReply(const QString &name, const QString &msg) const;
    inline QDBusMessage createErrorReply(const QDBusError &err) const
    { return createErrorReply(err.name(), err.message()); }
    QDBusMessage createErrorReply(QDBusError::ErrorType type, const QString &msg) const;

    // there are no setters; if this changes, see qdbusmessage_p.h
    QString service() const;
    QString path() const;
    QString interface() const;
    QString member() const;
    QString errorName() const;
    QString errorMessage() const;
    MessageType type() const;
    QString signature() const;

    bool isReplyRequired() const;

    void setDelayedReply(bool enable) const;
    bool isDelayedReply() const;

    void setAutoStartService(bool enable);
    bool autoStartService() const;

    void setInteractiveAuthorizationAllowed(bool enable);
    bool isInteractiveAuthorizationAllowed() const;

    void setArguments(const QList<QVariant> &arguments);
    QList<QVariant> arguments() const;

    QDBusMessage &operator<<(const QVariant &arg);

private:
    friend class QDBusMessagePrivate;
    QDBusMessagePrivate *d_ptr;
};
Q_DECLARE_SHARED(QDBusMessage)

#ifndef QT_NO_DEBUG_STREAM
Q_DBUS_EXPORT QDebug operator<<(QDebug, const QDBusMessage &);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QDBusMessage)

#else
class Q_DBUS_EXPORT QDBusMessage {}; // dummy class for moc
#endif // QT_NO_DBUS
#endif

