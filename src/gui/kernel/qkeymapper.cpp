/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
QKeyMapper::QKeyMapper()
    : QObject(*new QKeyMapperPrivate, nullptr)
{
}

/*!
    Destroys the key mapper.
*/
QKeyMapper::~QKeyMapper()
{
}

static QList<int> extractKeyFromEvent(QKeyEvent *e)
{
    QList<int> result;
    if (e->key() && (e->key() != Qt::Key_unknown))
        result << e->keyCombination().toCombined();
    else if (!e->text().isEmpty())
        result << int(e->text().at(0).unicode() + (int)e->modifiers());
    return result;
}

QList<int> QKeyMapper::possibleKeys(QKeyEvent *e)
{
    return instance()->d_func()->possibleKeys(e);
}

extern bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event); // in qapplication_*.cpp
void QKeyMapper::changeKeyboard()
{
    // ## TODO: Support KeyboardLayoutChange on QPA
#if 0
    // inform all toplevel widgets of the change
    QEvent e(QEvent::KeyboardLayoutChange);
    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; i < list.size(); ++i) {
        QWidget *w = list.at(i);
        qt_sendSpontaneousEvent(w, &e);
    }
#endif
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

QKeyMapperPrivate *qt_keymapper_private()
{
    return QKeyMapper::instance()->d_func();
}

QKeyMapperPrivate::QKeyMapperPrivate()
{
    keyboardInputLocale = QLocale::system();
    keyboardInputDirection = keyboardInputLocale.textDirection();
}

QKeyMapperPrivate::~QKeyMapperPrivate()
{
}

QList<int> QKeyMapperPrivate::possibleKeys(QKeyEvent *e)
{
    QList<int> result = QGuiApplicationPrivate::platformIntegration()->possibleKeys(e);
    if (!result.isEmpty())
        return result;

    return extractKeyFromEvent(e);
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
