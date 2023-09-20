// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qguiapplication.h"

#include <private/qobject_p.h>
#include "qkeymapper_p.h"

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

/*!
    \class QKeyMapper
    \since 4.2
    \internal

    \sa QObject
*/

/*!
    Constructs a new key mapper.
*/
QKeyMapper::QKeyMapper() : QObject()
{
}

/*!
    Destroys the key mapper.
*/
QKeyMapper::~QKeyMapper()
{
}

QList<int> QKeyMapper::possibleKeys(const QKeyEvent *e)
{
    QList<int> result = QGuiApplicationPrivate::platformIntegration()->possibleKeys(e);
    if (!result.isEmpty())
        return result;

    if (e->key() && (e->key() != Qt::Key_unknown))
        result << e->keyCombination().toCombined();
    else if (!e->text().isEmpty())
        result << int(e->text().at(0).unicode() + (int)e->modifiers());
    return result;
}

Q_GLOBAL_STATIC(QKeyMapper, keymapper)
/*!
    Returns the pointer to the single instance of QKeyMapper in the application.
    If none yet exists, the function ensures that one is created.
*/
QKeyMapper *QKeyMapper::instance()
{
    return keymapper();
}

void *QKeyMapper::resolveInterface(const char *name, int revision) const
{
    Q_UNUSED(name); Q_UNUSED(revision);
    using namespace QNativeInterface::Private;

#if QT_CONFIG(evdev)
    QT_NATIVE_INTERFACE_RETURN_IF(QEvdevKeyMapper, QGuiApplicationPrivate::platformIntegration());
#endif

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qkeymapper_p.cpp"
