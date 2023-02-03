// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbusmessage.h"
#include "qdbusconnection.h"
#include "qdbusabstractadaptor.h"

#include "qdbuscontext.h"
#include "qdbuscontext_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QDBusContextPrivate *QDBusContextPrivate::set(QObject *obj, QDBusContextPrivate *newContext)
{
    // determine if this is an adaptor or not
    if (qobject_cast<QDBusAbstractAdaptor *>(obj))
        obj = obj->parent();

    Q_ASSERT(obj);

    void *ptr = obj->qt_metacast("QDBusContext");
    QDBusContext *q_ptr = reinterpret_cast<QDBusContext *>(ptr);
    if (q_ptr) {
        QDBusContextPrivate *old = q_ptr->d_ptr;
        q_ptr->d_ptr = newContext;
        return old;
    }

    return nullptr;
}

/*!
    \since 4.3
    \class QDBusContext
    \inmodule QtDBus

    \brief The QDBusContext class allows slots to determine the D-Bus context of the calls.

    When a slot is called in an object due to a signal delivery or due
    to a remote method call, it is sometimes necessary to know the
    context in which that happened. In particular, if the slot
    determines that it wants to send the reply at a later opportunity
    or if it wants to reply with an error, the context is needed.

    The QDBusContext class is an alternative to accessing the context
    that doesn't involve modifying the code generated by the \l
    {Qt D-Bus XML compiler (qdbusxml2cpp)}.

    QDBusContext is used by subclassing it from the objects being
    exported using QDBusConnection::registerObject(). The following
    example illustrates the usage:

    \snippet code/src_qdbus_qdbuscontext.cpp 0

    The example illustrates the two typical uses, that of sending
    error replies and that of delayed replies.

    Note: do not subclass QDBusContext and QDBusAbstractAdaptor at the
    same time. QDBusContext should appear in the real object, not the
    adaptor. If it's necessary from the adaptor code to determine the
    context, use a public inheritance and access the functions via
    QObject::parent().
*/

/*!
  Constructs an empty QDBusContext.
 */
QDBusContext::QDBusContext()
    : d_ptr(nullptr)
{
}

/*!
  An empty destructor.
 */
QDBusContext::~QDBusContext()
{
}

/*!
    Returns \c true if we are processing a D-Bus call. If this function
    returns \c true, the rest of the functions in this class are
    available.

    Accessing those functions when this function returns \c false is
    undefined and may lead to crashes.
*/
bool QDBusContext::calledFromDBus() const
{
    return d_ptr;
}

/*!
    Returns the connection from which this call was received.
*/
QDBusConnection QDBusContext::connection() const
{
    return d_ptr->connection;
}

/*!
    Returns the message that generated this call.
*/
const QDBusMessage &QDBusContext::message() const
{
    return d_ptr->message;
}

/*!
    Returns \c true if this call will have a delayed reply.

    \sa setDelayedReply()
*/
bool QDBusContext::isDelayedReply() const
{
    return message().isDelayedReply();
}

/*!
    Sets whether this call will have a delayed reply or not.

    If \a enable is false, Qt D-Bus will automatically generate a reply
    back to the caller, if needed, as soon as the called slot returns.

    If \a enable is true, Qt D-Bus will not generate automatic
    replies. It will also ignore the return value from the slot and
    any output parameters. Instead, the called object is responsible
    for storing the incoming message and send a reply or error at a
    later time.

    Failing to send a reply will result in an automatic timeout error
    being generated by D-Bus.
*/
void QDBusContext::setDelayedReply(bool enable) const
{
    message().setDelayedReply(enable);
}

/*!
    Sends an error \a name as a reply to the caller. The optional \a
    msg parameter is a human-readable text explaining the failure.

    If an error is sent, the return value and any output parameters
    from the called slot will be ignored by Qt D-Bus.
*/
void QDBusContext::sendErrorReply(const QString &name, const QString &msg) const
{
    setDelayedReply(true);
    connection().send(message().createErrorReply(name, msg));
}

/*!
    \overload
    Sends an error \a type as a reply to the caller. The optional \a
    msg parameter is a human-readable text explaining the failure.

    If an error is sent, the return value and any output parameters
    from the called slot will be ignored by Qt D-Bus.
*/
void QDBusContext::sendErrorReply(QDBusError::ErrorType type, const QString &msg) const
{
    setDelayedReply(true);
    connection().send(message().createErrorReply(type, msg));
}

/*!
    \overload
    Sends an error \a err as a reply to the caller.

    If an error is sent, the return value and any output parameters
    from the called slot will be ignored by Qt D-Bus.
*/
void QDBusContext::sendErrorReply(const QDBusError &err) const
{
    setDelayedReply(true);
    connection().send(message().createError(err));
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
