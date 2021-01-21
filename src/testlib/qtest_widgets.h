/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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
****************************************************************************/

#ifndef QTEST_WIDGETS_H
#define QTEST_WIDGETS_H

// enable WIDGETS features
#ifndef QT_WIDGETS_LIB
#define QT_WIDGETS_LIB
#endif
#if 0
#pragma qt_class(QtTestWidgets)
#endif

#include <QtTest/qtest_gui.h>

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtWidgets/QSizePolicy>
#include <QtCore/QMetaEnum>

QT_BEGIN_NAMESPACE

namespace QTest
{

//
// QSizePolicy & friends:
//

namespace Internal
{

inline const char *toString(QSizePolicy::Policy p)
{
    static const QMetaEnum me = QSizePolicy::staticMetaObject.enumerator(QSizePolicy::staticMetaObject.indexOfEnumerator("Policy"));
    return me.valueToKey(int(p));
}

inline QByteArray toString(QSizePolicy::ControlTypes ct)
{
    static const QMetaEnum me = QSizePolicy::staticMetaObject.enumerator(QSizePolicy::staticMetaObject.indexOfEnumerator("ControlTypes"));
    return me.valueToKeys(int(ct));
}

inline QByteArray toString(QSizePolicy sp)
{
    static const char comma[] = ", ";
    return QByteArray("QSizePolicy(")
            + Internal::toString(sp.horizontalPolicy()) + comma
            + Internal::toString(sp.verticalPolicy()) + comma
            + QByteArray::number(sp.horizontalStretch()) + comma
            + QByteArray::number(sp.verticalStretch()) + comma
            + Internal::toString(QSizePolicy::ControlTypes(sp.controlType())) + comma
            + "height for width: " + (sp.hasHeightForWidth() ? "yes" : "no") + comma
            + "width for height: " + (sp.hasWidthForHeight() ? "yes" : "no") + comma
            + (sp.retainSizeWhenHidden() ? "" : "don't " ) + "retain size when hidden"
            + ')';
}

} // namespace Internal

inline char *toString(QSizePolicy::Policy p)
{
    return qstrdup(Internal::toString(p));
}

inline char *toString(QSizePolicy::ControlTypes ct)
{
    return qstrdup(Internal::toString(ct).constData());
}

inline char *toString(QSizePolicy::ControlType ct)
{
    return toString(QSizePolicy::ControlTypes(ct));
}

inline char *toString(QSizePolicy sp)
{
    return qstrdup(Internal::toString(sp).constData());
}

} // namespace QTest

QT_END_NAMESPACE

#endif

